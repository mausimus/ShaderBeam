/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"
#include "Renderer.h"
#include "Helpers.h"
#include "CaptureBase.h"

namespace ShaderBeam
{

Renderer::Renderer(const Options& options, UI& ui, Watcher& watcher, ShaderManager& shaderManager, RenderContext& renderContext) :
    m_options(options), m_ui(ui), m_watcher(watcher), m_shaderManager(shaderManager), m_charts(m_options, m_watcher), m_renderContext(renderContext)
{ }

void Renderer::Start()
{
    m_renderContext.frameNo    = 0;
    m_renderContext.subFrameNo = 0;

    m_charts.Create(m_renderContext.m_device, m_renderContext.m_deviceContext);
    m_shaderManager.Create(m_renderContext, m_options.shaderProfileNo);

    auto inputsRequired = m_shaderManager.NumInputsRequired();
    if(inputsRequired > MAX_INPUTS)
        THROW(E_FAIL, "Too many inputs");
    m_renderContext.CreateInputs(inputsRequired);
}

void Renderer::Stop()
{
    m_shaderManager.Destroy();
    m_charts.Destroy();
    Destroy();
}

void Renderer::Render(bool present, bool ui)
{
    m_renderContext.BeginFrame();

    m_shaderManager.Render(m_renderContext);

    if(m_options.splitScreen)
    {
        m_renderContext.DrawSplitScreen();
    }

    //if(ui && options.charts)
    //m_charts.Render(m_renderContext.outputTexture, options.outputHeight);

    if(ui && m_ui.RenderRequired())
    {
        m_renderContext.BeginUI();
        m_ui.Render();
    }

    m_renderContext.EndFrame();

    m_renderContext.subFrameNo++;
    if(m_renderContext.subFrameNo == m_options.subFrames)
    {
        m_renderContext.frameNo++;
        m_renderContext.subFrameNo = 0;
    }

    if(present)
    {
        Present(true);
    }
    m_watcher.FrameSubmitted();
}

void Renderer::Present(bool vsync)
{
    m_renderContext.Present(vsync);
}

int Renderer::GetNextSlot() const
{
    // return either a free slot, or the oldest one to receive new input
    bool inputUsed[MAX_INPUTS];
    ZeroMemory(inputUsed, sizeof(inputUsed));
    for(auto slot : m_renderContext.inputSlots)
    {
        inputUsed[slot] = true;
    }
    for(auto t = 0; t < m_renderContext.inputSlots.size(); t++)
    {
        if(!inputUsed[t])
            return t;
    }
    return m_renderContext.inputSlots.back();
}

bool Renderer::NewInputRequired() const
{
    return m_shaderManager.NewInputRequired(m_renderContext);
}

bool Renderer::SupportsResync() const
{
    return m_shaderManager.SupportsResync(m_renderContext);
}

void Renderer::RollInput(bool newFrame)
{
    auto numInputs = m_renderContext.inputSlots.size();
    if(numInputs == 1)
        return;

    // if we didn't get a new frame, duplicate slot of the latest frame we have
    auto latestInput = newFrame ? GetNextSlot() : m_renderContext.inputSlots.front();

    // roll input slots and add latest to front
    for(auto i = numInputs - 1; i > 0; i--)
        m_renderContext.inputSlots[i] = m_renderContext.inputSlots[i - 1];
    m_renderContext.inputSlots[0] = latestInput;
}

void Renderer::Skip(int numFrames)
{
    m_renderContext.subFrameNo += numFrames;
    m_renderContext.subFrameNo %= m_options.subFrames;
}

void Renderer::SubmitInput(const winrt::com_ptr<ID3D11Texture2D>& input, int width, int height, int x, int y)
{
    m_renderContext.SubmitInput(GetNextSlot(), input, width, height, x, y);
    RollInput(true);
}

void Renderer::SubmitInput(void* data, unsigned size)
{
    m_renderContext.SubmitInput(GetNextSlot(), data, size);
    RollInput(true);
}

void Renderer::Benchmark(const std::shared_ptr<CaptureBase>& capture)
{
    const float benchmarkDuration = 4 * TICKS_PER_SEC;

    auto  start       = Helpers::GetTicks();
    float copyTime    = 0.0f;
    float renderTime  = 0.0f;
    float presentTime = 0.0f;
    float now         = start;
    float prev        = now;
    int   frames      = 0;
    do
    {
        auto pctComplete           = (now - start) / benchmarkDuration;
        m_renderContext.subFrameNo = ((int)(m_options.subFrames * pctComplete)) % m_options.subFrames;
        if(m_options.crossAdapter && (frames % m_options.subFrames == 0))
        {
            capture->BenchmarkCopy();
            m_renderContext.WaitTillIdle();
            auto now = Helpers::GetTicks();
            copyTime += now - prev;
            prev = now;
        }

        Render(false, false);
        m_renderContext.WaitTillIdle();
        now = Helpers::GetTicks();
        renderTime += now - prev;
        prev = now;

        Present(false);
        m_renderContext.WaitTillIdle();
        frames++;
        now = Helpers::GetTicks();
        presentTime += now - prev;
        prev = now;
    } while(now < start + benchmarkDuration);
    auto totalTime = now - start;
    m_ui.SetBenchmark({
        .totalFPS   = totalTime != 0 ? frames / (totalTime / TICKS_PER_SEC) : 0,
        .copyFPS    = copyTime != 0 ? frames / (copyTime / TICKS_PER_SEC) : 0,
        .renderFPS  = renderTime != 0 ? frames / (renderTime / TICKS_PER_SEC) : 0,
        .presentFPS = presentTime != 0 ? frames / (presentTime / TICKS_PER_SEC) : 0,
    });
}

void Renderer::Destroy()
{
    m_renderContext.frameNo    = 0;
    m_renderContext.subFrameNo = 0;
    m_renderContext.Destroy();
}

} // namespace ShaderBeam