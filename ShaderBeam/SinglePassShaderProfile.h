/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "ShaderProfile.h"

namespace ShaderBeam
{

class SinglePassShaderProfile : public ShaderProfile
{
public:
    virtual void Destroy();

protected:
    void SetShader(const wchar_t* filename, D3D10_SHADER_MACRO* macros, const RenderContext& renderContext);
    void SetParameterBuffer(void* data, int size, const RenderContext& renderContext);
    void CreatePipeline(const RenderContext& renderContext);
    void RenderPipeline(const RenderContext& renderContext);
    void UpdateParameters(const RenderContext& renderContext);

    virtual void OverrideInputs(const RenderContext& renderContext, const std::span<ID3D11ShaderResourceView*>& inputs);

private:
    void* m_parametersBuffer { nullptr };
    int   m_parametersSize { 0 };

    winrt::com_ptr<ID3D11VertexShader> m_vertexShader;
    winrt::com_ptr<ID3D11PixelShader>  m_pixelShader;
    winrt::com_ptr<ID3D11SamplerState> m_samplerState;
    winrt::com_ptr<ID3D11Buffer>       m_constantBuffer;
};
} // namespace ShaderBeam