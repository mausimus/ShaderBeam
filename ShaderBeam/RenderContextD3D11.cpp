#include "stdafx.h"

#include "RenderContext.h"

#include "backends\imgui_impl_dx11.h"

namespace ShaderBeam
{

RenderContextD3D11::RenderContextD3D11(const Options& options) : RenderContextBase(options) { }

void RenderContextD3D11::Create(const AdapterInfo& adapter)
{
    m_device = Helpers::CreateD3DDevice(adapter.adapter.get());
    m_device->GetImmediateContext(m_deviceContext.put());

    auto dxgiDevice = m_device.as<IDXGIDevice1>();
    dxgiDevice->SetMaximumFrameLatency(options.maxQueuedFrames);
    if(options.gpuThreadPriority)
        dxgiDevice->SetGPUThreadPriority(options.gpuThreadPriority | 0x40000000);

#if _DEBUG
    m_debug = m_device.as<ID3D11Debug>();
#endif

    winrt::com_ptr<IDXGIAdapter> dxgiAdapter;
    THROW(dxgiDevice->GetAdapter(dxgiAdapter.put()), "Unable to get DXGI adapter");
    winrt::com_ptr<IDXGIFactory2> dxgiFactory;
    THROW(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)dxgiFactory.put()), "Unable to get DXGI factory");

    DXGI_SWAP_CHAIN_DESC1 d3d11SwapChainDesc = {};
    d3d11SwapChainDesc.Width                 = 0;
    d3d11SwapChainDesc.Height                = 0;
    d3d11SwapChainDesc.Format                = options.format;
    d3d11SwapChainDesc.SampleDesc.Count      = 1;
    d3d11SwapChainDesc.SampleDesc.Quality    = 0;
    d3d11SwapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    d3d11SwapChainDesc.BufferCount           = options.swapChainBuffers;
    d3d11SwapChainDesc.Scaling               = DXGI_SCALING_NONE;
    d3d11SwapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    d3d11SwapChainDesc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;
    d3d11SwapChainDesc.Flags                 = 0;
    // fun fact: on Intel Arc setting DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
    // makes Present(1) not wait on vsync any more and BSODs Windows

    THROW(dxgiFactory->CreateSwapChainForHwnd(m_device.get(), options.outputWindow, &d3d11SwapChainDesc, 0, 0, m_swapChain.put()), "Unable to create SwapChain");

    /*
    *  works if using Output for another display, but breaks WGC capture if a window is dragged onto it for example
    */
    if(options.exclusive)
    {
        winrt::com_ptr<IDXGIOutput> output;
        THROW(dxgiAdapter->EnumOutputs(options.shaderDisplayNo, output.put()), "Unable to get output");

        DXGI_OUTPUT_DESC outputDesc;
        THROW(output->GetDesc(&outputDesc), "Unable to get output desc");

        THROW(m_swapChain->SetFullscreenState(TRUE, output.get()), "Unable to set fullscreen");

        THROW(m_swapChain->ResizeBuffers(d3d11SwapChainDesc.BufferCount, options.outputWidth, options.outputHeight, options.format, 0), "Unable to resize buffers");
    }

    if(options.useHdr)
    {
        winrt::com_ptr<IDXGISwapChain3> swapChain3;
        THROW(m_swapChain->QueryInterface(__uuidof(IDXGISwapChain3), reinterpret_cast<void**>(swapChain3.put())), "Unable to get SwapChain3");
        THROW(swapChain3->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709), "Unable to set HDR ColorSpace");
    }

    THROW(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)m_outputTexture.put()), "Unable to get display buffer");

    D3D11_RENDER_TARGET_VIEW_DESC rtv {};
    rtv.Format        = options.hardwareSrgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : options.format;
    rtv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    THROW(m_device->CreateRenderTargetView(m_outputTexture.get(), &rtv, m_outputTargetView.put()), "Unable to create render target");

    D3D11_RENDER_TARGET_VIEW_DESC uitv {};
    uitv.Format        = options.format;
    uitv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    THROW(m_device->CreateRenderTargetView(m_outputTexture.get(), &uitv, m_uiTargetView.put()), "Unable to create render target");

    D3D11_RASTERIZER_DESC rsDesc {};
    rsDesc.CullMode          = D3D11_CULL_NONE;
    rsDesc.FillMode          = D3D11_FILL_SOLID;
    rsDesc.DepthClipEnable   = FALSE;
    rsDesc.MultisampleEnable = FALSE;
    rsDesc.ScissorEnable     = TRUE;
    THROW(m_device->CreateRasterizerState(&rsDesc, m_rasterizerState.put()), "Unable to create rasterizer state");

    D3D11_DEPTH_STENCIL_DESC dsDesc {};
    dsDesc.DepthEnable   = false;
    dsDesc.StencilEnable = false;
    THROW(m_device->CreateDepthStencilState(&dsDesc, m_depthStencilState.put()), "Unable to create depth/stencil state");

    D3D11_RECT scissor {};
    scissor.left   = 0;
    scissor.top    = 0;
    scissor.right  = options.splitScreen == 1 ? options.outputWidth / 2 : options.outputWidth;
    scissor.bottom = options.splitScreen == 2 ? options.outputHeight / 2 : options.outputHeight;

    m_deviceContext->RSSetState(m_rasterizerState.get());
    m_deviceContext->OMSetDepthStencilState(m_depthStencilState.get(), 0);
    m_deviceContext->OMSetBlendState(NULL, NULL, 1);
    m_deviceContext->RSSetScissorRects(1, &scissor);
}

