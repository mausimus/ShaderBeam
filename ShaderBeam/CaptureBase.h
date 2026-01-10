/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "Common.h"
#include "Watcher.h"

namespace ShaderBeam
{

class CaptureBase
{
public:
    // device is the device to capture using
    // frame & context are in device used to process
    CaptureBase(Watcher& watcher, const Options& options);
    void Start(winrt::com_ptr<IDXGIDevice> captureDevice, winrt::com_ptr<ID3D11DeviceContext> outputContext);
    void Stop();
    bool Poll(const winrt::com_ptr<ID3D11Texture2D>& outputTexture);

    virtual bool IsSupported() = 0;
    virtual bool SupportsWindowCapture() = 0;

    const char* m_name;

protected:
    Watcher&                    m_watcher;
    const Options&              m_options;
    volatile bool               m_stopping { false };
    winrt::com_ptr<IDXGIDevice> m_captureDevice;

    virtual void InternalStart()                                                    = 0;
    virtual bool InternalPoll(const winrt::com_ptr<ID3D11Texture2D>& outputTexture) = 0;
    virtual void InternalStop()                                                     = 0;

    void CopyToOutput(const winrt::com_ptr<ID3D11Texture2D>& capturedFrame, int width, int height, const winrt::com_ptr<ID3D11Texture2D>& outputTexture);

private:
    winrt::com_ptr<ID3D11DeviceContext> m_outputContext;

    // for copying between devices
    winrt::com_ptr<ID3D11Device>        m_stagingDevice;
    winrt::com_ptr<ID3D11DeviceContext> m_stagingContext;
    winrt::com_ptr<ID3D11Texture2D>     m_stagingFrame;
    bool                                m_stagingCopyRequired { false };
    int                                 m_windowX { 0 };
    int                                 m_windowY { 0 };

    void CopyTrimToOutputSize(ID3D11DeviceContext* context, ID3D11Texture2D* output, ID3D11Texture2D* source, int width, int height);
    void CopyStagingToOutput(const winrt::com_ptr<ID3D11Texture2D>& outputTexture);
};

} // namespace ShaderBeam