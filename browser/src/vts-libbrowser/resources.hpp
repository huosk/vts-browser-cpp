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

#ifndef RESOURCES_HPP_zujhgfw89e7rjk7
#define RESOURCES_HPP_zujhgfw89e7rjk7

#include <memory>
#include <unordered_map>
#include <vector>

#include "../include/vts-browser/buffer.hpp"

#include "../utilities/threadName.hpp"
#include "../utilities/threadQueue.hpp"
#include "../validity.hpp"

namespace vts
{

class MapImpl;
class Resource;
class Resources;
class Fetcher;
class Cache;
class AuthConfig;
class SearchTask;
class SearchTaskImpl;
class FetchTaskImpl;
class GeodataTile;
class GpuAtmosphereDensityTexture;

class CacheData
{
public:
    CacheData() = default;
    CacheData(CacheData &&) noexcept = default;
    CacheData(FetchTaskImpl *task, bool availFailed = false);
    CacheData &operator = (CacheData &&) noexcept = default;

    Buffer buffer;
    std::string name;
    sint64 expires = 0;
    bool availFailed = false;
};

class UploadData
{
public:
    UploadData() = default;
    explicit UploadData(const std::shared_ptr<Resource> &resource); // upload
    explicit UploadData(std::shared_ptr<void> &userData, int); // destroy
    UploadData(const UploadData &) = delete;
    UploadData(UploadData &&) = default;
    UploadData &operator = (const UploadData &) = delete;
    UploadData &operator = (UploadData &&) = default;

    void process();

protected:
    std::weak_ptr<Resource> uploadData;
    std::shared_ptr<void> destroyData;
};

template<class Item, void (Resources::*Process)(Item), float (Resources ::*Priority)(const Item &), int ThreadName>
class ResourceProcessor
{
public:
    void push(const Item &item)
    {
        {
            std::lock_guard<std::mutex> lock(mut);
            q.push_back(item);
        }
        con.notify_one();
    }

    void push(Item &&item)
    {
        {
            std::lock_guard<std::mutex> lock(mut);
            q.push_back(std::move(item));
        }
        con.notify_one();
    }

    bool runOne();
    void runAll();

    void terminate()
    {
        stop = true;
    }

    uint32 estimateSize() const
    {
        return q.size();
    }

    ResourceProcessor(Resources *resources) : resources(resources)
    {
        if (ThreadName)
            thr = std::thread(&ResourceProcessor::entry, this);
    }

    ~ResourceProcessor()
    {
        if (thr.joinable())
            thr.join();
    }

    // private:
    std::vector<Item> q;
    std::mutex mut;
    std::condition_variable con;
    std::thread thr;
    std::atomic<bool> stop{ false };
    Resources *resources = nullptr;

    static constexpr const char *ThreadNames[] =
    {
        "",
        "fetching",
        "cacheRead",
        "cacheWrite",
        "decode",
        "geodata",
        "atmosphere",
    };

    void entry()
    {
        setThreadName(ThreadNames[ThreadName]);
        runAll();
    }

    Item getBest();
};

class Resources : private Immovable
{
public:
    Resources(MapImpl *map);
    ~Resources();

    void purgeResourcesCache();

    void dataFinalize();
    void renderFinalize();
    void dataUpdate();
    void renderUpdate();
    void dataAllRun();

    // private:
    void decodeProcess(const std::shared_ptr<Resource> &r);
    void uploadProcess(const std::shared_ptr<Resource> &r);
    void cacheReadProcess(const std::shared_ptr<Resource> &r);

    void fetcherProcessorEntry();

    void removeOld();
    void checkInitialized();

    bool tryRemove(std::shared_ptr<Resource> &r);
    void saveCorruptedFile(const std::shared_ptr<Resource> &r);
    void terminateAllQueues();

    void cacheInit();
    void cacheWrite(const CacheData &data);
    CacheData cacheRead(const std::string &name);

    void oneCacheRead(std::weak_ptr<Resource> r);
    void oneFetch(std::weak_ptr<Resource> r);
    void oneDecode(std::weak_ptr<Resource> r);
    void oneGeodata(std::weak_ptr<GeodataTile> r);
    void oneAtmosphere(std::weak_ptr<GpuAtmosphereDensityTexture> r);
    void oneCacheWrite(CacheData r);
    void oneUpload(UploadData r);
    float priority(const std::weak_ptr<Resource> &r);
    float priority(const std::weak_ptr<GeodataTile> &r);
    float priority(const std::weak_ptr<GpuAtmosphereDensityTexture> &r);
    float priority(const CacheData &) { return 0; };
    float priority(const UploadData &) { return 0; };

    ResourceProcessor<std::weak_ptr<Resource>, &Resources::oneFetch, &Resources::priority, 0> queFetching;
    ResourceProcessor<std::weak_ptr<Resource>, &Resources::oneCacheRead, &Resources::priority, 2> queCacheRead;
    ResourceProcessor<CacheData, &Resources::oneCacheWrite, &Resources::priority, 3> queCacheWrite;
    ResourceProcessor<std::weak_ptr<Resource>, &Resources::oneDecode, &Resources::priority, 4> queDecode;
    ResourceProcessor<std::weak_ptr<GeodataTile>, &Resources::oneGeodata, &Resources::priority, 5> queGeodata;
    ResourceProcessor<std::weak_ptr<GpuAtmosphereDensityTexture>, &Resources::oneAtmosphere, &Resources::priority, 6> queAtmosphere;
    ResourceProcessor<UploadData, &Resources::oneUpload, &Resources::priority, 0> queUpload;

    std::unordered_map<std::string, std::shared_ptr<Resource>> resources;
    MapImpl *const map;
    std::atomic<uint32> downloads{ 0 }; // number of active downloads
};

template<class Item, void (Resources::*Process)(Item), float (Resources::*Priority)(const Item &), int ThreadName>
inline bool ResourceProcessor<Item, Process, Priority, ThreadName>::runOne()
{
    Item item;
    {
        std::unique_lock<std::mutex> lock(mut);
        if (q.empty() || stop)
            return false;
        item = getBest();
    }
    (resources->*Process)(std::move(item));
    return true;
}

template<class Item, void (Resources::*Process)(Item), float (Resources::*Priority)(const Item &), int ThreadName>
inline void ResourceProcessor<Item, Process, Priority, ThreadName>::runAll()
{
    while (!stop)
    {
        Item item;
        {
            std::unique_lock<std::mutex> lock(mut);
            while (q.empty() && !stop)
                con.wait(lock);
            if (stop)
                return;
            item = getBest();
        }
        (resources->*Process)(std::move(item));
    }
}

template<class Item, void (Resources::*Process)(Item), float (Resources::*Priority)(const Item &), int ThreadName>
inline Item ResourceProcessor<Item, Process, Priority, ThreadName>::getBest()
{
    auto it = q.begin();
    auto et = q.end();
    auto b = it;
    float p = (resources->*Priority)(*it++);
    while (it != et)
    {
        float r = (resources->*Priority)(*it);
        if (r > p)
        {
            b = it;
            p = r;
        }
        it++;
    }
    auto r = std::move(*b);
    q.erase(b);
    return r;
}

} // namespace vts

#endif
