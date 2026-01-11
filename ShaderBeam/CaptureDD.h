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

class CaptureDD : public CaptureBase
{
public:
    CaptureDD(Watcher& watcher, const Options& options);

    bool IsSupported();
    bool SupportsWindowCapture();

protected:
    void InternalStart();
    bool InternalPoll(const winrt::com_ptr<ID3D11Texture2D>& outputTexture);
    void InternalStop();

private:
    Watcher& m_watcher;

    winrt::com_ptr<IDXGIOutput1>           m_capturedOutput;
    winrt::com_ptr<IDXGIOutputDuplication> m_desktopDuplication;
    unsigned                               m_width { 0 };
    unsigned                               m_height { 0 };
};
} // namespace ShaderBeam
