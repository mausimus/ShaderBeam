#pragma once

#include "CaptureBase.h"

namespace ShaderBeam
{
class CaptureImage : public CaptureBase
{
public:
    CaptureImage(Watcher& watcher, const Options& options, Renderer& renderer, RenderContext& renderContext);

    // Inherited via CaptureBase
    bool IsSupported() override;
    bool SupportsWindowCapture() override;
    void InternalStart() override;
    bool InternalPoll() override;
    void InternalStop() override;
    void CopyStagingToOutput() override;

private:
    std::vector<uint8_t> m_data;
};
} // namespace ShaderBeam