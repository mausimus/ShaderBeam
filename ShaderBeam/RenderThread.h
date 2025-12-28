/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "Renderer.h"
#include "CaptureBase.h"

namespace ShaderBeam
{

class RenderThread
{
public:
    RenderThread(const Options& options, Renderer& renderer);

    void Start(const std::shared_ptr<CaptureBase>& capture);

    void Run();

    void Benchmark();

    void Stop();

private:
    HANDLE                       m_handle;
    const Options&               m_options;
    Renderer&                    m_renderer;
    std::shared_ptr<CaptureBase> m_capture;

    void PollCapture();

    volatile bool m_stop { false };
    volatile bool m_stopped { false };
    volatile bool m_benchmark { false };
};
} // namespace ShaderBeam
