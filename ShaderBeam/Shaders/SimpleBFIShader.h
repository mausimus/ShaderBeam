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
        if(renderContext.subFrameNo != 0)
            renderContext.BlackFrame();
        else
            renderContext.Passthrough();
    }
};
} // namespace ShaderBeam