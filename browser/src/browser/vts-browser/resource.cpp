#include "map.hpp"

namespace vts
{

Resource::Resource(const std::string &name)
{
    impl = std::make_shared<ResourceImpl>(name);
}

Resource::~Resource()
{}

void Resource::setMemoryUsage(uint32 ram, uint32 gpu)
{
    impl->ramMemoryCost = ram;
    impl->gpuMemoryCost = gpu;
}

Resource::operator bool() const
{
    return impl->state == ResourceImpl::State::ready;
}

bool ResourceImpl::performAvailTest() const
{
    if (!availTest)
        return true;
    switch (availTest->type)
    {
    case vtslibs::registry::BoundLayer::Availability::Type::negativeCode:
        if (availTest->codes.find(code) == availTest->codes.end())
            return false;
        break;
    case vtslibs::registry::BoundLayer::Availability::Type::negativeType:
        if (availTest->mime == contentType)
            return false;
        break;
    case vtslibs::registry::BoundLayer::Availability::Type::negativeSize:
        if (contentData.size() <= (unsigned)availTest->size)
            return false;
        break;
    default:
        LOGTHROW(fatal, std::invalid_argument) << "Invalid available test type";
    }
    return true;
}

ResourceImpl::ResourceImpl(const std::string &name)
    : FetchTask(name), state(State::initializing), priority(0),
      lastAccessTick(0), ramMemoryCost(0), gpuMemoryCost(0)
{}

ResourceImpl::~ResourceImpl()
{}

} // namespace vts
