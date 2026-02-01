/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "ShaderBeam.h"
#include "Helpers.h"
#include "CaptureDD.h"
#include "CaptureWGC.h"
#include "CaptureFile.h"

namespace ShaderBeam
{

ShaderBeam::ShaderBeam() :
    m_options(), m_ui(m_options, m_shaderManager), m_watcher(m_ui), m_renderer(m_options, m_ui, m_watcher, m_shaderManager), m_renderThread(m_options, m_ui, m_renderer)
{ }

void ShaderBeam::Create(HWND hwnd, GLFWwindow* window)
{
    m_options.hwnd   = hwnd;
    m_options.window = window;

    m_ui.m_adapters = GetAdapters();
    m_ui.m_displays = GetDisplays();
    m_ui.m_shaders  = m_shaderManager.GetShaders();

#ifdef _WIN32
    auto wgc = std::make_shared<CaptureWGC>(m_watcher, m_options);
    if(wgc->IsSupported())
        m_ui.m_captures.emplace_back((int)m_ui.m_captures.size(), wgc->m_name, wgc);
    auto dd = std::make_shared<CaptureDD>(m_watcher, m_options);
    if(dd->IsSupported())
        m_ui.m_captures.emplace_back((int)m_ui.m_captures.size(), dd->m_name, dd);
#endif
    auto fc = std::make_shared<CaptureFile>(m_watcher, m_options);
    if(fc->IsSupported())
        m_ui.m_captures.emplace_back((int)m_ui.m_captures.size(), fc->m_name, fc);

    m_ui.m_monitorTypes.push_back("LCD");
    m_ui.m_monitorTypes.push_back("OLED");

    m_ui.m_queuedFrames.push_back("Driver Default");
    for(int i = 1; i <= 14; i++)
        m_ui.m_queuedFrames.push_back(std::to_string(i));

    m_ui.m_splitScreens.push_back("None");
    m_ui.m_splitScreens.push_back("Vertical");
    m_ui.m_splitScreens.push_back("Horizontal");

    m_options.Load(m_shaderManager);

    DefaultOptions();
}

void ShaderBeam::Destroy() { }

void ShaderBeam::RunBenchmark()
{
    m_renderThread.Benchmark();
}

void ShaderBeam::UpdateVsyncRate()
{
    // get VSync duration from DWM (should match fastest display)
    #ifdef _WIN32
    DWM_TIMING_INFO dwmInfo {};
    dwmInfo.cbSize = sizeof(dwmInfo);
    DwmGetCompositionTimingInfo(NULL, &dwmInfo);
    m_options.vsyncDuration = Helpers::QPCToDeltaMs(dwmInfo.qpcRefreshPeriod);
    m_options.vsyncRate     = 1000.0f / m_options.vsyncDuration;
    #else
    m_options.vsyncDuration = 16.6666f;
    m_options.vsyncRate     = 60.0f;
    #endif
}

void ShaderBeam::DefaultOptions()
{
    UpdateVsyncRate();

    m_options.subFrames = (int)roundf(m_options.vsyncRate / 60);
    if(m_options.subFrames <= 0)
        m_options.subFrames = 1;

    if(m_options.captureAdapterNo >= m_ui.m_adapters.size())
        m_options.captureAdapterNo = 0;
    if(m_options.shaderAdapterNo >= m_ui.m_adapters.size())
        m_options.shaderAdapterNo = 0;
    if(m_options.captureDisplayNo >= m_ui.m_displays.size())
        m_options.captureDisplayNo = 0;
    if(m_options.shaderDisplayNo >= m_ui.m_displays.size())
        m_options.shaderDisplayNo = 0;
    if(m_options.monitorType >= m_ui.m_monitorTypes.size())
        m_options.monitorType = 0;
    if(m_options.captureMethod >= m_ui.m_captures.size())
        m_options.captureMethod = 0;
    if(m_options.splitScreen >= m_ui.m_splitScreens.size())
        m_options.splitScreen = 0;
    if(m_options.shaderProfileNo >= m_ui.m_shaders.size())
        m_options.shaderProfileNo = 0;
}

void ShaderBeam::Start()
{
    m_options.crossAdapter     = m_options.shaderAdapterNo != m_options.captureAdapterNo;
    const auto& captureAdapter = m_ui.m_adapters.at(m_options.captureAdapterNo);
    const auto& shaderAdapter  = m_ui.m_adapters.at(m_options.shaderAdapterNo);
    auto        captureDevice  = Helpers::CreateD3DDevice(captureAdapter.adapter.get());
    auto        shaderDevice   = m_options.crossAdapter ? Helpers::CreateD3DDevice(shaderAdapter.adapter.get()) : captureDevice;

    UpdateVsyncRate();

    const auto& captureDisplay = m_ui.m_displays.at(m_options.captureDisplayNo);
    const auto& shaderDisplay  = m_ui.m_displays.at(m_options.shaderDisplayNo);

    m_options.captureMonitor   = captureDisplay.monitor;
    m_options.outputWidth      = shaderDisplay.width;
    m_options.outputHeight     = shaderDisplay.height;
    m_options.swapChainBuffers = m_options.maxQueuedFrames ? m_options.maxQueuedFrames + 1 : 3;
    m_options.format           = m_options.useHdr ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM;
    if(m_options.useHdr)
        m_options.hardwareSrgb = false;

    winrt::com_ptr<ID3D11DeviceContext> deviceContext;
    shaderDevice->GetImmediateContext(deviceContext.put());
    const auto& capture = m_ui.m_captures[m_options.captureMethod].api;
    if(m_options.captureWindow && !capture->SupportsWindowCapture())
        m_options.captureWindow = NULL;

    try
    {
        m_ui.Start(m_options.window, m_options.scale, shaderDevice, deviceContext);
    }
    catch(std::exception& ex)
    {
        ErrorMessage(ex.what());
        AppMessage(WM_USER_QUIT, 0, 0);
        return;
    }

    try
    {
        m_renderer.Start(shaderDevice, deviceContext);
    }
    catch(std::exception& ex)
    {
        ErrorMessage(ex.what());
        AppMessage(WM_USER_QUIT, 0, 0);
        return;
    }

    m_watcher.Start();

    try
    {
        capture->Start(captureDevice.as<IDXGIDevice>(), deviceContext);
    }
    catch(std::exception& ex)
    {
        m_ui.SetError(ex.what());
    }
  
    m_renderThread.Start(capture);

    m_active = true;
}

void ShaderBeam::Stop()
{
    m_options.Save(m_shaderManager);

    m_renderThread.Stop();
    m_ui.m_captures[m_options.captureMethod].api->Stop();
    m_watcher.Stop();
    m_renderer.Stop();
    m_ui.Stop();

    m_active = false;
}

std::vector<winrt::com_ptr<IDXGIAdapter1>> ShaderBeam::EnumerateAdapters()
{
    winrt::com_ptr<IDXGIAdapter1>              pAdapter;
    std::vector<winrt::com_ptr<IDXGIAdapter1>> vAdapters;
    IDXGIFactory1*                             pFactory = NULL;

    if(FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory)))
    {
        return vAdapters;
    }
    for(UINT i = 0; pFactory->EnumAdapters1(i, pAdapter.put()) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        vAdapters.push_back(pAdapter/*.as<IDXGIAdapter2>()*/);
    }
    if(pFactory)
    {
        pFactory->Release();
    }
    return vAdapters;
}

