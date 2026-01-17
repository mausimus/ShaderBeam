/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "CaptureDD.h"
#include "Helpers.h"

namespace ShaderBeam
{

CaptureDD::CaptureDD(Watcher& watcher, const Options& options, Renderer& renderer, RenderContext& renderContext) :
    CaptureD3D11(watcher, options, renderer, renderContext), m_watcher(watcher)
{
    m_name = "Desktop Duplication";
}

bool CaptureDD::IsSupported()
{
    return true;
}

bool CaptureDD::SupportsWindowCapture()
{
    return false;
}

void CaptureDD::InternalStart()
{
    winrt::com_ptr<IDXGIAdapter> dxgiAdapter;
    THROW(m_captureDevice->GetAdapter(dxgiAdapter.put()), "Unable to get DXGI adapter");

    UINT                        i = 0;
    winrt::com_ptr<IDXGIOutput> output;
    while(!m_capturedOutput && dxgiAdapter->EnumOutputs(i++, output.put()) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_OUTPUT_DESC desc;
        output->GetDesc(&desc);
        if(desc.Monitor == m_options.captureMonitor)
        {
            m_capturedOutput = output.try_as<IDXGIOutput1>();
            if(!m_capturedOutput)
                THROW(E_FAIL, "Unable to get DXGI output1");
            break;
        }
    }

    if(!m_capturedOutput)
        throw std::runtime_error("Unable to find IDXGIOutput for this monitor, wrong GPU?");

    THROW(m_capturedOutput->DuplicateOutput(m_captureDevice.get(), m_desktopDuplication.put()), "Unable to start Desktop Duplication");
}

void CaptureDD::InternalStop()
{
    m_desktopDuplication = nullptr;
    m_capturedOutput     = nullptr;
    m_width              = 0;
    m_height             = 0;
}

bool CaptureDD::InternalPoll()
{
    if(!m_desktopDuplication || !m_capturedOutput)
        return false;

    DXGI_OUTDUPL_FRAME_INFO       frameInfo;
    winrt::com_ptr<IDXGIResource> resource;

    auto hr = m_desktopDuplication->AcquireNextFrame(0, &frameInfo, resource.put());
    if(hr == S_OK)
    {
        m_watcher.FrameReceived(Helpers::QPCToTicks(frameInfo.LastPresentTime.QuadPart));

        auto texture = resource.as<ID3D11Texture2D>();
        if(m_width == 0 || m_height == 0)
        {
            D3D11_TEXTURE2D_DESC desc;
            texture->GetDesc(&desc);
            m_width  = desc.Width;
            m_height = desc.Height;
        }

        CopyToOutput(texture, m_width, m_height);

        THROW(m_desktopDuplication->ReleaseFrame(), "Unable to Release Frame");
        return true;
    }
    else if(hr == DXGI_ERROR_WAIT_TIMEOUT)
    {
        // return
    }
    else
    {
        THROW(hr, "Unable to aquire frame");
    }
    return false;
}
} // namespace ShaderBeam
