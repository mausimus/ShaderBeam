#pragma once

#include "CaptureBase.h"

namespace ShaderBeam
{
class CaptureD3D11 : public CaptureBase
{
public:
    CaptureD3D11(Watcher& watcher, const Options& options, Renderer& renderer, RenderContext& renderContext);
    void Start(const AdapterInfo& adapter);
    void Stop();

protected:
    winrt::com_ptr<IDXGIDevice> m_captureDevice;

    // for copying between devices
    winrt::com_ptr<ID3D11Device>        m_stagingDevice;
    winrt::com_ptr<ID3D11DeviceContext> m_stagingContext;
    winrt::com_ptr<ID3D11Texture2D>     m_stagingFrame;

    void CopyStagingToOutput();
    void CopyToOutput(const winrt::com_ptr<ID3D11Texture2D>& capturedFrame, int width, int height);
    void CopyTrimToOutputSize(ID3D11DeviceContext* context, ID3D11Texture2D* output, ID3D11Texture2D* source, int width, int height);
};
} // namespace ShaderBeam