std::vector<AdapterInfo> ShaderBeam::GetAdapters()
{
    std::vector<AdapterInfo> adapters;
    const auto&              dxgiAdapters = EnumerateAdapters();
    int                      no = 0;
    for(auto adapter : dxgiAdapters)
    {
        DXGI_ADAPTER_DESC desc;
        if(SUCCEEDED(adapter->GetDesc(&desc)))
        {
            if((desc.VendorId == 0x1414) && (desc.DeviceId == 0x8c))
                continue; // Microsoft Basic Render Driver

            /*const char* preempt = "";
            switch(desc.GraphicsPreemptionGranularity)
            {
            case DXGI_GRAPHICS_PREEMPTION_DMA_BUFFER_BOUNDARY:
                preempt = " (D)";
                break;
            case DXGI_GRAPHICS_PREEMPTION_PRIMITIVE_BOUNDARY:
                preempt = " (P)";
                break;
            case DXGI_GRAPHICS_PREEMPTION_TRIANGLE_BOUNDARY:
                preempt = " (T)";
                break;
            case DXGI_GRAPHICS_PREEMPTION_PIXEL_BOUNDARY:
                preempt = " (PX)";
                break;
            case DXGI_GRAPHICS_PREEMPTION_INSTRUCTION_BOUNDARY:
                preempt = " (I)";
                break;
            }*/
            auto name = std::string("#") + std::to_string(no + 1) + ": ";// + Helpers::WCharToString(desc.Description) + std::string(preempt);
            adapters.emplace_back(no++, name, adapter, desc.AdapterLuid);
        }
    }
    return adapters;
}

#ifdef _WIN32
static BOOL CALLBACK EnumDisplayMonitorsProc(_In_ HMONITOR hMonitor, _In_ HDC hDC, _In_ LPRECT lpRect, _In_ LPARAM lParam)
{
    std::vector<DisplayInfo>* displays = (std::vector<DisplayInfo>*)lParam;

    MONITORINFOEX info;
    info.cbSize = sizeof(info);
    GetMonitorInfo(hMonitor, &info);
    unsigned w    = info.rcMonitor.right - info.rcMonitor.left;
    unsigned h    = info.rcMonitor.bottom - info.rcMonitor.top;
    unsigned no   = (unsigned)displays->size();
    auto     name = std::string("Desktop ") + std::to_string(no + 1);
    displays->emplace_back(no, w, h, name, hMonitor);
    return true;
}
#endif

std::vector<DisplayInfo> ShaderBeam::GetDisplays()
{
    std::vector<DisplayInfo> displays;
    #ifdef _WIN32
    EnumDisplayMonitors(NULL, NULL, EnumDisplayMonitorsProc, (LPARAM)&displays);
    #else
    displays.emplace_back(0, 1920, 1080, "Dummy Display", nullptr);
    #endif
    return displays;
}

} // namespace ShaderBeam