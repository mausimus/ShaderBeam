/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "CaptureBase.h"
#include "Watcher.h"

namespace ShaderBeam
{

class CaptureWGC : public CaptureBase
{
public:
    CaptureWGC(Watcher& watcher, const Options& options);

    bool IsSupported();
    bool SupportsWindowCapture();

protected:
    void InternalStart();
    bool InternalPoll(const winrt::com_ptr<ID3D11Texture2D>& outputTexture);
    void InternalStop();

private:
    template <typename T>
    static winrt::com_ptr<T>                                              GetDXGIInterfaceFromObject(winrt::Windows::Foundation::IInspectable const& object);
    static winrt::Windows::Graphics::Capture::GraphicsCaptureItem         CreateCaptureItemForMonitor(HMONITOR hmon);
    static winrt::Windows::Graphics::Capture::GraphicsCaptureItem         CreateCaptureItemForWindow(HWND hwnd);
    static winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice CreateDirect3DDevice(IDXGIDevice* dxgi_device);
    static bool                                                           HasCaptureAPI();
    static bool                                                           CanDisableBorder();
    static bool                                                           CanSetCaptureRate();
    static bool                                                           CanUpdateCursor();

    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool { nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession     m_session { nullptr };
    HWND                                                          m_captureWindow { NULL };
};
} // namespace ShaderBeam