void RenderContextD3D11::CreateInputs(int inputsRequired)
{
    D3D11_TEXTURE2D_DESC desc {};
    desc.Width              = options.outputWidth;
    desc.Height             = options.outputHeight;
    desc.ArraySize          = 1;
    desc.Format             = options.hardwareSrgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : options.format;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.MipLevels          = 1;
    desc.MiscFlags          = 0;
    desc.CPUAccessFlags     = options.crossAdapter ? D3D11_CPU_ACCESS_WRITE : 0;
    desc.Usage              = options.crossAdapter ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA data {};
    auto                   formatWidth = options.useHdr ? 2 : 1;
    std::vector<uint32_t>  initialData;
    initialData.resize(options.outputWidth * options.outputHeight * formatWidth);
    data.pSysMem          = initialData.data();
    data.SysMemPitch      = options.outputWidth * sizeof(uint32_t) * formatWidth;
    data.SysMemSlicePitch = options.outputWidth * options.outputHeight * sizeof(uint32_t) * formatWidth;
    for(int i = 0; i < inputsRequired; i++)
    {
#ifdef RGB_TEST
        uint32_t color;
        switch(i)
        {
        case 2:
            color = 0xff0000ff;
            break;
        case 1:
            color = 0xff00ff00;
            break;
        case 0:
            color = 0xffff0000;
            break;
        }
        for(auto& d : initialData)
            d = color;
#endif
        inputSlots.push_back(i);
        auto& inputTexture     = m_inputTextures.emplace_back(nullptr);
        auto& inputTextureView = m_inputTextureViews.emplace_back(nullptr);
        THROW(m_device->CreateTexture2D(&desc, &data, inputTexture.put()), "Unable to create texture");
        THROW(m_device->CreateShaderResourceView(inputTexture.get(), nullptr, inputTextureView.put()), "Unable to create input view");
    }
}

void RenderContextD3D11::Destroy()
{
    m_depthStencilState = nullptr;
    m_rasterizerState   = nullptr;
    m_swapChain         = nullptr;
    m_uiTargetView      = nullptr;
    m_outputTargetView  = nullptr;
    m_outputTexture     = nullptr;

    for(int i = 0; i < m_inputTextureViews.size(); i++)
        m_inputTextureViews[i] = nullptr;
    m_inputTextureViews.clear();
    for(int i = 0; i < m_inputTextures.size(); i++)
        m_inputTextures[i] = nullptr;
    m_inputTextures.clear();
    inputSlots.clear();

    if(m_deviceContext)
    {
        m_deviceContext->RSSetState(NULL); // fix exception in nvapi...
        m_deviceContext->ClearState();
        m_deviceContext->Flush();
        m_deviceContext = nullptr;
    }
#if _DEBUG
    m_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
    m_debug = nullptr;
#endif
    m_device = nullptr;
}

void RenderContextD3D11::BeginFrame()
{
    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)options.outputWidth, (float)options.outputHeight };
    m_deviceContext->RSSetViewports(1, &viewport);

    ID3D11RenderTargetView* targets[1] = { m_outputTargetView.get() };
    m_deviceContext->OMSetRenderTargets(1, targets, NULL);
}

