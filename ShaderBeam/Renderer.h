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

class Renderer
{
public:
    Renderer(const Options& options, UI& ui, Watcher& watcher, ShaderManager& shaderManager);

    void Start(winrt::com_ptr<ID3D11Device> device, winrt::com_ptr<ID3D11DeviceContext> context);
    void Stop();
    void Render(bool present);
    void Present(bool vsync);

    const winrt::com_ptr<ID3D11Texture2D>& GetNextInput() const;
    bool                                   NewInputRequired() const;
    void                                   RollInput(bool newFrame);
    void                                   Skip(int numFrames);

    void Benchmark();

private:
    const Options& m_options;
    UI&            m_ui;
    Watcher&       m_watcher;
    Charts         m_charts;
    RenderContext  m_renderContext;
    ShaderManager& m_shaderManager;

    void Create();
    void CreateInputs();
    void Destroy();
    void DestroyInputs();
    int  GetNextSlot() const;

    winrt::com_ptr<IDXGISwapChain1>         m_swapChain { nullptr };
    winrt::com_ptr<ID3D11RasterizerState>   m_rasterizerState { nullptr };
    winrt::com_ptr<ID3D11DepthStencilState> m_depthStencilState { nullptr };
    winrt::com_ptr<ID3D11RenderTargetView>  m_uiTargetView { nullptr };
#if _DEBUG
    winrt::com_ptr<ID3D11Debug> m_debug { nullptr };
#endif
};
} // namespace ShaderBeam