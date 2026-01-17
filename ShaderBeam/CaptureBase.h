/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "Common.h"
#include "Watcher.h"

namespace ShaderBeam
{

class Renderer;

class CaptureBase
{
public:
    // device is the device to capture using
    // frame & context are in device used to process
    CaptureBase(Watcher& watcher, const Options& options, Renderer& renderer, RenderContext& renderContext);
    virtual void Start(const AdapterInfo& adapter);
    void         BenchmarkCopy();
    virtual void Stop();
    bool         Poll();

    virtual bool IsSupported()           = 0;
    virtual bool SupportsWindowCapture() = 0;

    const char* m_name;

protected:
    Watcher&       m_watcher;
    Renderer&      m_renderer;
    RenderContext& m_renderContext;
    const Options& m_options;
    volatile bool  m_stopping { false };

    bool m_stagingCopyRequired { false };
    int  m_windowX { 0 };
    int  m_windowY { 0 };

    virtual void InternalStart()       = 0;
    virtual bool InternalPoll()        = 0;
    virtual void InternalStop()        = 0;
    virtual void CopyStagingToOutput() = 0;
};

} // namespace ShaderBeam