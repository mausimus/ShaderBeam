#include "stdafx.h"

#include "CaptureImage.h"
#include "Renderer.h"

namespace ShaderBeam
{
CaptureImage::CaptureImage(Watcher& watcher, const Options& options, Renderer& renderer, RenderContext& renderContext) : CaptureBase(watcher, options, renderer, renderContext)
{
    m_name = "Image";
}

bool CaptureImage::IsSupported()
{
    return true;
}

bool CaptureImage::SupportsWindowCapture()
{
    return false;
}

void CaptureImage::InternalStart()
{
    m_data.resize(m_options.outputWidth * m_options.outputHeight * 4);
    for(auto& b : m_data)
    {
        b = 0xff;
    }
}

bool CaptureImage::InternalPoll()
{
    m_stagingCopyRequired = true;
    return true;
}

void CaptureImage::InternalStop()
{
    m_data.clear();
}

void CaptureImage::CopyStagingToOutput()
{
    m_renderer.SubmitInput(m_data.data(), (unsigned)m_data.size());
}
} // namespace ShaderBeam