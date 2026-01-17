/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "CaptureBase.h"
#include "Renderer.h"

namespace ShaderBeam
{

CaptureBase::CaptureBase(Watcher& watcher, const Options& options, Renderer& renderer, RenderContext& renderContext) :
    m_watcher(watcher), m_renderer(renderer), m_renderContext(renderContext), m_options(options), m_name()
{ }

void CaptureBase::BenchmarkCopy()
{
    if(m_options.crossAdapter)
        CopyStagingToOutput();
}

void CaptureBase::Start(const AdapterInfo& adapter)
{
    m_stopping            = false;
    m_stagingCopyRequired = false;

    InternalStart();
}

void CaptureBase::Stop()
{
    m_stopping            = true;
    m_stagingCopyRequired = false;

    InternalStop();
}

bool CaptureBase::Poll()
{
    if(m_stopping)
        return false;

    bool newFrame = InternalPoll();

    // we do this separately so that capture API can get its resource back asap
    if(m_stagingCopyRequired)
    {
        m_stagingCopyRequired = false;
        CopyStagingToOutput();
    }

    return newFrame;
}

} // namespace ShaderBeam