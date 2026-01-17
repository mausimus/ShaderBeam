#include "stdafx.h"

#include "CaptureD3D11.h"
#include "Renderer.h"
#include "Helpers.h"

namespace ShaderBeam
{
CaptureD3D11::CaptureD3D11(Watcher& watcher, const Options& options, Renderer& renderer, RenderContext& renderContext) : CaptureBase(watcher, options, renderer, renderContext) { }

void CaptureD3D11::Start(const AdapterInfo& adapter)
{
    if(m_options.crossAdapter)
    {
        m_captureDevice = Helpers::CreateD3DDevice(adapter.adapter.get()).as<IDXGIDevice>();
    }
    else
    {
        m_captureDevice = m_renderContext.m_device.as<IDXGIDevice>();
    }
    m_windowX = 0;
    m_windowY = 0;
    if(m_options.captureWindow)
    {
        RECT captureRect;
        if(DwmGetWindowAttribute(m_options.captureWindow, DWMWA_EXTENDED_FRAME_BOUNDS, &captureRect, sizeof(RECT)) == S_OK)
        {
            auto        monitor = MonitorFromWindow(m_options.captureWindow, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitorInfo;
            monitorInfo.cbSize = sizeof(monitorInfo);
            if(GetMonitorInfo(monitor, &monitorInfo))
            {
                m_windowX = captureRect.left - monitorInfo.rcMonitor.left;
                m_windowY = captureRect.top - monitorInfo.rcMonitor.top;
            }
        }
    }

    CaptureBase::Start(adapter);
}

void CaptureD3D11::Stop()
{
    CaptureBase::Stop();

    m_stagingFrame   = nullptr;
    m_stagingContext = nullptr;
    m_stagingDevice  = nullptr;
    m_captureDevice  = nullptr;
    m_windowX        = 0;
    m_windowY        = 0;
}

void CaptureD3D11::CopyToOutput(const winrt::com_ptr<ID3D11Texture2D>& capturedFrame, int width, int height)
{
    if(m_stopping || !capturedFrame || width == 0 || height == 0)
        return;

    if(m_options.crossAdapter)
    {
        // capture and render are different adapters, we need to copy the texture via CPU
        if(!m_stagingDevice)
        {
            m_stagingDevice = m_captureDevice.as<ID3D11Device>();
            m_stagingDevice->GetImmediateContext(m_stagingContext.put());
        }
        if(!m_stagingFrame)
        {
            D3D11_TEXTURE2D_DESC stagingDesc {};
            stagingDesc.Width              = m_options.outputWidth;
            stagingDesc.Height             = m_options.outputHeight;
            stagingDesc.ArraySize          = 1;
            stagingDesc.Format             = m_options.format;
            stagingDesc.SampleDesc.Count   = 1;
            stagingDesc.SampleDesc.Quality = 0;
            stagingDesc.MipLevels          = 1;
            stagingDesc.MiscFlags          = 0;
            stagingDesc.CPUAccessFlags     = D3D11_CPU_ACCESS_READ;
            stagingDesc.Usage              = D3D11_USAGE_STAGING;
            stagingDesc.BindFlags          = 0;
            THROW(m_stagingDevice->CreateTexture2D(&stagingDesc, nullptr, m_stagingFrame.put()), "Unable to create staging texture");
        }

        // copy captured frame to staging frame (because capturedFrame wont't have CPU_ACCESS_READ)
        CopyTrimToOutputSize(m_stagingContext.get(), m_stagingFrame.get(), capturedFrame.get(), width, height);

        m_stagingCopyRequired = true;
    }
    else
    {
        // same adapter, just copy directly
        m_renderer.SubmitInput(capturedFrame, width, height, m_windowX, m_windowY);
    }
}

void CaptureD3D11::CopyStagingToOutput()
{
    // map both textures to CPU memory and copy between
    // we can do this because we are polling so render frame is not used by another thread
    D3D11_MAPPED_SUBRESOURCE stagingResource;
    THROW(m_stagingContext->Map(m_stagingFrame.get(), 0, D3D11_MAP_READ, 0, &stagingResource), "Unable to map staging texture");

    m_renderer.SubmitInput(stagingResource.pData, stagingResource.DepthPitch);

    m_stagingContext->Unmap(m_stagingFrame.get(), 0);
}

void CaptureD3D11::CopyTrimToOutputSize(ID3D11DeviceContext* context, ID3D11Texture2D* output, ID3D11Texture2D* source, int width, int height)
{
    if(width == m_options.outputWidth && height == m_options.outputHeight && m_windowX == 0 && m_windowY == 0)
    {
        context->CopyResource(output, source);
    }
    else
    {
        // truncate if monitor sizes differ
        D3D11_BOX box;
        box.left   = 0;
        box.top    = 0;
        box.right  = min(m_options.outputWidth - m_windowX, (unsigned)width);
        box.bottom = min(m_options.outputHeight - m_windowY, (unsigned)height);
        box.front  = 0;
        box.back   = 1;
        context->CopySubresourceRegion(output, 0, m_windowX, m_windowY, 0, source, 0, &box);
    }
}

} // namespace ShaderBeam