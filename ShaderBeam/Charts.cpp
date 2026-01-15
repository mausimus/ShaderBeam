/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "Charts.h"
#include "Helpers.h"

namespace ShaderBeam
{

#define CHARTS_H 256
#define CHARTS_W (CHARTS_LEN + 1)
#define CHART_H 128

Charts::Charts(const Options& options, Watcher& watcher) : m_options(options), m_watcher(watcher) { }

void Charts::Create(winrt::com_ptr<ID3D11Device> device, winrt::com_ptr<ID3D11DeviceContext> context)
{
    m_device  = device;
    m_context = context;
    m_chartsBuffer.resize(CHARTS_W * CHARTS_H);

    D3D11_TEXTURE2D_DESC desc {};
    desc.Width              = CHARTS_W;
    desc.Height             = CHARTS_H;
    desc.ArraySize          = 1;
    desc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;
    desc.MipLevels          = 1;
    desc.MiscFlags          = 0;
    desc.Usage              = D3D11_USAGE_DEFAULT;
    desc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA initialData {};
    initialData.pSysMem     = m_chartsBuffer.data();
    initialData.SysMemPitch = CHARTS_W * 4;

    THROW(m_device->CreateTexture2D(&desc, &initialData, m_chartsTexture.put()), "Unable to create charting texture");
}

void Charts::Destroy()
{
    m_chartsTexture = nullptr;
    m_context       = nullptr;
    m_device        = nullptr;
    m_chartsBuffer.clear();
}

void Charts::Render(const winrt::com_ptr<ID3D11Texture2D>& texture, int bottom)
{
    Update();

    if(m_options.useHdr)
        return;

    D3D11_BOX box {};
    box.top    = 0;
    box.left   = 0;
    box.right  = CHARTS_W;
    box.bottom = CHARTS_H;
    box.front  = 0;
    box.back   = 1;
    m_context->CopySubresourceRegion(texture.get(), 0, 0, bottom - CHARTS_H, 0, m_chartsTexture.get(), 0, &box);
}

int Charts::Clamp(float v, float min, float max, int resolution)
{
    return (int)(std::clamp((v - min) / (max - min), 0.0f, 1.0f) * resolution);
}

void Charts::Update(Chart& chart, int y, float min, float max)
{
    int   index;
    float value;
    while(chart.Read(index, value))
    {
        int       height = CHART_H - Clamp(value - min, 0, max - min, CHARTS_H);
        uint32_t* start  = m_chartsBuffer.data() + index;
        uint32_t* ptr    = start;
        for(int y = 0; y < height; y++)
        {
            *ptr++ = 0xffffffff;
            *ptr   = 0xffff3333;
            ptr += CHARTS_W - 1;
        }
        for(int y = 0; y < CHART_H - height; y++)
        {
            *ptr++ = 0xff3333ff;
            *ptr   = 0xffff3333;
            ptr += CHARTS_W - 1;
        }

        D3D11_BOX box {};
        box.top    = y;
        box.bottom = y + CHART_H;
        box.left   = index;
        box.right  = box.left + 2;
        box.front  = 0;
        box.back   = 1;

        m_context->UpdateSubresource(m_chartsTexture.get(), 0, &box, start, CHARTS_W * 4, 1);
    }
}

void Charts::Update()
{
    if(m_options.charts)
    {
        Update(m_watcher.m_receiveChart, 0, 0.0f, 50.0f);
        Update(m_watcher.m_submitChart, CHART_H, 0.0f, 50.0f);
    }
}
} // namespace ShaderBeam