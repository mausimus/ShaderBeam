/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"
#include "Renderer.h"
#include "Helpers.h"

namespace ShaderBeam
{

Renderer::Renderer(const Options& options, UI& ui, Watcher& watcher, ShaderManager& shaderManager) :
    m_options(options), m_ui(ui), m_watcher(watcher), m_shaderManager(shaderManager), m_charts(m_options, m_watcher), m_renderContext(m_options)
{ }

void Renderer::Start(winrt::com_ptr<ID3D11Device> device, winrt::com_ptr<ID3D11DeviceContext> context)
{
    m_renderContext.device        = device;
    m_renderContext.deviceContext = context;

    Create();
    m_charts.Create(m_renderContext.device, m_renderContext.deviceContext);
    m_shaderManager.Create(m_renderContext, m_options.shaderProfileNo);
    CreateInputs();
}

void Renderer::Stop()
{
    m_shaderManager.Destroy();
    m_charts.Destroy();
    Destroy();
}

void Renderer::Render(bool present)
{
    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, (float)m_options.outputWidth, (float)m_options.outputHeight };
    m_renderContext.deviceContext->RSSetViewports(1, &viewport);

    ID3D11RenderTargetView* targets[1] = { m_renderContext.outputTargetView.get() };
    m_renderContext.deviceContext->OMSetRenderTargets(1, targets, NULL);

    m_shaderManager.Render(m_renderContext);

    if(m_options.splitScreen)
    {
        D3D11_BOX box {};
        UINT      destX = m_options.splitScreen == 1 ? m_options.outputWidth / 2 : 0;
        UINT      destY = m_options.splitScreen == 2 ? m_options.outputHeight / 2 : 0;
        box.left        = destX;
        box.top         = destY;
        box.right       = m_options.outputWidth;
        box.bottom      = m_options.outputHeight;
        box.front       = 0;
        box.back        = 1;
        m_renderContext.deviceContext->CopySubresourceRegion(
            m_renderContext.outputTexture.get(), 0, destX, destY, 0, m_renderContext.inputTextures[m_renderContext.inputSlots[0]].get(), 0, &box);
    }

    if(m_options.charts)
        m_charts.Render(m_renderContext.outputTexture, m_options.outputHeight);

    if(m_ui.RenderRequired())
    {
        targets[0] = m_uiTargetView.get();
        m_renderContext.deviceContext->OMSetRenderTargets(1, targets, NULL);
        m_ui.Render();
    }

    ID3D11RenderTargetView* null[] = { nullptr };
    m_renderContext.deviceContext->OMSetRenderTargets(1, null, NULL);

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
    else
    {
        m_renderContext.deviceContext->Flush();
    }
    m_watcher.FrameSubmitted();
}

void Renderer::Present(bool vsync)
{
    DXGI_PRESENT_PARAMETERS pp {};
    m_swapChain->Present1(vsync ? 1 : 0, 0, &pp);
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
    for(auto t = 0; t < m_renderContext.inputTextures.size(); t++)
    {
        if(!inputUsed[t])
            return t;
    }
    return m_renderContext.inputSlots.back();
}

