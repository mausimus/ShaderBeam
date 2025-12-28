/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "Common.h"

namespace ShaderBeam
{

struct RenderContext
{
    RenderContext(const Options& options) : options(options) { }

    int                                                   frameNo { 0 };
    int                                                   subFrameNo { 0 };
    winrt::com_ptr<ID3D11Device>                          device;
    winrt::com_ptr<ID3D11DeviceContext>                   deviceContext;
    std::vector<winrt::com_ptr<ID3D11Texture2D>>          inputTextures;
    std::vector<winrt::com_ptr<ID3D11ShaderResourceView>> inputTextureViews;
    std::vector<int>                                      inputSlots;
    winrt::com_ptr<ID3D11Texture2D>                       outputTexture;
    winrt::com_ptr<ID3D11RenderTargetView>                outputTargetView;

    const Options& options;
};
} // namespace ShaderBeam
