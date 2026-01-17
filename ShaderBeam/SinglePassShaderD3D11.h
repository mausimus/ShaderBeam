/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "SinglePassShaderProfile.h"

namespace ShaderBeam
{

class SinglePassShaderD3D11 : public SinglePassShaderBase
{
public:
    virtual void Destroy();

protected:
    void SetShader(const std::string& filename, const std::map<std::string, std::string>& macros, const RenderContext& renderContext);
    void SetParameterBuffer(void* data, int size, const RenderContext& renderContext);
    void CreatePipeline(const RenderContext& renderContext);
    void RenderPipeline(const RenderContext& renderContext);
    void UpdateParameters(const RenderContext& renderContext);

    winrt::com_ptr<ID3D11VertexShader> m_vertexShader;
    winrt::com_ptr<ID3D11PixelShader>  m_pixelShader;
    winrt::com_ptr<ID3D11SamplerState> m_samplerState;
    winrt::com_ptr<ID3D11Buffer>       m_constantBuffer;
};
} // namespace ShaderBeam
