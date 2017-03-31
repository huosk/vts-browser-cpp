#ifndef OPTIONS_H_kwegfdzvgsdfj
#define OPTIONS_H_kwegfdzvgsdfj

#include "foundation.h"

namespace melown
{

class MELOWN_API MapOptions
{
public:
    MapOptions();
    ~MapOptions();
    
    double maxTexelToPixelScale;
    uint32 maxResourcesMemory;
    uint32 maxConcurrentDownloads;
    bool renderWireBoxes;
    bool renderSurrogates;
};

} // namespace melown

#endif