const winrt::com_ptr<ID3D11Texture2D>& Renderer::GetNextInput() const
{
    return m_renderContext.inputTextures[GetNextSlot()];
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

void Renderer::Benchmark()
{
    const float benchmarkDuration = 2 * TICKS_PER_SEC;

    auto  start  = Helpers::GetTicks();
    float end    = start;
    int   frames = 0;
    do
    {
        Render(false);
        frames++;
        end = Helpers::GetTicks();
    } while(end < start + benchmarkDuration);
    m_ui.SetBenchmark(end != start ? frames / ((end - start) / TICKS_PER_SEC) : 0);
}

void Renderer::Create()
{
    auto dxgiDevice = m_renderContext.device.as<IDXGIDevice1>();
    dxgiDevice->SetMaximumFrameLatency(m_options.maxQueuedFrames);
    if(m_options.gpuThreadPriority)
        dxgiDevice->SetGPUThreadPriority(m_options.gpuThreadPriority | 0x40000000);

#if _DEBUG
    m_debug = m_renderContext.device.as<ID3D11Debug>();
#endif

    winrt::com_ptr<IDXGIAdapter> dxgiAdapter;
    THROW(dxgiDevice->GetAdapter(dxgiAdapter.put()), "Unable to get DXGI adapter");
    winrt::com_ptr<IDXGIFactory2> dxgiFactory;
    THROW(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), (void**)dxgiFactory.put()), "Unable to get DXGI factory");

    DXGI_SWAP_CHAIN_DESC1 d3d11SwapChainDesc = {};
    d3d11SwapChainDesc.Width                 = 0;
    d3d11SwapChainDesc.Height                = 0;
    d3d11SwapChainDesc.Format                = m_options.format;
    d3d11SwapChainDesc.SampleDesc.Count      = 1;
    d3d11SwapChainDesc.SampleDesc.Quality    = 0;
    d3d11SwapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    d3d11SwapChainDesc.BufferCount           = m_options.swapChainBuffers;
    d3d11SwapChainDesc.Scaling               = DXGI_SCALING_NONE;
    d3d11SwapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    d3d11SwapChainDesc.AlphaMode             = DXGI_ALPHA_MODE_UNSPECIFIED;
    d3d11SwapChainDesc.Flags                 = 0;
    // fun fact: on Intel Arc setting DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT
    // makes Present(1) not wait on vsync any more and BSODs Windows

    THROW(dxgiFactory->CreateSwapChainForHwnd(m_renderContext.device.get(), m_options.outputWindow, &d3d11SwapChainDesc, 0, 0, m_swapChain.put()), "Unable to create SwapChain");

    /*
    *  works if using Output for another display, but breaks WGC capture if a window is dragged onto it for example
    */
    if(m_options.exclusive)
    {
        winrt::com_ptr<IDXGIOutput> output;
        THROW(dxgiAdapter->EnumOutputs(m_options.shaderDisplayNo, output.put()), "Unable to get output");

        DXGI_OUTPUT_DESC outputDesc;
        THROW(output->GetDesc(&outputDesc), "Unable to get output desc");

        THROW(m_swapChain->SetFullscreenState(TRUE, output.get()), "Unable to set fullscreen");

        THROW(m_swapChain->ResizeBuffers(d3d11SwapChainDesc.BufferCount, m_options.outputWidth, m_options.outputHeight, m_options.format, 0), "Unable to resize buffers");
    }

    if(m_options.useHdr)
    {
        winrt::com_ptr<IDXGISwapChain3> swapChain3;
        THROW(m_swapChain->QueryInterface(__uuidof(IDXGISwapChain3), reinterpret_cast<void**>(swapChain3.put())), "Unable to get SwapChain3");
        THROW(swapChain3->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709), "Unable to set HDR ColorSpace");
    }

    THROW(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)m_renderContext.outputTexture.put()), "Unable to get display buffer");

    D3D11_RENDER_TARGET_VIEW_DESC rtv {};
    rtv.Format        = m_options.hardwareSrgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : m_options.format;
    rtv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    THROW(m_renderContext.device->CreateRenderTargetView(m_renderContext.outputTexture.get(), &rtv, m_renderContext.outputTargetView.put()), "Unable to create render target");

    D3D11_RENDER_TARGET_VIEW_DESC uitv {};
    uitv.Format        = m_options.format;
    uitv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    THROW(m_renderContext.device->CreateRenderTargetView(m_renderContext.outputTexture.get(), &uitv, m_uiTargetView.put()), "Unable to create render target");

    D3D11_RASTERIZER_DESC rsDesc {};
    rsDesc.CullMode          = D3D11_CULL_NONE;
    rsDesc.FillMode          = D3D11_FILL_SOLID;
    rsDesc.DepthClipEnable   = FALSE;
    rsDesc.MultisampleEnable = FALSE;
    rsDesc.ScissorEnable     = TRUE;
    THROW(m_renderContext.device->CreateRasterizerState(&rsDesc, m_rasterizerState.put()), "Unable to create rasterizer state");

    D3D11_DEPTH_STENCIL_DESC dsDesc {};
    dsDesc.DepthEnable   = false;
    dsDesc.StencilEnable = false;
    THROW(m_renderContext.device->CreateDepthStencilState(&dsDesc, m_depthStencilState.put()), "Unable to create depth/stencil state");

    D3D11_RECT scissor {};
    scissor.left   = 0;
    scissor.top    = 0;
    scissor.right  = m_options.splitScreen == 1 ? m_options.outputWidth / 2 : m_options.outputWidth;
    scissor.bottom = m_options.splitScreen == 2 ? m_options.outputHeight / 2 : m_options.outputHeight;

    m_renderContext.deviceContext->RSSetState(m_rasterizerState.get());
    m_renderContext.deviceContext->OMSetDepthStencilState(m_depthStencilState.get(), 0);
    m_renderContext.deviceContext->OMSetBlendState(NULL, NULL, 1);
    m_renderContext.deviceContext->RSSetScissorRects(1, &scissor);

    m_renderContext.frameNo    = 0;
    m_renderContext.subFrameNo = 0;
}

