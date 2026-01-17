/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "Common.h"
#include "UI.h"
#include "Charts.h"
#include "RenderContext.h"
#include "ShaderManager.h"

namespace ShaderBeam
{

class CaptureBase;

class Renderer
{
public:
    Renderer(const Options& options, UI& ui, Watcher& watcher, ShaderManager& shaderManager, RenderContext& renderContext);

    void Start();
    void Stop();
    void Render(bool present, bool ui = true);
    void Present(bool vsync);

    bool NewInputRequired() const;
    bool SupportsResync() const;
    void Skip(int numFrames);

    void SubmitInput(const winrt::com_ptr<ID3D11Texture2D>& input, int width, int height, int x, int y);
    void SubmitInput(void* data, unsigned size);

    void Benchmark(const std::shared_ptr<CaptureBase>& capture);

private:
    const Options& m_options;
    UI&            m_ui;
    Watcher&       m_watcher;
    Charts         m_charts;
    RenderContext& m_renderContext;
    ShaderManager& m_shaderManager;

    void RollInput(bool newFrame);
    void Destroy();
    int  GetNextSlot() const;
};
} // namespace ShaderBeam