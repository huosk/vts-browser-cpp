#ifndef RESOURCE_H_seghioqnh
#define RESOURCE_H_seghioqnh

#include <string>
#include <atomic>

#include <vts-libs/registry/referenceframe.hpp>
#include <boost/optional.hpp>

#include <vts/resources.hpp>
#include <vts/fetcher.hpp>

namespace vts
{

class ResourceImpl : public FetchTask
{
public:
    enum class State
    {
        initializing,
        downloading,
        downloaded,
        ready,
        error,
        finalizing,
    };
    
    ResourceImpl(const std::string &name);
    virtual ~ResourceImpl();

    bool performAvailTest() const;
    
    boost::optional<vtslibs::registry::BoundLayer::Availability> availTest;
    double priority;
    std::atomic<State> state;
    uint32 lastAccessTick;
    uint32 ramMemoryCost;
    uint32 gpuMemoryCost;
};

} // namespace vts

#endif
