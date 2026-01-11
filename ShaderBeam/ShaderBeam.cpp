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

namespace ShaderBeam
{

ShaderBeam::ShaderBeam() :
    m_options(), m_ui(m_options, m_shaderManager), m_watcher(m_ui), m_renderer(m_options, m_ui, m_watcher, m_shaderManager), m_renderThread(m_options, m_ui, m_renderer)
{ }

void ShaderBeam::Create(HWND window)
{
    m_options.outputWindow = window;

    RegisterHotKey(m_options.outputWindow, HOTKEY_TOGGLEUI, MOD_CONTROL | MOD_SHIFT, HOTKEY_TOGGLEUI_KEY);
    RegisterHotKey(m_options.outputWindow, HOTKEY_BRINGTOFRONT, MOD_CONTROL | MOD_SHIFT, HOTKEY_BRINGTOFRONT_KEY);
    //RegisterHotKey(m_options.outputWindow, HOTKEY_STOPSTART, MOD_CONTROL | MOD_SHIFT, HOTKEY_STOPSTART_KEY);
    RegisterHotKey(m_options.outputWindow, HOTKEY_RESTART, MOD_CONTROL | MOD_SHIFT, HOTKEY_RESTART_KEY);
    RegisterHotKey(m_options.outputWindow, HOTKEY_QUIT, MOD_CONTROL | MOD_SHIFT, HOTKEY_QUIT_KEY);

    m_ui.m_adapters = GetAdapters();
    m_ui.m_displays = GetDisplays();
    m_ui.m_shaders  = m_shaderManager.GetShaders();

    auto wgc = std::make_shared<CaptureWGC>(m_watcher, m_options);
    if(wgc->IsSupported())
        m_ui.m_captures.emplace_back((int)m_ui.m_captures.size(), wgc->m_name, wgc);
    auto dd = std::make_shared<CaptureDD>(m_watcher, m_options);
    if(dd->IsSupported())
        m_ui.m_captures.emplace_back((int)m_ui.m_captures.size(), dd->m_name, dd);

    m_ui.m_monitorTypes.push_back("LCD");
    m_ui.m_monitorTypes.push_back("OLED");

    m_ui.m_queuedFrames.push_back("Driver Default");
    for(int i = 1; i <= 14; i++)
        m_ui.m_queuedFrames.push_back(std::to_string(i));

    m_ui.m_splitScreens.push_back("None");
    m_ui.m_splitScreens.push_back("Vertical");
    m_ui.m_splitScreens.push_back("Horizontal");

    timeBeginPeriod(1);
    Helpers::InitQPC();

    m_options.Load(m_shaderManager);

    DefaultOptions();
}

void ShaderBeam::Destroy()
{
    UnregisterHotKey(m_options.outputWindow, HOTKEY_TOGGLEUI);
    UnregisterHotKey(m_options.outputWindow, HOTKEY_BRINGTOFRONT);
    //UnregisterHotKey(m_options.outputWindow, HOTKEY_STOPSTART);
    UnregisterHotKey(m_options.outputWindow, HOTKEY_RESTART);
    UnregisterHotKey(m_options.outputWindow, HOTKEY_QUIT);

    timeEndPeriod(1);
}

void ShaderBeam::RunBenchmark()
{
    m_renderThread.Benchmark();
}

void ShaderBeam::UpdateVsyncRate()
{
    // get VSync duration from DWM (should match fastest display)
    DWM_TIMING_INFO dwmInfo {};
    dwmInfo.cbSize = sizeof(dwmInfo);
    DwmGetCompositionTimingInfo(NULL, &dwmInfo);
    m_options.vsyncDuration = Helpers::QPCToDeltaMs(dwmInfo.qpcRefreshPeriod);
    m_options.vsyncRate     = 1000.0f / m_options.vsyncDuration;
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
        float dpi = (float)GetDpiForWindow(m_options.outputWindow);
        m_ui.Start(m_options.outputWindow, dpi / USER_DEFAULT_SCREEN_DPI, shaderDevice, deviceContext);
    }
    catch(std::exception& ex)
    {
        MessageBoxA(m_options.outputWindow, ex.what(), SHADERBEAM_TITLE, MB_ICONERROR | MB_OK);
        PostMessage(m_options.outputWindow, WM_DESTROY, 0, 0);
        return;
    }

    try
    {
        m_renderer.Start(shaderDevice, deviceContext);
    }
    catch(std::exception& ex)
    {
        MessageBoxA(m_options.outputWindow, ex.what(), SHADERBEAM_TITLE, MB_ICONERROR | MB_OK);
        PostMessage(m_options.outputWindow, WM_DESTROY, 0, 0);
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

std::vector<winrt::com_ptr<IDXGIAdapter2>> ShaderBeam::EnumerateAdapters()
{
    winrt::com_ptr<IDXGIAdapter1>              pAdapter;
    std::vector<winrt::com_ptr<IDXGIAdapter2>> vAdapters;
    IDXGIFactory1*                             pFactory = NULL;

    if(FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory)))
    {
        return vAdapters;
    }
    for(UINT i = 0; pFactory->EnumAdapters1(i, pAdapter.put()) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        vAdapters.push_back(pAdapter.as<IDXGIAdapter2>());
    }
    if(pFactory)
    {
        pFactory->Release();
    }
    return vAdapters;
}

std::vector<AdapterInfo> ShaderBeam::GetAdapters()
{
    const auto&              dxgiAdapters = EnumerateAdapters();
    std::vector<AdapterInfo> adapters;
    int                      no = 0;
    for(auto adapter : dxgiAdapters)
    {
        DXGI_ADAPTER_DESC2 desc;
        if(SUCCEEDED(adapter->GetDesc2(&desc)))
        {
            if((desc.VendorId == 0x1414) && (desc.DeviceId == 0x8c))
                continue; // Microsoft Basic Render Driver

            auto name = std::string("#") + std::to_string(no + 1) + ": " + Helpers::WCharToString(desc.Description);
            adapters.emplace_back(no++, name, adapter, desc.AdapterLuid);
        }
    }
    return adapters;
}

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

std::vector<DisplayInfo> ShaderBeam::GetDisplays()
{
    std::vector<DisplayInfo> displays;
    EnumDisplayMonitors(NULL, NULL, EnumDisplayMonitorsProc, (LPARAM)&displays);
    return displays;
}

} // namespace ShaderBeam