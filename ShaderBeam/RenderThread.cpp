/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "RenderThread.h"
#include "Helpers.h"

namespace ShaderBeam
{

static DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
    ((RenderThread*)lpParameter)->Run();
    return 0;
}

RenderThread::RenderThread(const Options& options, const UI& ui, Renderer& renderer) : m_options(options), m_ui(ui), m_renderer(renderer), m_handle(NULL) { }

void RenderThread::Start(const std::shared_ptr<CaptureBase>& capture)
{
    m_capture    = capture;
    m_nextResync = 0;
    m_stop       = false;
    m_stopped    = false;
    m_handle     = CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
    if(m_handle == NULL)
    {
        throw std::runtime_error("Unable to create render thread.");
    }
    SetThreadPriority(m_handle, THREAD_PRIORITY_TIME_CRITICAL);
}

void RenderThread::Stop()
{
    m_stop = true;
    if(m_handle)
    {
        while(!m_stopped)
        {
            Sleep(5);
        }
    }
}

void RenderThread::PollCapture()
{
#ifdef RGB_TEST
    m_renderer.RollInput(true);
#else
    bool newFrame = m_capture->Poll(m_renderer.GetNextInput());
    m_renderer.RollInput(newFrame);
    if(newFrame && m_options.autoSync && m_renderer.SupportsResync())
    {
        if(m_nextResync-- == 0)
        {
            m_nextResync = (int)(AUTOSYNC_INTERVAL * m_options.vsyncRate / m_options.subFrames);
            if(m_ui.m_captureLag > m_options.vsyncDuration && m_ui.m_inputFPS < m_ui.m_outputFPS * 0.75f)
            {
                // we receive frames older than one display frame; skip output frames until we're in sync
                auto laggedFrames = (int)floorf(m_ui.m_captureLag / m_options.vsyncDuration);
                m_renderer.Skip(m_options.subFrames - (laggedFrames % m_options.subFrames));
            }
        }
    }
#endif
}

void RenderThread::Run()
{
    while(!m_stop && !m_stopped)
    {
        try
        {
            if(m_benchmark)
            {
                m_benchmark = false;
                m_renderer.Benchmark();
            }
            if(m_renderer.NewInputRequired())
            {
                PollCapture();
            }
            m_renderer.Render(true); // waits till vsync
        }
        catch(...)
        { }
    }
    m_capture.reset();
    m_stopped = true;
    m_handle  = NULL;
}

void RenderThread::Benchmark()
{
    m_benchmark = true;
}

} // namespace ShaderBeam