/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "RenderContext.h"

namespace ShaderBeam
{

// Shows the captured image again outside an effect area's rounded corners and curved edges. The
// shader draws the area's whole rectangle (the scissor rectangle); this blends the captured image
// back in over the parts of it outside the area's shape, with a one-pixel soft edge.
class EffectMask
{
public:
    void Create(const RenderContext& renderContext);
    void Destroy();
    // Does nothing when the area is a plain rectangle.
    void Render(const RenderContext& renderContext, const EffectArea& area);

private:
    winrt::com_ptr<ID3D11VertexShader> m_vertexShader;
    winrt::com_ptr<ID3D11PixelShader>  m_pixelShader;
    winrt::com_ptr<ID3D11Buffer>       m_parameters;
    winrt::com_ptr<ID3D11BlendState>   m_blendState;
};
} // namespace ShaderBeam
