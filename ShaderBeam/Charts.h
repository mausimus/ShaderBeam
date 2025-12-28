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

class Charts
{
public:
    Charts(const Options& options, Watcher& watcher);

    void Create(winrt::com_ptr<ID3D11Device> device, winrt::com_ptr<ID3D11DeviceContext> context);
    void Render(const winrt::com_ptr<ID3D11Texture2D>& texture, int bottom);
    void Destroy();

private:
    const Options& m_options;
    Watcher&       m_watcher;

    std::vector<uint32_t>           m_chartsBuffer;
    winrt::com_ptr<ID3D11Texture2D> m_chartsTexture { nullptr };

    winrt::com_ptr<ID3D11Device>        m_device { nullptr };
    winrt::com_ptr<ID3D11DeviceContext> m_context { nullptr };

    static int Clamp(float v, float min, float max, int resolution);

    void Update(Chart& chart, int y, float min, float max);
    void Update();
};
} // namespace ShaderBeam
