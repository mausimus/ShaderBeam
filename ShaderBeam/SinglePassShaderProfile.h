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

class SinglePassShaderBase : public ShaderProfile
{
public:
    virtual void Destroy() { }

protected:
    virtual void SetShader(const std::string& filename, const std::map<std::string, std::string>& macros, const RenderContext& renderContext) = 0;
    virtual void SetParameterBuffer(void* data, int size, const RenderContext& renderContext)                                                 = 0;
    virtual void CreatePipeline(const RenderContext& renderContext)                                                                           = 0;
    virtual void RenderPipeline(const RenderContext& renderContext)                                                                           = 0;
    virtual void UpdateParameters(const RenderContext& renderContext)                                                                         = 0;

    virtual void OverrideInputs(const RenderContext& renderContext, const std::span<void*>& inputs) { }

    void* m_parametersBuffer { nullptr };
    int   m_parametersSize { 0 };
};
} // namespace ShaderBeam

#include "SinglePassShaderD3D11.h"

