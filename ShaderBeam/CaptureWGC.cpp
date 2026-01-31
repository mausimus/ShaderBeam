/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "CaptureWGC.h"
#include "Helpers.h"

#include <windows.graphics.directx.direct3d11.interop.h>
#include <windows.graphics.capture.interop.h>

namespace ShaderBeam
{

CaptureWGC::CaptureWGC(Watcher& watcher, const Options& options) : CaptureBase(watcher, options)
{
    m_name = "Windows Graphics Capture";
}

bool CaptureWGC::IsSupported()
{
    return HasCaptureAPI();
}

bool CaptureWGC::SupportsWindowCapture()
{
    return true;
}

void CaptureWGC::InternalStart()
{
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item { 0 };

    if(m_options.captureWindow)
    {
        item            = CreateCaptureItemForWindow(m_options.captureWindow);
        m_captureWindow = m_options.captureWindow;
    }
    else
    {
        item            = CreateCaptureItemForMonitor(m_options.captureMonitor);
        m_captureWindow = NULL;
    }

    auto format =
        m_options.useHdr ? winrt::Windows::Graphics::DirectX::DirectXPixelFormat::R16G16B16A16Float : winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized;
    auto contentSize = item.Size();

    m_framePool = winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::Create(CreateDirect3DDevice(m_captureDevice.get()), format, m_options.wgcBuffers, contentSize);

    if(m_options.gpuThreadPriority)
        m_captureDevice->SetGPUThreadPriority(m_options.gpuThreadPriority | 0x40000000);

    m_session = m_framePool.CreateCaptureSession(item);

    if(CanDisableBorder())
        m_session.IsBorderRequired(false);

    if(CanSetCaptureRate())
    {
        // workaround for an older bug
        m_session.MinUpdateInterval(winrt::Windows::Foundation::TimeSpan(std::chrono::milliseconds(1)));
        m_session.MinUpdateInterval(winrt::Windows::Foundation::TimeSpan(0));
    }

    if(CanUpdateCursor())
        m_session.IsCursorCaptureEnabled(m_options.captureDisplayNo != m_options.shaderDisplayNo);

    m_session.StartCapture();
}

void CaptureWGC::InternalStop()
{
    if(m_session)
    {
        m_session.Close();
        m_session = nullptr;
    }
    if(m_framePool)
    {
        m_framePool.Close();
        m_framePool = nullptr;
    }
}

bool CaptureWGC::InternalPoll(const winrt::com_ptr<ID3D11Texture2D>& outputTexture)
{
    if(m_captureWindow && !IsWindow(m_captureWindow))
    {
        // window closed, restart capture with desktop
        AppMessage(WM_USER_NOWINDOW, 0, 0);
        return false;
    }

    auto frame = m_framePool.TryGetNextFrame();
    if(frame)
    {
        // take everything queued
        auto newerFrame = m_framePool.TryGetNextFrame();
        while(newerFrame)
        {
            swap(frame, newerFrame);
            auto timestamp = frame.SystemRelativeTime();
            m_watcher.FrameReceived(Helpers::QPCToTicks(timestamp.count()));
            newerFrame = m_framePool.TryGetNextFrame();
        }

        auto texture   = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
        auto size      = frame.ContentSize();
        auto timestamp = frame.SystemRelativeTime();
        m_watcher.FrameReceived(Helpers::QPCToTicks(timestamp.count()));
        CopyToOutput(texture, size.Width, size.Height, outputTexture);
        return true;
    }
    return false;
}

template <typename T>
winrt::com_ptr<T> CaptureWGC::GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& object)
{
    auto              access = object.as<Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
    winrt::com_ptr<T> result;
    winrt::check_hresult(access->GetInterface(winrt::guid_of<T>(), result.put_void()));
    return result;
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem CaptureWGC::CreateCaptureItemForMonitor(HMONITOR hmon)
{
    auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
    winrt::check_hresult(interop_factory->CreateForMonitor(hmon, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
    return item;
}

winrt::Windows::Graphics::Capture::GraphicsCaptureItem CaptureWGC::CreateCaptureItemForWindow(HWND hwnd)
{
    auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
    winrt::check_hresult(interop_factory->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
    return item;
}

winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice CaptureWGC::CreateDirect3DDevice(IDXGIDevice* dxgi_device)
{
    winrt::com_ptr<::IInspectable> d3d_device;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi_device, d3d_device.put()));
    return d3d_device.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
}

bool CaptureWGC::HasCaptureAPI()
{
    try
    {
        return winrt::Windows::Foundation::Metadata::ApiInformation::IsTypePresent(L"Windows.Graphics.Capture.GraphicsCaptureItem");
    }
    catch(...)
    {
        return false;
    }
}

bool CaptureWGC::CanDisableBorder()
{
    return winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsBorderRequired");
}

bool CaptureWGC::CanSetCaptureRate()
{
    return winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession", L"MinUpdateInterval");
}

bool CaptureWGC::CanUpdateCursor()
{
    return winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsCursorCaptureEnabled");
}
} // namespace ShaderBeam
