#include <cstring>
#include <set>

#include <vts/map.hpp>
#include <vts/statistics.hpp>
#include <vts/options.hpp>
#include <vts/view.hpp>

#include "mainWindow.hpp"
#include <nuklear.h>
#include <GLFW/glfw3.h>
#include "guiSkin.hpp"

Mark::Mark() : open(false)
{}

class GuiImpl
{
public:
    struct vertex
    {
        float position[2];
        float uv[2];
        nk_byte col[4];
    };
    
    GuiImpl(MainWindow *window) : window(window),
        consumeEvents(true), prepareFirst(true),
        statTraversedDetails(false), statRenderedDetails(false),
        optSensitivityDetails(false), positionSrs(2), autoPan(0)
    {
        { // load font
            struct nk_font_config cfg;
            memset(&cfg, 0, sizeof(cfg));
            cfg.oversample_h = 6;
            cfg.oversample_v = 6;
            nk_font_atlas_init_default(&atlas);
            nk_font_atlas_begin(&atlas);
            vts::Buffer buffer = vts::readInternalMemoryBuffer(
                        "data/fonts/roboto-regular.ttf");
            font = nk_font_atlas_add_from_memory(&atlas,
                buffer.data(), buffer.size(), 14, &cfg);
            vts::GpuTextureSpec spec;
            spec.verticalFlip = false;
            const void* img = nk_font_atlas_bake(&atlas,
                (int*)&spec.width, (int*)&spec.height, NK_FONT_ATLAS_RGBA32);
            spec.components = 4;
            spec.buffer.allocate(spec.width * spec.height * spec.components);
            memcpy(spec.buffer.data(), img, spec.buffer.size());
            fontTexture = std::make_shared<GpuTextureImpl>("font texture");
            fontTexture->loadTexture(spec);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            nk_font_atlas_end(&atlas, nk_handle_id(fontTexture->id), &null);
        }
        
        nk_init_default(&ctx, &font->handle);
        nk_buffer_init_default(&cmds);
        
        static const nk_draw_vertex_layout_element vertex_layout[] =
        {
            { NK_VERTEX_POSITION, NK_FORMAT_FLOAT, 0 },
            { NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, 8 },
            { NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, 16 },
            { NK_VERTEX_LAYOUT_END }
        };
        memset(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(vertex);
        config.vertex_alignment = alignof(vertex);
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = NK_ANTI_ALIASING_ON;
        config.line_AA = NK_ANTI_ALIASING_ON;
        config.null = null;
        
        initializeGuiSkin(ctx, skinMedia, skinTexture);
        
        { // load shader
            shader = std::make_shared<GpuShaderImpl>();
            vts::Buffer vert = vts::readInternalMemoryBuffer(
                        "data/shaders/gui.vert.glsl");
            vts::Buffer frag = vts::readInternalMemoryBuffer(
                        "data/shaders/gui.frag.glsl");
            shader->loadShaders(
                std::string(vert.data(), vert.size()),
                std::string(frag.data(), frag.size()));
            std::vector<vts::uint32> &uls = shader->uniformLocations;
            GLuint id = shader->id;
            uls.push_back(glGetUniformLocation(id, "ProjMtx"));
            glUseProgram(id);
            glUniform1i(glGetUniformLocation(id, "Texture"), 0);
        }
        
        { // prepare mesh buffers
            mesh = std::make_shared<GpuMeshImpl>("gui mesh");
            glGenVertexArrays(1, &mesh->vao);
            glGenBuffers(1, &mesh->vbo);
            glGenBuffers(1, &mesh->vio);
            glBindVertexArray(mesh->vao);
            glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->vio);
            glBufferData(GL_ARRAY_BUFFER, MaxVertexMemory,
                         NULL, GL_STREAM_DRAW);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, MaxElementMemory,
                         NULL, GL_STREAM_DRAW);
            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                sizeof(vertex), (void*)0);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 
                sizeof(vertex), (void*)8);
            glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                sizeof(vertex), (void*)16);
        }
    }

    ~GuiImpl()
    {
        nk_buffer_free(&cmds);
        nk_font_atlas_clear(&atlas);
        nk_free(&ctx);
    }

    void dispatch(int width, int height)
    {
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(mesh->vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->vio);
        shader->bind();
        
        { // proj matrix
            GLfloat ortho[4][4] = {
                    {2.0f, 0.0f, 0.0f, 0.0f},
                    {0.0f,-2.0f, 0.0f, 0.0f},
                    {0.0f, 0.0f,-1.0f, 0.0f},
                    {-1.0f,1.0f, 0.0f, 1.0f},
            };
            ortho[0][0] /= (GLfloat)width;
            ortho[1][1] /= (GLfloat)height;
            glUniformMatrix4fv(shader->uniformLocations[0], 1,
                    GL_FALSE, &ortho[0][0]);
        }
        
        { // upload buffer data
            void *vertices = glMapBuffer(GL_ARRAY_BUFFER,
                                         GL_WRITE_ONLY);
            void *elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER,
                                         GL_WRITE_ONLY);
            nk_buffer vbuf, ebuf;
            nk_buffer_init_fixed(&vbuf, vertices, MaxVertexMemory);
            nk_buffer_init_fixed(&ebuf, elements, MaxElementMemory);
            nk_convert(&ctx, &cmds, &vbuf, &ebuf, &config);
            glUnmapBuffer(GL_ARRAY_BUFFER);
            glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        }
        
        { // draw commands
            struct nk_vec2 scale;
            scale.x = 1;
            scale.y = 1;
            const nk_draw_command *cmd;
            const nk_draw_index *offset = NULL;
            nk_draw_foreach(cmd, &ctx, &cmds)
            {
                if (!cmd->elem_count)
                    continue;
                glBindTexture(GL_TEXTURE_2D, cmd->texture.id);
                glScissor(
                    (GLint)(cmd->clip_rect.x * scale.x),
                    (GLint)((height - (GLint)(cmd->clip_rect.y
                                              + cmd->clip_rect.h)) * scale.y),
                    (GLint)(cmd->clip_rect.w * scale.x),
                    (GLint)(cmd->clip_rect.h * scale.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count,
                               GL_UNSIGNED_SHORT, offset);
                offset += cmd->elem_count;
            }
        }
        
        nk_clear(&ctx);
        
        glUseProgram(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glDisable(GL_SCISSOR_TEST);
    }

    void input()
    {
        consumeEvents = nk_item_is_any_active(&ctx);
        nk_input_begin(&ctx);
        glfwPollEvents();
        nk_input_key(&ctx, NK_KEY_DEL, glfwGetKey(window->window,
                GLFW_KEY_DELETE) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_ENTER, glfwGetKey(window->window,
                GLFW_KEY_ENTER) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_TAB, glfwGetKey(window->window,
                GLFW_KEY_TAB) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_BACKSPACE, glfwGetKey(window->window,
                GLFW_KEY_BACKSPACE) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_LEFT, glfwGetKey(window->window,
                GLFW_KEY_LEFT) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_RIGHT, glfwGetKey(window->window,
                GLFW_KEY_RIGHT) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_UP, glfwGetKey(window->window,
                GLFW_KEY_UP) == GLFW_PRESS);
        nk_input_key(&ctx, NK_KEY_DOWN, glfwGetKey(window->window,
                GLFW_KEY_DOWN) == GLFW_PRESS);
        if (glfwGetKey(window->window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window->window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
        {
            nk_input_key(&ctx, NK_KEY_COPY, glfwGetKey(window->window,
                    GLFW_KEY_C) == GLFW_PRESS);
            nk_input_key(&ctx, NK_KEY_PASTE, glfwGetKey(window->window,
                    GLFW_KEY_P) == GLFW_PRESS);
            nk_input_key(&ctx, NK_KEY_CUT, glfwGetKey(window->window,
                    GLFW_KEY_X) == GLFW_PRESS);
            nk_input_key(&ctx, NK_KEY_CUT, glfwGetKey(window->window,
                    GLFW_KEY_E) == GLFW_PRESS);
            nk_input_key(&ctx, NK_KEY_SHIFT, 1);
        } 
        else
        {
            nk_input_key(&ctx, NK_KEY_COPY, 0);
            nk_input_key(&ctx, NK_KEY_PASTE, 0);
            nk_input_key(&ctx, NK_KEY_CUT, 0);
            nk_input_key(&ctx, NK_KEY_SHIFT, 0);
        }
        double x, y;
        glfwGetCursorPos(window->window, &x, &y);
        nk_input_motion(&ctx, (int)x, (int)y);
        nk_input_button(&ctx, NK_BUTTON_LEFT, (int)x, (int)y,
                        glfwGetMouseButton(window->window,
                        GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        nk_input_button(&ctx, NK_BUTTON_MIDDLE, (int)x, (int)y,
                        glfwGetMouseButton(window->window,
                        GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
        nk_input_button(&ctx, NK_BUTTON_RIGHT, (int)x, (int)y,
                        glfwGetMouseButton(window->window,
                        GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
        nk_input_end(&ctx);
    }

    void mousePositionCallback(double xpos, double ypos)
    {
        if (!consumeEvents)
            window->mousePositionCallback(xpos, ypos);
    }
    
    void mouseButtonCallback(int button, int action, int mods)
    {
        if (!consumeEvents)
            window->mouseButtonCallback(button, action, mods);
    }

    void mouseScrollCallback(double xoffset, double yoffset)
    {
        struct nk_vec2 pos;
        pos.x = xoffset;
        pos.y = yoffset;
        nk_input_scroll(&ctx, pos);
        if (!consumeEvents)
            window->mouseScrollCallback(xoffset, yoffset);
    }
    
    void keyboardCallback(int key, int scancode, int action, int mods)
    {
        if (!consumeEvents)
            window->keyboardCallback(key, scancode, action, mods);
    }
    
    void keyboardUnicodeCallback(unsigned int codepoint)
    {
        nk_input_unicode(&ctx, codepoint);
        if (!consumeEvents)
            window->keyboardUnicodeCallback(codepoint);
    }

    void prepareOptions()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Options", nk_rect(10, 10, 250, 400), flags))
        {
            vts::MapOptions &o = window->map->options();
            float width = nk_window_get_content_region_size(&ctx).x - 15;
            float ratio[] = { width * 0.4, width * 0.5, width * 0.1 };
            nk_layout_row(&ctx, NK_STATIC, 16, 3, ratio);
            char buffer[256];
            
            // camera control sensitivity
            nk_label(&ctx, "Mouse sensitivity:", NK_TEXT_LEFT);
            nk_checkbox_label(&ctx, "", &optSensitivityDetails);
            nk_label(&ctx, "", NK_TEXT_LEFT);
            if (optSensitivityDetails)
            {
                nk_layout_row(&ctx, NK_STATIC, 16, 3, ratio);
                nk_label(&ctx, "Pan speed:", NK_TEXT_LEFT);
                o.cameraSensitivityPan = nk_slide_float(&ctx,
                        0.1, o.cameraSensitivityPan, 3, 0.01);
                sprintf(buffer, "%4.2f", o.cameraSensitivityPan);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Vertical speed:", NK_TEXT_LEFT);
                o.cameraSensitivityAltitude = nk_slide_float(&ctx,
                        0.1, o.cameraSensitivityAltitude, 3, 0.01);
                sprintf(buffer, "%4.2f", o.cameraSensitivityAltitude);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Zoom speed:", NK_TEXT_LEFT);
                o.cameraSensitivityZoom = nk_slide_float(&ctx,
                        0.1, o.cameraSensitivityZoom, 3, 0.01);
                sprintf(buffer, "%4.2f", o.cameraSensitivityZoom);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Rotate speed:", NK_TEXT_LEFT);
                o.cameraSensitivityRotate = nk_slide_float(&ctx,
                        0.1, o.cameraSensitivityRotate, 3, 0.01);
                sprintf(buffer, "%4.2f", o.cameraSensitivityRotate);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                
                nk_label(&ctx, "Pan inertia:", NK_TEXT_LEFT);
                o.cameraInertiaPan = nk_slide_float(&ctx,
                        0, o.cameraInertiaPan, 0.99, 0.01);
                sprintf(buffer, "%4.2f", o.cameraInertiaPan);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Vertical inertia:", NK_TEXT_LEFT);
                o.cameraInertiaAltitude = nk_slide_float(&ctx,
                        0, o.cameraInertiaAltitude, 0.99, 0.01);
                sprintf(buffer, "%4.2f", o.cameraInertiaAltitude);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Zoom inertia:", NK_TEXT_LEFT);
                o.cameraInertiaZoom = nk_slide_float(&ctx,
                        0, o.cameraInertiaZoom, 0.99, 0.01);
                sprintf(buffer, "%4.2f", o.cameraInertiaZoom);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Rotate inertia:", NK_TEXT_LEFT);
                o.cameraInertiaRotate = nk_slide_float(&ctx,
                        0, o.cameraInertiaRotate, 0.99, 0.01);
                sprintf(buffer, "%4.2f", o.cameraInertiaRotate);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                
                nk_label(&ctx, "", NK_TEXT_LEFT);
                if (nk_button_label(&ctx, "Reset sensitivity"))
                {
                    vts::MapOptions d;
                    o.cameraSensitivityPan       = d.cameraSensitivityPan;
                    o.cameraSensitivityAltitude  = d.cameraSensitivityAltitude;
                    o.cameraSensitivityZoom      = d.cameraSensitivityZoom;
                    o.cameraSensitivityRotate    = d.cameraSensitivityRotate;
                    o.cameraInertiaPan           = d.cameraInertiaPan;
                    o.cameraInertiaAltitude      = d.cameraInertiaAltitude;
                    o.cameraInertiaZoom          = d.cameraInertiaZoom;
                    o.cameraInertiaRotate        = d.cameraInertiaRotate;
                }
                nk_label(&ctx, "", NK_TEXT_LEFT);
            }
                
            // maxTexelToPixelScale
            nk_label(&ctx, "Detail control:", NK_TEXT_LEFT);
            o.maxTexelToPixelScale = nk_slide_float(&ctx,
                    1, o.maxTexelToPixelScale, 5, 0.01);
            sprintf(buffer, "%3.1f", o.maxTexelToPixelScale);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            
            // maxResourcesMemory
            nk_label(&ctx, "Max memory:", NK_TEXT_LEFT);
            o.maxResourcesMemory = 1024 * 1024 * nk_slide_int(&ctx,
                    128, o.maxResourcesMemory / 1024 / 1024, 2048, 32);
            sprintf(buffer, "%3d", o.maxResourcesMemory / 1024 / 1024);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            
            // maxConcurrentDownloads
            nk_label(&ctx, "Max downloads:", NK_TEXT_LEFT);
            o.maxConcurrentDownloads = nk_slide_int(&ctx,
                    1, o.maxConcurrentDownloads, 50, 1);
            sprintf(buffer, "%3d", o.maxConcurrentDownloads);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            
            // maxResourceProcessesPerTick
            nk_label(&ctx, "Max res. procs.:", NK_TEXT_LEFT);
            o.maxResourceProcessesPerTick = nk_slide_int(&ctx,
                    1, o.maxResourceProcessesPerTick, 50, 1);
            sprintf(buffer, "%3d", o.maxResourceProcessesPerTick);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            
            // maxNodeUpdatesPerFrame
            nk_label(&ctx, "Max updates:", NK_TEXT_LEFT);
            o.maxNodeUpdatesPerTick = nk_slide_int(&ctx,
                    1, o.maxNodeUpdatesPerTick, 50, 1);
            sprintf(buffer, "%3d", o.maxNodeUpdatesPerTick);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            
            // navigation samples per view extent
            nk_label(&ctx, "Nav. samples:", NK_TEXT_LEFT);
            o.navigationSamplesPerViewExtent = nk_slide_int(&ctx,
                    1, o.navigationSamplesPerViewExtent, 16, 1);
            sprintf(buffer, "%3d", o.navigationSamplesPerViewExtent);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            
            // render mesh wire boxes
            nk_label(&ctx, "Display:", NK_TEXT_LEFT);
            o.renderMeshBoxes = nk_check_label(&ctx, "mesh boxes",
                                               o.renderMeshBoxes);
            nk_label(&ctx, "", NK_TEXT_LEFT);
            
            // render tile corners
            nk_label(&ctx, "", NK_TEXT_LEFT);
            o.renderTileBoxes = nk_check_label(&ctx, "tile boxes",
                                                o.renderTileBoxes);
            nk_label(&ctx, "", NK_TEXT_LEFT);
            
            // render surrogates
            nk_label(&ctx, "", NK_TEXT_LEFT);
            o.renderSurrogates = nk_check_label(&ctx, "surrogates",
                                                o.renderSurrogates);
            nk_label(&ctx, "", NK_TEXT_LEFT);
            
            // render objective position
            nk_label(&ctx, "", NK_TEXT_LEFT);
            o.renderObjectPosition = nk_check_label(&ctx, "object. pos.",
                                                o.renderObjectPosition);
            nk_label(&ctx, "", NK_TEXT_LEFT);
            
            // detached camera
            nk_label(&ctx, "Debug:", NK_TEXT_LEFT);
            o.debugDetachedCamera = nk_check_label(&ctx, "detached camera",
                                                o.debugDetachedCamera);
            nk_label(&ctx, "", NK_TEXT_LEFT);
            
            { // debug disable meta 5
                nk_label(&ctx, "", NK_TEXT_LEFT);
                bool old = o.debugDisableMeta5;
                o.debugDisableMeta5 = nk_check_label(&ctx, "disable meta5",
                                                    o.debugDisableMeta5);
                nk_label(&ctx, "", NK_TEXT_LEFT);
                if (old != o.debugDisableMeta5)
                    window->map->purgeTraverseCache(false);
            }
            
            // print debug info
            nk_label(&ctx, "", NK_TEXT_LEFT);
            if (nk_button_label(&ctx, "Print debug info"))
                window->map->printDebugInfo();
            nk_label(&ctx, "", NK_TEXT_LEFT);
        }
        nk_end(&ctx);
    }

    void prepareStatistics()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Statistics", nk_rect(270, 10, 250, 500), flags))
        {
            vts::MapStatistics &s = window->map->statistics();
            float width = nk_window_get_content_region_size(&ctx).x - 15;
            float ratio[] = { width * 0.5, width * 0.5 };
            nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
            char buffer[256];
#define S(NAME, VAL, UNIT) { \
                nk_label(&ctx, NAME, NK_TEXT_LEFT); \
                sprintf(buffer, "%d" UNIT, VAL); \
                nk_label(&ctx, buffer, NK_TEXT_RIGHT); \
            }
            // general
            S("Time map:", (int)(1000 * window->timingMapProcess), " ms");
            S("Time app:", (int)(1000 * window->timingAppProcess), " ms");
            S("Time gui:", (int)(1000 * window->timingGuiProcess), " ms");
            S("Time frame:", (int)(1000 * window->timingTotalFrame), " ms");
            S("Frame index:", s.frameIndex, "");
            S("Downloading:", s.currentResourceDownloads, "");
            S("Node updates:", s.currentNodeUpdates, "");
            S("Gpu Memory:", s.currentGpuMemUse / 1024 / 1024, " MB");
            S("Nav. lod:", s.lastHeightRequestLod, "");
            nk_label(&ctx, "Z range:", NK_TEXT_LEFT);
            sprintf(buffer, "%0.0f - %0.0f", window->camNear, window->camFar);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            // resources
            S("Res. active:", s.currentResources, "");
            S("Res. preparing:", s.currentResourcePreparing, "");
            S("Res. downloaded:", s.resourcesDownloaded, "");
            S("Res. disk loaded:", s.resourcesDiskLoaded, "");
            S("Res. processed:", s.resourcesProcessLoaded, "");
            S("Res. released:", s.resourcesReleased, "");
            S("Res. ignored:", s.resourcesIgnored, "");
            S("Res. failed:", s.resourcesFailed, "");
            // traversed
            S("Traversed:", s.metaNodesTraversedTotal, "");
            nk_label(&ctx, "", NK_TEXT_LEFT);
            nk_checkbox_label(&ctx, "details", &statTraversedDetails);
            if (statTraversedDetails)
            {
                for (int i = 0; i < vts::MapStatistics::MaxLods; i++)
                {
                    if (s.metaNodesTraversedPerLod[i] == 0)
                        continue;
                    sprintf(buffer, "[%d]:", i);
                    S(buffer, s.metaNodesTraversedPerLod[i], "");
                }
            }
            // rendered
            S("Rendered:", s.meshesRenderedTotal, "");
            nk_label(&ctx, "", NK_TEXT_LEFT);
            nk_checkbox_label(&ctx, "details", &statRenderedDetails);
            if (statRenderedDetails)
            {
                for (int i = 0; i < vts::MapStatistics::MaxLods; i++)
                {
                    if (s.meshesRenderedPerLod[i] == 0)
                        continue;
                    sprintf(buffer, "[%d]:", i);
                    S(buffer, s.meshesRenderedPerLod[i], "");
                }
            }
#undef S
        }
        nk_end(&ctx);
    }

    void preparePosition()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Position", nk_rect(530, 10, 200, 400), flags))
        {
            float width = nk_window_get_content_region_size(&ctx).x - 15;
            float ratio[] = { width * 0.4, width * 0.6 };
            nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
            char buffer[256];
            { // subjective position
                int subj = window->map->getPositionSubjective();
                int prev = subj;
                nk_label(&ctx, "Type:", NK_TEXT_LEFT);
                nk_checkbox_label(&ctx, "subjective", &subj);
                if (subj != prev)
                    window->map->setPositionSubjective(!!subj, true);
            }
            { // srs
                static const char *names[] = {
                    "Physical",
                    "Navigation",
                    "Public",
                };
                nk_label(&ctx, "Srs:", NK_TEXT_LEFT);
                if (nk_combo_begin_label(&ctx,
                                 names[positionSrs],
                                 nk_vec2(nk_widget_width(&ctx), 200)))
                {
                    nk_layout_row_dynamic(&ctx, 16, 1);
                    for (int i = 0; i < 3; i++)
                        if (nk_combo_item_label(&ctx, names[i], NK_TEXT_LEFT))
                            positionSrs = i;
                    nk_combo_end(&ctx);
                }
            }
            nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
            { // position
                vts::Point n = window->map->getPositionPoint();
                n = window->map->convert(n, vts::Srs::Navigation,
                                         (vts::Srs)positionSrs);
                nk_label(&ctx, "X:", NK_TEXT_LEFT);
                sprintf(buffer, "%.8f", n.x);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Y:", NK_TEXT_LEFT);
                sprintf(buffer, "%.8f", n.y);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Z:", NK_TEXT_LEFT);
                sprintf(buffer, "%.8f", n.z);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Auto: ", NK_TEXT_LEFT);
                nk_checkbox_label(&ctx, "", &autoPan);
                if (autoPan)
                    window->map->pan(vts::Point(0, 1, 0));
                nk_label(&ctx, "", NK_TEXT_LEFT);
                if (nk_button_label(&ctx, "Reset altitude"))
                    window->map->resetPositionAltitude();
            }
            { // rotation
                vts::Point r = window->map->getPositionRotation();
                nk_label(&ctx, "Rotation:", NK_TEXT_LEFT);
                sprintf(buffer, "%5.1f", r.x);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "", NK_TEXT_LEFT);
                sprintf(buffer, "%5.1f", r.y);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "", NK_TEXT_LEFT);
                sprintf(buffer, "%5.1f", r.z);
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                nk_label(&ctx, "Auto:", NK_TEXT_LEFT);
                window->map->setAutorotate(nk_slide_float(&ctx,
                        0, window->map->getAutorotate(), 1, 0.001));
                nk_label(&ctx, "", NK_TEXT_LEFT);
                if (nk_button_label(&ctx, "Reset rotation"))
                {
                    r.x = 0;
                    r.y = 270;
                    r.z = 0;
                    window->map->setPositionRotation(r);
                    window->map->setAutorotate(0);
                }
            }
            { // view extent
                nk_label(&ctx, "View extent:", NK_TEXT_LEFT);
                sprintf(buffer, "%10.1f", window->map->getPositionViewExtent());
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            }
            { // fov
                nk_label(&ctx, "Fov:", NK_TEXT_LEFT);
                window->map->setPositionFov(nk_slide_float(&ctx, 10,
                                    window->map->getPositionFov(), 100, 1));
                nk_label(&ctx, "", NK_TEXT_LEFT);
                sprintf(buffer, "%5.1f", window->map->getPositionFov());
                nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            }
        }
        nk_end(&ctx);
    }

    bool prepareViewsBoundLayers(vts::MapView &view,
                                 vts::MapView::BoundLayerInfo::Map &bl)
    {
        const std::vector<std::string> boundLayers
                = window->map->getResourceBoundLayers();
        std::set<std::string> bls(boundLayers.begin(), boundLayers.end());
        float width = nk_window_get_content_region_size(&ctx).x - 15 - 10 - 25;
        float ratio[] = { 10, width * 0.7, width * 0.3, 20};
        nk_layout_row(&ctx, NK_STATIC, 16, 4, ratio);
        bool changed = false;
        int idx = 0;
        for (auto &&bn : bl)
        {
            nk_label(&ctx, "", NK_TEXT_LEFT);
            if (!nk_check_label(&ctx, bn.id.c_str(), 1))
            {
                bl.erase(bl.begin() + idx);
                return true;
            }
            bls.erase(bn.id);
            // alpha
            double a2 = nk_slide_float(&ctx, 0.1, bn.alpha , 1, 0.1);
            if (bn.alpha != a2)
            {
                bn.alpha = a2;
                changed = true;
            }
            // arrows
            if (idx > 0)
            {
                if (nk_button_label(&ctx, "S"))
                {
                    std::swap(bl[idx - 1], bl[idx]);
                    return true;
                }
            }
            else
                nk_label(&ctx, "", NK_TEXT_LEFT);
            idx++;
        }
        for (auto &&bn : bls)
        {
            nk_label(&ctx, "", NK_TEXT_LEFT);
            if (nk_check_label(&ctx, bn.c_str(), 0))
            {
                bl.push_back(vts::MapView::BoundLayerInfo(bn));
                return true;
            }
            nk_label(&ctx, "", NK_TEXT_LEFT);
            nk_label(&ctx, "", NK_TEXT_LEFT);
        }
        return changed;
    }
    
    void prepareViews()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Views", nk_rect(740, 10, 300, 400), flags))
        {
            float width = nk_window_get_content_region_size(&ctx).x - 15;
            
            if (window->mapConfigPaths.size() > 1)
            { // mapconfig selector
                float ratio[] = { width * 0.2, width * 0.8 };
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
                nk_label(&ctx, "Config:", NK_TEXT_LEFT);
                if (nk_combo_begin_label(&ctx,
                                 window->map->getMapConfigPath().c_str(),
                                 nk_vec2(nk_widget_width(&ctx), 200)))
                {
                    const std::vector<std::string> &names
                            = window->mapConfigPaths;
                    nk_layout_row_dynamic(&ctx, 16, 1);
                    for (int i = 0, e = names.size(); i < e; i++)
                    {
                        if (nk_combo_item_label(&ctx, names[i].c_str(),
                                                NK_TEXT_LEFT))
                        {
                            window->marks.clear();
                            window->map->setMapConfigPath(names[i]);
                            nk_combo_end(&ctx);
                            nk_end(&ctx);
                            return;
                        }
                    }
                    nk_combo_end(&ctx);
                }
            }
            
            if (!window->map->getViewNames().empty())
            { // view selector
                float ratio[] = { width * 0.2, width * 0.8 };
                nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
                nk_label(&ctx, "View:", NK_TEXT_LEFT);
                if (nk_combo_begin_label(&ctx,
                                 window->map->getViewCurrent().c_str(),
                                 nk_vec2(nk_widget_width(&ctx), 200)))
                {
                    std::vector<std::string> names
                            = window->map->getViewNames();
                    nk_layout_row_dynamic(&ctx, 16, 1);
                    for (int i = 0, e = names.size(); i < e; i++)
                        if (nk_combo_item_label(&ctx, names[i].c_str(),
                                                NK_TEXT_LEFT))
                            window->map->setViewCurrent(names[i]);
                    nk_combo_end(&ctx);
                }
            }
            
            bool viewChanged = false;
            vts::MapView view;
            window->map->getViewData("", view);
            { // surfaces
                const std::vector<std::string> surfaces
                        = window->map->getResourceSurfaces();
                for (const std::string &sn : surfaces)
                {
                    float ratio[] = { width };
                    nk_layout_row(&ctx, NK_STATIC, 16, 1, ratio);
                    bool v1 = view.surfaces.find(sn) != view.surfaces.end();
                    bool v2 = nk_check_label(&ctx, sn.c_str(), v1);
                    if (v2)
                    { // bound layers
                        vts::MapView::SurfaceInfo &s = view.surfaces[sn];
                        viewChanged = viewChanged
                                || prepareViewsBoundLayers(view, s.boundLayers);
                    }
                    else
                        view.surfaces.erase(sn);
                    if (v1 != v2)
                        viewChanged = true;
                }
            }
            { // free layers
                // todo
            }
            if (viewChanged)
                window->map->setViewData("", view);
        }
        nk_end(&ctx);
    }

    void prepareMarks()
    {
        int flags = NK_WINDOW_BORDER | NK_WINDOW_MOVABLE
                | NK_WINDOW_SCALABLE | NK_WINDOW_TITLE
                | NK_WINDOW_MINIMIZABLE;
        if (prepareFirst)
            flags |= NK_WINDOW_MINIMIZED;
        if (nk_begin(&ctx, "Marks", nk_rect(1050, 10, 250, 400), flags))
        {
            std::vector<Mark> &marks = window->marks;
            float width = nk_window_get_content_region_size(&ctx).x - 15;
            float ratio[] = { width * 0.6, width * 0.4 };
            nk_layout_row(&ctx, NK_STATIC, 16, 2, ratio);
            char buffer[256];
            Mark *prev = nullptr;
            int i = 0;
            double length = 0;
            for (Mark &m : marks)
            {
                sprintf(buffer, "%d", (i + 1));
                nk_checkbox_label(&ctx, buffer, &m.open);
                double l = prev
                        ? vts::length(vts::vec3(prev->coord - m.coord))
                        : 0;
                length += l;
                sprintf(buffer, "%.3f", l);
                nk_color c;
                c.r = 255 * m.color(0);
                c.g = 255 * m.color(1);
                c.b = 255 * m.color(2);
                c.a = 255;
                nk_label_colored(&ctx, buffer, NK_TEXT_RIGHT, c);
                if (m.open)
                {
                    vts::Point n;
                    vts::vecToPoint(m.coord, n);
                    n = window->map->convert(n, vts::Srs::Physical,
                                             (vts::Srs)positionSrs);
                    sprintf(buffer, "%.8f", n.x);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                    if (nk_button_label(&ctx, "Go"))
                    {
                        vts::vecToPoint(m.coord, n);
                        n = window->map->convert(n, vts::Srs::Physical,
                                                 vts::Srs::Navigation);
                        window->map->setPositionPoint(n);
                    }
                    sprintf(buffer, "%.8f", n.y);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                    nk_label(&ctx, "", NK_TEXT_RIGHT);
                    sprintf(buffer, "%.8f", n.z);
                    nk_label(&ctx, buffer, NK_TEXT_RIGHT);
                    if (nk_button_label(&ctx, "Remove"))
                    {
                        marks.erase(marks.begin() + i);
                        break;
                    }
                }
                prev = &m;
                i++;
            }
            nk_label(&ctx, "Total:", NK_TEXT_LEFT);
            sprintf(buffer, "%.3f", length);
            nk_label(&ctx, buffer, NK_TEXT_RIGHT);
            nk_label(&ctx, "", NK_TEXT_LEFT);
            if (nk_button_label(&ctx, "Clear all"))
                marks.clear();
        }
        nk_end(&ctx);
    }
    
    void prepare(int width, int height)
    {
        prepareOptions();
        prepareStatistics();
        preparePosition();
        prepareViews();
        prepareMarks();
        prepareFirst = false;
    }

    void render(int width, int height)
    {
        prepare(width, height);
        dispatch(width, height);
    }

    GuiSkinMedia skinMedia;
    nk_context ctx;
    nk_font_atlas atlas;
    nk_font *font;
    nk_buffer cmds;
    nk_convert_config config;
    nk_draw_null_texture null;
    bool consumeEvents;
    bool prepareFirst;

    int statTraversedDetails;
    int statRenderedDetails;
    int optSensitivityDetails;
    int positionSrs;
    int autoPan;

    MainWindow *window;
    std::shared_ptr<GpuTextureImpl> fontTexture;
    std::shared_ptr<GpuTextureImpl> skinTexture;
    std::shared_ptr<GpuShaderImpl> shader;
    std::shared_ptr<GpuMeshImpl> mesh;
    
    static const int MaxVertexMemory = 512 * 1024;
    static const int MaxElementMemory = 128 * 1024;
};

void MainWindow::Gui::mousePositionCallback(double xpos, double ypos)
{
    impl->mousePositionCallback(xpos, ypos);
}

void MainWindow::Gui::mouseButtonCallback(int button, int action, int mods)
{
    impl->mouseButtonCallback(button, action, mods);
}

void MainWindow::Gui::mouseScrollCallback(double xoffset, double yoffset)
{
    impl->mouseScrollCallback(xoffset, yoffset);
}

void MainWindow::Gui::keyboardCallback(int key, int scancode,
                                       int action, int mods)
{
    impl->keyboardCallback(key, scancode, action, mods);
}

void MainWindow::Gui::keyboardUnicodeCallback(unsigned int codepoint)
{
    impl->keyboardUnicodeCallback(codepoint);
}

void MainWindow::Gui::initialize(MainWindow *window)
{
    impl = std::make_shared<GuiImpl>(window);
}

void MainWindow::Gui::render(int width, int height)
{
    impl->render(width, height);
}

void MainWindow::Gui::input()
{
    impl->input();
}

void MainWindow::Gui::finalize()
{
    impl.reset();
}
