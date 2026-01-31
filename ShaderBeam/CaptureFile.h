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

class CaptureFile : public CaptureBase
{
public:
    CaptureFile(Watcher& watcher, const Options& options);

    bool IsSupported();
    bool SupportsWindowCapture();

protected:
    void InternalStart();
    bool InternalPoll(const winrt::com_ptr<ID3D11Texture2D>& outputTexture);
    void InternalStop();

private:
    int                   m_frameNo { 0 };
    std::vector<uint32_t> m_data;
};
} // namespace ShaderBeam