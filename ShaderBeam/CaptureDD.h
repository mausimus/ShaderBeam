/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "CaptureD3D11.h"
#include "Watcher.h"

namespace ShaderBeam
{

class CaptureDD : public CaptureD3D11
{
public:
    CaptureDD(Watcher& watcher, const Options& options, Renderer& renderer, RenderContext& renderContext);

    bool IsSupported();
    bool SupportsWindowCapture();

protected:
    void InternalStart();
    bool InternalPoll();
    void InternalStop();

private:
    Watcher& m_watcher;

    winrt::com_ptr<IDXGIOutput1>           m_capturedOutput;
    winrt::com_ptr<IDXGIOutputDuplication> m_desktopDuplication;
    unsigned                               m_width { 0 };
    unsigned                               m_height { 0 };
};
} // namespace ShaderBeam
