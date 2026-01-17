#pragma once

#include "Helpers.h"

namespace ShaderBeam
{

class RenderContextD3D11 : public RenderContextBase
{
    friend class SinglePassShaderD3D11;

public:
    RenderContextD3D11(const Options& options);

    void Create(const AdapterInfo& adapter);
    void CreateInputs(int inputsRequired);
    void Destroy();
    void BeginFrame();
    void DrawSplitScreen();
    void BeginUI();
    void EndFrame();
    void Present(bool vsync);
    void BlackFrame() const;
    void Passthrough() const;
    void WaitTillIdle();

    // ImGui
    void ImGuiInit();
    void ImGuiNewFrame();
    void ImGuiRender();
    void ImGuiShutdown();

    void SubmitInput(int slotNo, void* data, unsigned size);

    // D3D-specific
    void SubmitInput(int slotNo, const winrt::com_ptr<ID3D11Texture2D>& input, int width, int height, int x, int y);

    winrt::com_ptr<ID3D11Device>        m_device;
    winrt::com_ptr<ID3D11DeviceContext> m_deviceContext;

private:
    std::vector<winrt::com_ptr<ID3D11Texture2D>>          m_inputTextures;
    std::vector<winrt::com_ptr<ID3D11ShaderResourceView>> m_inputTextureViews;
    winrt::com_ptr<ID3D11Texture2D>                       m_outputTexture;
    winrt::com_ptr<ID3D11RenderTargetView>                m_outputTargetView;

    // renderer
    winrt::com_ptr<IDXGISwapChain1>         m_swapChain { nullptr };
    winrt::com_ptr<ID3D11RasterizerState>   m_rasterizerState { nullptr };
    winrt::com_ptr<ID3D11DepthStencilState> m_depthStencilState { nullptr };
    winrt::com_ptr<ID3D11RenderTargetView>  m_uiTargetView { nullptr };
#if _DEBUG
    winrt::com_ptr<ID3D11Debug> m_debug { nullptr };
#endif
};
} // namespace ShaderBeam
