/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "CaptureFile.h"
#include "Helpers.h"

namespace ShaderBeam
{

CaptureFile::CaptureFile(Watcher& watcher, const Options& options) : CaptureBase(watcher, options)
{
    m_name = "Dummy Capture";
}

bool CaptureFile::IsSupported()
{
    return true;
}

bool CaptureFile::SupportsWindowCapture()
{
    return false;
}

void CaptureFile::InternalStart()
{
    m_frameNo = 0;
    m_data.resize(m_options.outputWidth * m_options.outputHeight);
}

void CaptureFile::InternalStop()
{
    m_data.clear();
}

bool CaptureFile::InternalPoll(const winrt::com_ptr<ID3D11Texture2D>& outputTexture)
{
    uint32_t* dp = m_data.data();
    for(int y = 0; y < m_options.outputHeight; y++)
        for(int x = 0; x < m_options.outputWidth; x++)
        {
            auto xf = x + m_frameNo * 8;
            auto v  = ((xf / 20) % 2) * 255;
            *dp++   = 0xff000000 + (v << 16) + (v << 8) + v;
        }

    m_frameNo++;
    CopyToOutput(m_data.data(), m_data.size() * sizeof(uint32_t), outputTexture);
    return true;
}

} // namespace ShaderBeam
