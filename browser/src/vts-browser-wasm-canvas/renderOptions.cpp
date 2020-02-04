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

#include <vts-renderer/renderer.hpp>
#include "vts-libbrowser/utilities/json.hpp"

using namespace vts;

void applyRenderOptions(const std::string &json,
        vts::renderer::RenderOptions &opt)
{
    struct T : public vtsCRenderOptionsBase
    {
        void apply(const std::string &json)
        {
            Json::Value v = stringToJson(json);
            AJ(textScale, asFloat);
            AJ(antialiasingSamples, asUInt);
            AJ(renderGeodataDebug, asUInt);
            AJ(renderAtmosphere, asBool);
            AJ(geodataHysteresis, asBool);
            AJ(debugDepthFeedback, asBool);
        }
    };
    T t = (T&)opt;
    t.apply(json);
    opt = (vts::renderer::RenderOptions&)t;
}

std::string getRenderOptions(const vts::renderer::RenderOptions &opt)
{
    struct T : public vtsCRenderOptionsBase
    {
        std::string get() const
        {
            Json::Value v;
            TJ(textScale, asFloat);
            TJ(antialiasingSamples, asUInt);
            TJ(renderGeodataDebug, asUInt);
            TJ(renderAtmosphere, asBool);
            TJ(geodataHysteresis, asBool);
            TJ(debugDepthFeedback, asBool);
            return jsonToString(v);
        }
    };
    const T t = (const T&)opt;
    return t.get();
}