void Renderer::CreateInputs()
{
    auto inputsRequired = m_shaderManager.NumInputsRequired();
    if(inputsRequired > MAX_INPUTS)
        THROW(E_FAIL, "Too many inputs");

    D3D11_TEXTURE2D_DESC desc {};
    desc.Width              = m_options.outputWidth;
    desc.Height             = m_options.outputHeight;
    desc.ArraySize          = 1;
    desc.Format             = m_options.hardwareSrgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : m_options.format;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.MipLevels          = 1;
    desc.MiscFlags          = 0;
    desc.CPUAccessFlags     = m_options.crossAdapter ? D3D11_CPU_ACCESS_WRITE : 0;
    desc.Usage              = m_options.crossAdapter ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
    desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA data {};
    auto                   formatWidth = m_options.useHdr ? 2 : 1;
    std::vector<uint32_t>  initialData;
    initialData.resize(m_options.outputWidth * m_options.outputHeight * formatWidth);
    data.pSysMem          = initialData.data();
    data.SysMemPitch      = m_options.outputWidth * sizeof(uint32_t) * formatWidth;
    data.SysMemSlicePitch = m_options.outputWidth * m_options.outputHeight * sizeof(uint32_t) * formatWidth;
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
        m_renderContext.inputSlots.push_back(i);
        auto& inputTexture     = m_renderContext.inputTextures.emplace_back(nullptr);
        auto& inputTextureView = m_renderContext.inputTextureViews.emplace_back(nullptr);
        THROW(m_renderContext.device->CreateTexture2D(&desc, &data, inputTexture.put()), "Unable to create texture");
        THROW(m_renderContext.device->CreateShaderResourceView(inputTexture.get(), nullptr, inputTextureView.put()), "Unable to create input view");
    }
}

void Renderer::Destroy()
{
    m_depthStencilState              = nullptr;
    m_rasterizerState                = nullptr;
    m_swapChain                      = nullptr;
    m_uiTargetView                   = nullptr;
    m_renderContext.frameNo          = 0;
    m_renderContext.subFrameNo       = 0;
    m_renderContext.outputTargetView = nullptr;
    m_renderContext.outputTexture    = nullptr;
    DestroyInputs();
    if(m_renderContext.deviceContext)
    {
        m_renderContext.deviceContext->RSSetState(NULL); // fix exception in nvapi...
        m_renderContext.deviceContext->ClearState();
        m_renderContext.deviceContext->Flush();
        m_renderContext.deviceContext = nullptr;
    }
#if _DEBUG
    m_debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
    m_debug = nullptr;
#endif
    m_renderContext.device = nullptr;
}

void Renderer::DestroyInputs()
{
    for(int i = 0; i < m_renderContext.inputTextureViews.size(); i++)
        m_renderContext.inputTextureViews[i] = nullptr;
    m_renderContext.inputTextureViews.clear();
    for(int i = 0; i < m_renderContext.inputTextures.size(); i++)
        m_renderContext.inputTextures[i] = nullptr;
    m_renderContext.inputTextures.clear();
    m_renderContext.inputSlots.clear();
}

} // namespace ShaderBeam