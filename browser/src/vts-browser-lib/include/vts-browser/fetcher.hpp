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

#ifndef FETCHER_H_wjehfduj
#define FETCHER_H_wjehfduj

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <ctime>

#include "foundation.hpp"
#include "buffer.hpp"

namespace vts
{

class VTS_API FetchTask
{
public:
    enum class ResourceType
    {
        General,
        MapConfig,
        AuthConfig,
        BoundLayerConfig,
        TilesetMappingConfig,
        BoundMetaTile,
        BoundMaskTile,
        MetaTile,
        Mesh,
        MeshPart,
        Texture,
        NavTile,
        Search,
        SriIndex,
    };
    static bool isResourceTypeMandatory(ResourceType resourceType);
    
    const ResourceType resourceType;
    
    // query
    std::string queryUrl;
    std::map<std::string, std::string> queryHeaders;

    // reply
    Buffer contentData;
    std::string contentType;
    std::string replyRedirectUrl;
    std::time_t replyExpires;
    uint32 replyCode;

    FetchTask(const std::string &url, ResourceType resourceType);
    virtual ~FetchTask();
    virtual void fetchDone() = 0;
};

class VTS_API FetcherOptions
{
public:
    FetcherOptions();

    uint32 threads;
    sint32 timeout;
    bool extraFileLog;

    // curl options
    uint32 maxHostConnections;
    uint32 maxTotalConections;
    uint32 maxCacheConections;
    sint32 pipelining; // 0, 1, 2 or 3
};

class VTS_API Fetcher
{
public:
    static std::shared_ptr<Fetcher> create(const FetcherOptions &options);

    virtual ~Fetcher();
    virtual void initialize() = 0;
    virtual void finalize() = 0;
    virtual void fetch(const std::shared_ptr<FetchTask> &) = 0;
};

} // namespace vts

#endif
