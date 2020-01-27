/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <vts-browser/resources.hpp>
#include <vts-browser/cameraDraws.hpp>
#include <vts-browser/celestial.hpp>
#include <vts-browser/camera.hpp>

#include <optick.h>

#include "renderer.hpp"

#define FXAA

namespace vts { namespace renderer
{

void clearGlState()
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(0);
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glPolygonOffset(0, 0);
    CHECK_GL("cleared gl state");
}

void enableClipDistance(bool enable)
{
#ifndef VTSR_NO_CLIP
    if (enable)
    {
        for (int i = 0; i < 4; i++)
            glEnable(GL_CLIP_DISTANCE0 + i);
    }
    else
    {
        for (int i = 0; i < 4; i++)
            glDisable(GL_CLIP_DISTANCE0 + i);
    }
#endif
}

UboCache::UboCache() : current(0), last(0), prev(0)
{
    // initialize the buffer
    data.reserve(1000);
    data.resize(10);
}

UniformBuffer *UboCache::get()
{
    if ((current + 1) % data.size() == prev)
    {
        // grow the buffer
        data.insert(data.begin() + prev, nullptr);
        prev++;
        if (last > current)
            last++;
    }
    auto &c = data[current];
    current = (current + 1) % data.size();
    if (!c)
        c = std::make_unique<UniformBuffer>();
    return &*c;
}

void UboCache::frame()
{
    prev = last;
    last = current;
}

RenderViewImpl::RenderViewImpl(
    Camera *camera, RenderView *api,
    RenderContextImpl *context) :
    camera(camera),
    api(api),
    context(context),
    draws(nullptr),
    body(nullptr),
    atmosphereDensityTexture(nullptr),
    lastUboViewPointer(nullptr),
    elapsedTime(0),
    width(0),
    height(0),
    antialiasingPrev(0),
    frameIndex(0),
    projected(false),
    lodBlendingWithDithering(false),
    frameRender2BufferId(0),
    colorRender2TexId(0)
{
    depthBuffer.meshQuad = context->meshQuad;
    depthBuffer.shaderCopyDepth = context->shaderCopyDepth;
}

UniformBuffer *RenderViewImpl::useDisposableUbo(uint32 bindIndex,
    void *data, uint32 size)
{
    UniformBuffer *ubo = size > 256
        ? uboCacheLarge.get() : uboCacheSmall.get();
    ubo->bind();
    ubo->load(data, size, GL_DYNAMIC_DRAW);
    ubo->bindToIndex(bindIndex);
    return ubo;
}

void RenderViewImpl::drawSurface(const DrawSurfaceTask &t)
{
    Texture *tex = (Texture*)t.texColor.get();
    Mesh *m = (Mesh*)t.mesh.get();
    if (!m || !tex)
        return;

    struct mat3x4f
    {
        vec4f data[3];
        mat3x4f(){}
        mat3x4f(const mat3f &m)
        {
            for (int y = 0; y < 3; y++)
                for (int x = 0; x < 3; x++)
                    data[y][x] = m(x, y);
        }
    };

    struct UboSurface
    {
        mat4f p;
        mat4f mv;
        mat3x4f uvMat; // + blendingCoverage
        vec4f uvClip;
        vec4f color;
        vec4si32 flags; // mask, monochromatic, flat shading, uv source, lodBlendingWithDithering, ... frameIndex
    } data;

    data.p = proj.cast<float>();
    data.mv = rawToMat4(t.mv);
    data.uvMat = mat3x4f(rawToMat3(t.uvm));
    data.uvClip = rawToVec4(t.uvClip);
    data.color = rawToVec4(t.color);
    sint32 flags = 0;
    if (t.texMask)
        flags |= 1 << 0;
    if (tex->getGrayscale())
        flags |= 1 << 1;
    if (t.flatShading)
        flags |= 1 << 2;
    if (t.externalUv)
        flags |= 1 << 3;
    if (!std::isnan(t.blendingCoverage))
    {
        if (lodBlendingWithDithering)
        {
            data.uvMat.data[0][3] = t.blendingCoverage;
            flags |= 1 << 4;
        }
        else
            data.color[3] *= t.blendingCoverage;
    }
    data.flags = vec4si32(flags, 0, 0, frameIndex);

    useDisposableUbo(1, data)->setDebugId("UboSurface");

    if (t.texMask)
    {
        glActiveTexture(GL_TEXTURE0 + 1);
        ((Texture*)t.texMask.get())->bind();
        glActiveTexture(GL_TEXTURE0 + 0);
    }
    tex->bind();

    m->bind();
    m->dispatch();
}

void RenderViewImpl::drawInfographic(const DrawSimpleTask &t)
{
    Mesh *m = (Mesh*)t.mesh.get();
    if (!m)
        return;

    struct UboInfographics
    {
        mat4f mvp;
        vec4f color;
        vec4f data;
        vec4f data2;
    } data;

    data.mvp = proj.cast<float>() * rawToMat4(t.mv);
    data.color = rawToVec4(t.color);

    data.data = vec4f(t.data[0], t.data[1], t.data[2], t.data[3]);
    data.data2 = vec4f(t.data2[0], t.data2[1], t.data2[2], t.data2[3]);

    if (!t.texColor)
    {
        data.data[0] = 0;
        data.data[1] = 0;
    }

    useDisposableUbo(1, data)->setDebugId("UboInfographics");

    if (t.texColor)
    {
        Texture *tex = (Texture*)t.texColor.get();
        tex->bind();
    }

    m->bind();
    m->dispatch();
}

void RenderViewImpl::updateFramebuffers()
{
    OPTICK_EVENT();

    #ifdef FXAA
        bool filtersUsed = (options.filterAA || options.filterDOF || options.filterSSAO || options.filterFX);

        uint32 antialiasingCur = filtersUsed ? 1 : options.antialiasingSamples;

        if (options.filterAA > 0)
            antialiasingCur = 1;

        if (options.width != width || options.height != height
            || antialiasingCur != antialiasingPrev || (filtersUsed && !colorRender2TexId) )
        {

            width = options.width;
            height = options.height;
            antialiasingPrev = std::max(std::min(antialiasingCur,
            maxAntialiasingSamples), 1u);

    #else 
        if (options.width != width || options.height != height
            || options.antialiasingSamples != antialiasingPrev)
        {
            width = options.width;
            height = options.height;
            antialiasingPrev = std::max(std::min(options.antialiasingSamples,
            maxAntialiasingSamples), 1u);

    #endif

        vars.textureTargetType
            = antialiasingPrev > 1 ? GL_TEXTURE_2D_MULTISAMPLE
            : GL_TEXTURE_2D;

#ifdef __EMSCRIPTEN__
        static const bool forceSeparateSampleTextures = true;
#else
        static const bool forceSeparateSampleTextures = false;
#endif

        // delete old textures
        glDeleteTextures(1, &vars.depthReadTexId);
        if (vars.depthRenderTexId != vars.depthReadTexId)
            glDeleteTextures(1, &vars.depthRenderTexId);
        glDeleteTextures(1, &vars.colorRenderTexId);
        if (vars.colorRenderTexId != vars.colorReadTexId)
            glDeleteTextures(1, &vars.colorRenderTexId);
        vars.depthReadTexId = vars.depthRenderTexId = 0;
        vars.colorReadTexId = vars.colorRenderTexId = 0;

        #ifdef FXAA
            glDeleteTextures(1, &colorRender2TexId);
            colorRender2TexId = 0;
        #endif

        // depth texture for rendering
        glActiveTexture(GL_TEXTURE0 + 5);
        glGenTextures(1, &vars.depthRenderTexId);
        glBindTexture(vars.textureTargetType, vars.depthRenderTexId);
        if (GLAD_GL_KHR_debug)
        {
            glObjectLabel(GL_TEXTURE, vars.depthRenderTexId,
                -1, "depthRenderTexId");
        }
        if (antialiasingPrev > 1)
        {
            glTexImage2DMultisample(vars.textureTargetType,
                antialiasingPrev, GL_DEPTH24_STENCIL8,
                options.width, options.height, GL_TRUE);
        }
        else
        {
            glTexImage2D(vars.textureTargetType, 0, GL_DEPTH24_STENCIL8,
                options.width, options.height,
                0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
            glTexParameteri(vars.textureTargetType,
                GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(vars.textureTargetType,
                GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
        CHECK_GL("update depth texture for rendering");

        // depth texture for sampling
        glActiveTexture(GL_TEXTURE0 + 6);
        if (antialiasingPrev > 1 || forceSeparateSampleTextures)
        {
            glGenTextures(1, &vars.depthReadTexId);
            glBindTexture(GL_TEXTURE_2D, vars.depthReadTexId);
            if (GLAD_GL_KHR_debug)
            {
                glObjectLabel(GL_TEXTURE, vars.depthReadTexId,
                    -1, "depthReadTexId");
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
                options.width, options.height,
                0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                GL_NEAREST);
        }
        else
        {
            vars.depthReadTexId = vars.depthRenderTexId;
            glBindTexture(GL_TEXTURE_2D, vars.depthReadTexId);
        }
        CHECK_GL("update depth texture for sampling");

        // color texture for rendering
        glActiveTexture(GL_TEXTURE0 + 7);
        glGenTextures(1, &vars.colorRenderTexId);
        glBindTexture(vars.textureTargetType, vars.colorRenderTexId);
        if (GLAD_GL_KHR_debug)
        {
            glObjectLabel(GL_TEXTURE, vars.colorRenderTexId,
                -1, "colorRenderTexId");
        }
        if (antialiasingPrev > 1)
        {
            glTexImage2DMultisample(
                vars.textureTargetType, antialiasingPrev, GL_RGB8,
                options.width, options.height, GL_TRUE);
        }
        else
        {
            glTexImage2D(vars.textureTargetType, 0, GL_RGB8,
                options.width, options.height,
                0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(vars.textureTargetType,
                GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(vars.textureTargetType,
                GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
        CHECK_GL("update color texture for rendering");

        // color texture for sampling
        glActiveTexture(GL_TEXTURE0 + 8);
        if (antialiasingPrev > 1 || forceSeparateSampleTextures)
        {
            glGenTextures(1, &vars.colorReadTexId);
            glBindTexture(GL_TEXTURE_2D, vars.colorReadTexId);
            if (GLAD_GL_KHR_debug)
            {
                glObjectLabel(GL_TEXTURE, vars.colorReadTexId,
                    -1, "colorReadTexId");
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                options.width, options.height,
                0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                GL_NEAREST);
        }
        else
        {
            vars.colorReadTexId = vars.colorRenderTexId;
            glBindTexture(GL_TEXTURE_2D, vars.colorReadTexId);
        }

        #ifdef FXAA
            // color texture for postprocessing
            if (filtersUsed)
            {
                glActiveTexture(GL_TEXTURE0 + 9);

                glGenTextures(1, &colorRender2TexId);
                glBindTexture(GL_TEXTURE_2D, colorRender2TexId);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8,
                    options.width, options.height,
                    0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
            }
        #endif

        if (GLAD_GL_KHR_debug)
            glObjectLabel(GL_TEXTURE, vars.colorReadTexId,
                -1, "colorReadTexId");

        CHECK_GL("update color texture for sampling");

        // render frame buffer
        glDeleteFramebuffers(1, &vars.frameRenderBufferId);
        glGenFramebuffers(1, &vars.frameRenderBufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
        if (GLAD_GL_KHR_debug)
        {
            glObjectLabel(GL_FRAMEBUFFER, vars.frameRenderBufferId,
                -1, "frameRenderBufferId");
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
            vars.textureTargetType, vars.depthRenderTexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            vars.textureTargetType, vars.colorRenderTexId, 0);
        checkGlFramebuffer(GL_FRAMEBUFFER);

        // sample frame buffer
        glDeleteFramebuffers(1, &vars.frameReadBufferId);
        glGenFramebuffers(1, &vars.frameReadBufferId);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameReadBufferId);
        if (GLAD_GL_KHR_debug)
        {
            glObjectLabel(GL_FRAMEBUFFER, vars.frameReadBufferId,
                -1, "frameReadBufferId");
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
            GL_TEXTURE_2D, vars.depthReadTexId, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, vars.colorReadTexId, 0);
        checkGlFramebuffer(GL_FRAMEBUFFER);

        glActiveTexture(GL_TEXTURE0);
        CHECK_GL("update frame buffer");

        #ifdef FXAA
            if (filtersUsed)
            {
                // render frame buffer2
                glDeleteFramebuffers(1, &frameRender2BufferId);
                glGenFramebuffers(1, &frameRender2BufferId);
                glBindFramebuffer(GL_FRAMEBUFFER, frameRender2BufferId);
                if (GLAD_GL_KHR_debug)
                    glObjectLabel(GL_FRAMEBUFFER, frameRender2BufferId,
                        -1, "frameRender2BufferId");
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    vars.textureTargetType, colorRender2TexId, 0);
                checkGlFramebuffer(GL_FRAMEBUFFER);
            }
        #endif
    }
}

void RenderViewImpl::renderValid()
{
    OPTICK_EVENT();

    viewInv = view.inverse();
    projInv = proj.inverse();
    viewProj = proj * view;
    viewProjInv = viewProj.inverse();

    // update atmosphere
    updateAtmosphereBuffer();

    // render opaque
    if (!draws->opaque.empty())
    {
        OPTICK_EVENT("opaque");
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        context->shaderSurface->bind();
        enableClipDistance(true);
        for (const DrawSurfaceTask &t : draws->opaque)
            drawSurface(t);
        enableClipDistance(false);
        CHECK_GL("rendered opaque");
    }

    // render background (atmosphere)
    if (options.renderAtmosphere)
    {
        OPTICK_EVENT("background");
        // corner directions
        vec3 camPos = rawToVec3(draws->camera.eye) / body->majorRadius;
        mat4 inv = (viewProj * scaleMatrix(body->majorRadius)).inverse();
        vec4 cornerDirsD[4] = {
            inv * vec4(-1, -1, 0, 1),
            inv * vec4(+1, -1, 0, 1),
            inv * vec4(-1, +1, 0, 1),
            inv * vec4(+1, +1, 0, 1)
        };
        vec3f cornerDirs[4];
        for (uint32 i = 0; i < 4; i++)
            cornerDirs[i] = normalize(vec3(vec4to3(cornerDirsD[i], true)
                - camPos)).cast<float>();

        context->shaderBackground->bind();
        for (uint32 i = 0; i < 4; i++)
            context->shaderBackground->uniformVec3(i, cornerDirs[i].data());
        context->meshQuad->bind();
        context->meshQuad->dispatch();
        CHECK_GL("rendered background");
    }

    // render transparent
    if (!draws->transparent.empty())
    {
        OPTICK_EVENT("transparent");
        glEnable(GL_BLEND);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0, -10);
        glDepthMask(GL_FALSE);
        context->shaderSurface->bind();
        enableClipDistance(true);
        for (const DrawSurfaceTask &t : draws->transparent)
            drawSurface(t);
        enableClipDistance(false);
        glDepthMask(GL_TRUE);
        glDisable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0, 0);
        CHECK_GL("rendered transparent");
    }

    // render polygon edges
    if (options.renderPolygonEdges)
    {
        OPTICK_EVENT("polygon_edges");
        glDisable(GL_BLEND);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#ifndef VTSR_OPENGLES
        glEnable(GL_POLYGON_OFFSET_LINE);
#endif
        glPolygonOffset(0, -1000);
        context->shaderSurface->bind();
        enableClipDistance(true);
        for (const DrawSurfaceTask &it : draws->opaque)
        {
            DrawSurfaceTask t(it);
            t.flatShading = false;
            t.color[0] = t.color[1] = t.color[2] = t.color[3] = 0;
            drawSurface(t);
        }
        enableClipDistance(false);
#ifndef VTSR_OPENGLES
        glDisable(GL_POLYGON_OFFSET_LINE);
#endif
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glPolygonOffset(0, 0);
        glEnable(GL_BLEND);
        CHECK_GL("rendered polygon edges");
    }

    // copy the depth (resolve multisampling)
    if (vars.depthReadTexId != vars.depthRenderTexId)
    {
        OPTICK_EVENT("copy_depth_resolve_multisampling");
        glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vars.frameReadBufferId);
        CHECK_GL_FRAMEBUFFER(GL_READ_FRAMEBUFFER);
        CHECK_GL_FRAMEBUFFER(GL_DRAW_FRAMEBUFFER);
        glBlitFramebuffer(0, 0, options.width, options.height,
            0, 0, options.width, options.height,
            GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
        CHECK_GL("copied the depth (resolved multisampling)");
    }

    // copy the depth for future use
    {
        OPTICK_EVENT("copy_depth_to_cpu");
        clearGlState();
        if ((frameIndex % 2) == 1)
        {
            uint32 dw = width;
            uint32 dh = height;
            if (!options.debugDepthFeedback)
                dw = dh = 0;
            depthBuffer.performCopy(vars.depthReadTexId, dw, dh, viewProj);
        }
        glViewport(0, 0, options.width, options.height);
        glScissor(0, 0, options.width, options.height);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        CHECK_GL("copy depth");
    }

    // render geodata
    renderGeodata();
    CHECK_GL("rendered geodata");

    // render infographics
    if (!draws->infographics.empty())
    {
        OPTICK_EVENT("infographics");
        glDisable(GL_DEPTH_TEST);
        context->shaderInfographics->bind();
        for (const DrawSimpleTask &t : draws->infographics)
            drawInfographic(t);
        CHECK_GL("rendered infographics");
    }

    //render colorRender to colorRender2
    #ifdef FXAA

        if (options.filterAA || options.filterDOF || options.filterSSAO || options.filterFX)
        {
            OPTICK_EVENT("postprocssing");

            switch (options.filterAA)
            {
            case 2: //fxaa
                applyFilter(context->shaderFXAA); break;
            case 3: //fxaa2
                applyFilter(context->shaderFXAA2); break;
            }

            switch (options.filterDOF)
            {
            case 1: //dof
            case 2: //dof2

                double cnear, cfar;
                double eye[3], target[3], up[3];

                camera->getView(eye, target, up);
                camera->suggestedNearFar(cnear, cfar);

                vec3 delta = rawToVec3(target) - rawToVec3(eye);
                double distance = sqrt(delta[0]*delta[0] + delta[1] * delta[1] + delta[2] * delta[2]);

                std::shared_ptr<Shader> shader;

                if (options.filterDOF == 1)
                    shader = context->shaderDOF;
                else 
                    shader = context->shaderDOF2;

                shader->bind();

                uint32 id = shader->getId();
                glUniform1f(glGetUniformLocation(id, "maxblur"), options.filterDOFBlur); // 10);

                   
                if (!options.filterDOFFocus)
                {
                    glUniform1f(glGetUniformLocation(id, "focus"), distance);
                    glUniform1f(glGetUniformLocation(id, "aperture"), 1 / (distance * powf(1.4, options.filterDOFAperture)));
                }
                else
                {
                    glUniform1f(glGetUniformLocation(id, "focus"), 50);
                    glUniform1f(glGetUniformLocation(id, "aperture"), 1 / ((distance + 200) * powf(1.4, options.filterDOFAperture)));
                }

                glUniform1f(glGetUniformLocation(id, "nearClip"), cnear);
                glUniform1f(glGetUniformLocation(id, "farClip"), cfar);
                glUniform1f(glGetUniformLocation(id, "aspect"), 1);

                if (options.filterDOF == 1)
                    applyFilter(shader);
                else {
                    glUniform1f(glGetUniformLocation(id, "vpass"), 1);
                    applyFilter(shader);
                    glUniform1f(glGetUniformLocation(id, "vpass"), 0);
                    applyFilter(shader);
                }

                break;
            }

            switch (options.filterFX)
            {
            case 1: //greyscale
                applyFilter(context->shaderGreyscale); break;
            case 2: //depth
                applyFilter(context->shaderDepth); break;
            }

            CHECK_GL("rendered postprocessing");
        }
    #endif
}

void RenderViewImpl::renderEntry()
{
    CHECK_GL("pre-frame check");
    uboCacheLarge.frame();
    uboCacheSmall.frame();
    clearGlState();
    frameIndex++;

    assert(context->shaderSurface);
    view = rawToMat4(draws->camera.view);
    proj = rawToMat4(draws->camera.proj);

    if (options.width <= 0 || options.height <= 0)
        return;

    updateFramebuffers();

    // initialize opengl
    glViewport(0, 0, options.width, options.height);
    glScissor(0, 0, options.width, options.height);

    glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);

    CHECK_GL_FRAMEBUFFER(GL_FRAMEBUFFER);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
        | GL_STENCIL_BUFFER_BIT);
    CHECK_GL("initialized opengl state");

    // render
    if (proj(0, 0) != 0)
        renderValid();
    else
        hysteresisJobs.clear();

    // copy the color to output texture
    if (options.colorToTexture
        && vars.colorReadTexId != vars.colorRenderTexId)
    {
        OPTICK_EVENT("colorToTexture");
        glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, vars.frameReadBufferId);
        CHECK_GL_FRAMEBUFFER(GL_READ_FRAMEBUFFER);
        CHECK_GL_FRAMEBUFFER(GL_DRAW_FRAMEBUFFER);
        glBlitFramebuffer(0, 0, options.width, options.height,
            0, 0, options.width, options.height,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
        CHECK_GL("copied the color to texture");
    }

    // copy the color to target frame buffer
    if (options.colorToTargetFrameBuffer)
    {
        OPTICK_EVENT("colorToTargetFrameBuffer");
        uint32 w = options.targetViewportW ? options.targetViewportW
            : options.width;
        uint32 h = options.targetViewportH ? options.targetViewportH
            : options.height;
        bool same = w == options.width && h == options.height;
        glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, options.targetFrameBuffer);
        CHECK_GL_FRAMEBUFFER(GL_READ_FRAMEBUFFER);
        CHECK_GL_FRAMEBUFFER(GL_DRAW_FRAMEBUFFER);
        glBlitFramebuffer(0, 0, options.width, options.height,
            options.targetViewportX, options.targetViewportY,
            options.targetViewportX + w, options.targetViewportY + h,
            GL_COLOR_BUFFER_BIT, same ? GL_NEAREST : GL_LINEAR);
        CHECK_GL("copied the color to target frame buffer");
    }

    // clear the state
    clearGlState();

    checkGlImpl("frame end (unconditional check)");
}

void RenderViewImpl::updateAtmosphereBuffer()
{
    OPTICK_EVENT();

    ShaderAtm::AtmBlock atmBlock;

    if (options.renderAtmosphere
        && !projected
        && atmosphereDensityTexture)
    {
        // bind atmosphere density texture
        {
            glActiveTexture(GL_TEXTURE4);
            atmosphereDensityTexture->bind();
            glActiveTexture(GL_TEXTURE0);
        }

        double boundaryThickness, horizontalExponent, verticalExponent;
        atmosphereDerivedAttributes(*body, boundaryThickness,
            horizontalExponent, verticalExponent);

        // uniParams
        atmBlock.uniAtmSizes =
        {
            (float)(boundaryThickness / body->majorRadius),
            (float)(body->majorRadius / body->minorRadius),
            (float)(1.0 / body->majorRadius),
            0.f
        };
        atmBlock.uniAtmCoefs =
        {
            (float)(horizontalExponent),
            (float)(body->atmosphere.colorGradientExponent),
            0.f,
            0.f
        };

        // camera position
        vec3 camPos = rawToVec3(draws->camera.eye) / body->majorRadius;
        atmBlock.uniAtmCameraPosition = camPos.cast<float>();

        // view inv
        mat4 vi = rawToMat4(draws->camera.view).inverse();
        atmBlock.uniAtmViewInv = vi.cast<float>();

        // colors
        atmBlock.uniAtmColorHorizon
            = rawToVec4(body->atmosphere.colorHorizon);
        atmBlock.uniAtmColorZenith
            = rawToVec4(body->atmosphere.colorZenith);
    }
    else
    {
        memset(&atmBlock, 0, sizeof(atmBlock));
    }

    useDisposableUbo(0, atmBlock)->setDebugId("uboAtm");
}

void RenderViewImpl::getWorldPosition(const double screenPos[2],
    double worldPos[3])
{
    vecToRaw(nan3(), worldPos);
    double x = screenPos[0];
    double y = screenPos[1];
    y = height - y - 1;
    x = x / width * 2 - 1;
    y = y / height * 2 - 1;
    double z = depthBuffer.value(x, y) * 2 - 1;
    vecToRaw(vec4to3(vec4(viewProjInv
        * vec4(x, y, z, 1)), true), worldPos);
}

void RenderViewImpl::applyFilter(const std::shared_ptr<Shader> &shader)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, vars.frameRenderBufferId);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameRender2BufferId);
    CHECK_GL_FRAMEBUFFER(GL_READ_FRAMEBUFFER);
    CHECK_GL_FRAMEBUFFER(GL_DRAW_FRAMEBUFFER);

    glBlitFramebuffer(0, 0, options.width, options.height,
        0, 0, options.width, options.height,
        GL_COLOR_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, vars.frameRenderBufferId);
    CHECK_GL_FRAMEBUFFER(GL_FRAMEBUFFER);

    shader->bind();

    glActiveTexture(GL_TEXTURE0 + 9);
    glBindTexture(GL_TEXTURE_2D, colorRender2TexId);

    context->meshQuad->bind();
    context->meshQuad->dispatch();

    glDepthMask(GL_TRUE);
    glDisable(GL_DEPTH_TEST);
}

void RenderViewImpl::renderCompass(const double screenPosSize[3],
    const double mapRotation[3])
{
    glEnable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    context->texCompas->bind();
    context->shaderTexture->bind();
    mat4 p = orthographicMatrix(-1, 1, -1, 1, -1, 1)
        * scaleMatrix(1.0 / width, 1.0 / height, 1);
    mat4 v = translationMatrix(screenPosSize[0] * 2 - width,
        screenPosSize[1] * 2 - height, 0)
        * scaleMatrix(screenPosSize[2], screenPosSize[2], 1);
    mat4 m = rotationMatrix(0, mapRotation[1] + 90)
        * rotationMatrix(2, mapRotation[0]);
    mat4f mvpf = (p * v * m).cast<float>();
    mat3f uvmf = identityMatrix3().cast<float>();
    context->shaderTexture->uniformMat4(0, mvpf.data());
    context->shaderTexture->uniformMat3(1, uvmf.data());
    context->meshQuad->bind();
    context->meshQuad->dispatch();
}

} } // namespace vts renderer