void RenderContextD3D11::DrawSplitScreen()
{
    D3D11_BOX box {};
    UINT      destX = options.splitScreen == 1 ? options.outputWidth / 2 : 0;
    UINT      destY = options.splitScreen == 2 ? options.outputHeight / 2 : 0;
    box.left        = destX;
    box.top         = destY;
    box.right       = options.outputWidth;
    box.bottom      = options.outputHeight;
    box.front       = 0;
    box.back        = 1;
    m_deviceContext->CopySubresourceRegion(m_outputTexture.get(), 0, destX, destY, 0, m_inputTextures[inputSlots[0]].get(), 0, &box);
}

void RenderContextD3D11::BeginUI()
{
    ID3D11RenderTargetView* targets[1] = { m_uiTargetView.get() };
    m_deviceContext->OMSetRenderTargets(1, targets, NULL);
}

void RenderContextD3D11::EndFrame()
{
    ID3D11RenderTargetView* null[] = { nullptr };
    m_deviceContext->OMSetRenderTargets(1, null, NULL);
}

void RenderContextD3D11::Present(bool vsync)
{
    DXGI_PRESENT_PARAMETERS pp {};
    m_swapChain->Present1(vsync ? 1 : 0, 0, &pp);
}

void RenderContextD3D11::BlackFrame() const
{
    static const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_deviceContext->ClearRenderTargetView(m_outputTargetView.get(), black);
}

void RenderContextD3D11::Passthrough() const
{
    m_deviceContext->CopyResource(m_outputTexture.get(), m_inputTextures[inputSlots[0]].get());
}

void RenderContextD3D11::WaitTillIdle()
{
    m_deviceContext->Flush();

    D3D11_QUERY_DESC            queryDesc = { D3D11_QUERY_EVENT, 0 };
    winrt::com_ptr<ID3D11Query> query;
    BOOL                        done = FALSE;

    if(m_device->CreateQuery(&queryDesc, query.put()) == S_OK)
    {
        m_deviceContext->End(query.get());
        while(!done)
        {
            switch(m_deviceContext->GetData(query.get(), &done, sizeof(BOOL), 0))
            {
            case S_OK:
                break;
            case S_FALSE:
                break;
            default:
                // error
                return;
            }
        }
    }
}

void RenderContextD3D11::SubmitInput(int slotNo, const winrt::com_ptr<ID3D11Texture2D>& input, int width, int height, int x, int y)
{
    const winrt::com_ptr<ID3D11Texture2D>& targetTexture = m_inputTextures.at(slotNo);

    if(width == options.outputWidth && height == options.outputHeight && x == 0 && y == 0)
    {
        m_deviceContext->CopyResource(targetTexture.get(), input.get());
    }
    else
    {
        // truncate if monitor sizes differ
        D3D11_BOX box;
        box.left   = 0;
        box.top    = 0;
        box.right  = min(options.outputWidth - x, (unsigned)width);
        box.bottom = min(options.outputHeight - y, (unsigned)height);
        box.front  = 0;
        box.back   = 1;
        m_deviceContext->CopySubresourceRegion(targetTexture.get(), 0, x, y, 0, input.get(), 0, &box);
    }
}

void RenderContextD3D11::SubmitInput(int slotNo, void* data, unsigned size)
{
    const winrt::com_ptr<ID3D11Texture2D>& targetTexture = m_inputTextures.at(slotNo);

    D3D11_MAPPED_SUBRESOURCE targetResource;
    THROW(m_deviceContext->Map(targetTexture.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &targetResource), "Unable to map target texture");

    if(size == targetResource.DepthPitch)
    {
        // copy between mapped textures
        memcpy(targetResource.pData, data, size);
    }
    m_deviceContext->Unmap(targetTexture.get(), 0);
}

void RenderContextD3D11::ImGuiInit()
{
    ImGui_ImplDX11_Init(m_device.get(), m_deviceContext.get());
}

void RenderContextD3D11::ImGuiNewFrame()
{
    ImGui_ImplDX11_NewFrame();
}

void RenderContextD3D11::ImGuiRender()
{
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void RenderContextD3D11::ImGuiShutdown()
{
    ImGui_ImplDX11_Shutdown();
}

} // namespace ShaderBeam