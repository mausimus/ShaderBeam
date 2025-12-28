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

class SimpleBFIShader : public ShaderProfile
{
public:
    SimpleBFIShader()
    {
        m_name = "Simple BFI";
    }

    void Create(const RenderContext& renderContext) { }

    void Destroy() { }

    void Render(const RenderContext& renderContext)
    {
        static const float black[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

        if(renderContext.subFrameNo != 0)
            renderContext.deviceContext->ClearRenderTargetView(renderContext.outputTargetView.get(), black);
        else
            Passthrough(renderContext);
    }
};
} // namespace ShaderBeam