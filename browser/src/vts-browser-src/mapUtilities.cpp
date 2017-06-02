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

#include "map.hpp"

namespace vts
{

using vtslibs::registry::View;
using vtslibs::registry::BoundLayer;

BoundParamInfo::BoundParamInfo(const View::BoundLayerParams &params)
    : View::BoundLayerParams(params), orig(0), vars(0),
      bound(nullptr), depth(0), watertight(true), transparent(false)
{}

const mat3f BoundParamInfo::uvMatrix() const
{
    int dep = depth;
    if (dep == 0)
        return upperLeftSubMatrix(identityMatrix()).cast<float>();
    double scale = 1.0 / (1 << dep);
    double tx = scale * (orig.localId.x
                         - ((orig.localId.x >> dep) << dep));
    double ty = scale * (orig.localId.y
                         - ((orig.localId.y >> dep) << dep));
    ty = 1 - scale - ty;
    mat3f m;
    m << scale, 0, tx,
            0, scale, ty,
            0, 0, 1;
    return m;
}

Validity BoundParamInfo::prepare(const NodeInfo &nodeInfo, MapImpl *impl,
                             uint32 subMeshIndex)
{    
    bound = impl->mapConfig->getBoundInfo(id);
    if (!bound)
        return Validity::Indeterminate;
    
    { // check lodRange and tileRange
        TileId t = nodeInfo.nodeId();
        int m = bound->lodRange.min;
        if (t.lod < m)
            return Validity::Invalid;
        t.x >>= t.lod - m;
        t.y >>= t.lod - m;
        if (t.x < bound->tileRange.ll[0] || t.x > bound->tileRange.ur[0])
            return Validity::Invalid;
        if (t.y < bound->tileRange.ll[1] || t.y > bound->tileRange.ur[1])
            return Validity::Invalid;
    }
    
    orig = vars = UrlTemplate::Vars(nodeInfo.nodeId(), local(nodeInfo),
                                    subMeshIndex);
    
    depth = std::max(nodeInfo.nodeId().lod - bound->lodRange.max, 0);
    
    if (depth > 0)
    {
        vars.tileId.lod -= depth;
        vars.tileId.x >>= depth;
        vars.tileId.y >>= depth;
        vars.localId.lod -= depth;
        vars.localId.x >>= depth;
        vars.localId.y >>= depth;
    }
    
    if (bound->metaUrl)
    { // bound meta node
        UrlTemplate::Vars v(vars);
        v.tileId.x &= ~255;
        v.tileId.y &= ~255;
        v.localId.x &= ~255;
        v.localId.y &= ~255;
        std::string boundName = bound->urlMeta(v);
        std::shared_ptr<BoundMetaTile> bmt = impl->getBoundMetaTile(boundName);
        switch (impl->getResourceValidity(boundName))
        {
        case Validity::Indeterminate:
            return Validity::Indeterminate;
        case Validity::Invalid:
            return Validity::Invalid;
        case Validity::Valid:
            break;
        }        
        uint8 f = bmt->flags[(vars.tileId.y & 255) * 256
                + (vars.tileId.x & 255)];
        if ((f & BoundLayer::MetaFlags::available)
                != BoundLayer::MetaFlags::available)
            return Validity::Invalid;
        watertight = (f & BoundLayer::MetaFlags::watertight)
                == BoundLayer::MetaFlags::watertight;
    }
    
    transparent = bound->isTransparent || (!!alpha && *alpha < 1);
    
    return Validity::Valid;
}

DrawTask::DrawTask() :
    mesh(nullptr), texColor(nullptr), texMask(nullptr),
    externalUv(false), transparent(false)
{}

DrawTask::DrawTask(RenderTask *r, MapImpl *m) :
    mesh(nullptr), texColor(nullptr), texMask(nullptr)
{
    mesh = r->mesh->info.userData;
    if (r->textureColor)
        texColor = r->textureColor->info.userData;
    if (r->textureMask)
        texMask = r->textureMask->info.userData;
    mat4f mvp = (m->renderer.viewProjRender * r->model).cast<float>();
    memcpy(this->mvp, mvp.data(), sizeof(mvp));
    memcpy(uvm, r->uvm.data(), sizeof(uvm));
    memcpy(color, r->color.data(), sizeof(color));
    externalUv = r->externalUv;
    transparent = r->transparent;
}

MapDraws::MapDraws()
{
    draws.reserve(2000);
}

RenderTask::RenderTask() : model(identityMatrix()),
    uvm(upperLeftSubMatrix(identityMatrix()).cast<float>()),
    color(1,1,1,1), externalUv(false), transparent(false)
{}

bool RenderTask::ready() const
{
    if (meshAgg && !*meshAgg)
        return false;
    if (!*mesh)
        return false;
    if (textureColor && !*textureColor)
        return false;
    if (textureMask && !*textureMask)
        return false;
    return true;
}

TraverseNode::TraverseNode(const NodeInfo &nodeInfo)
    : nodeInfo(nodeInfo), surface(nullptr), lastAccessTime(0),
      flags(0), texelSize(0),
      surrogateValue(vtslibs::vts::GeomExtents::invalidSurrogate),
      displaySize(0), validity(Validity::Indeterminate), empty(false)
{
    { // initialize corners to NAN
        vec3 n;
        for (uint32 i = 0; i < 3; i++)
            n[i] = std::numeric_limits<double>::quiet_NaN();
        for (uint32 i = 0; i < 8; i++)
            cornersPhys[i] = n;
        surrogatePhys = n;
    }
    { // initialize aabb to universe
        double di = std::numeric_limits<double>::infinity();
        vec3 vi(di, di, di);
        aabbPhys[0] = -vi;
        aabbPhys[1] = vi;
    }
}

TraverseNode::~TraverseNode()
{}

void TraverseNode::clear()
{
    draws.clear();
    childs.clear();
    credits.clear();
    surface = nullptr;
    empty = false;
    if (validity == Validity::Valid)
        validity = Validity::Indeterminate;
}

bool TraverseNode::ready() const
{
    for (auto &&it : draws)
        if (!it->ready())
            return false;
    return true;
}

const std::string MapImpl::convertNameToCache(const std::string &path)
{
    auto p = path.find("://");
    std::string a = p == std::string::npos ? path : path.substr(p + 3);
    std::string b = boost::filesystem::path(a).parent_path().string();
    std::string c = a.substr(b.length() + 1);
    if (b.empty() || c.empty())
        LOGTHROW(err2, std::runtime_error)
                << "Cannot convert path '" << path
                << "' into a cache path";
    return resources.cachePath
            + convertNameToPath(b, false) + "/"
            + convertNameToPath(c, false);
}

const std::string MapImpl::convertNameToPath(std::string path,
                                           bool preserveSlashes)
{
    path = boost::filesystem::path(path).normalize().string();
    std::string res;
    res.reserve(path.size());
    for (char it : path)
    {
        if ((it >= 'a' && it <= 'z')
         || (it >= 'A' && it <= 'Z')
         || (it >= '0' && it <= '9')
         || (it == '-' || it == '.'))
            res += it;
        else if (preserveSlashes && (it == '/' || it == '\\'))
            res += '/';
        else
            res += '_';
    }
    return res;
}


} // namespace vts

