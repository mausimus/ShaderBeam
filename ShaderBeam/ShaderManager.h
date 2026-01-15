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

class ShaderManager
{
public:
    ShaderManager();

    void Create(const RenderContext& renderContext, int profileNo);
    void Render(const RenderContext& renderContext);
    void Destroy();

    std::vector<ShaderInfo>           GetShaders() const;
    const std::vector<ParameterInfo>& GetParameterInfos(int profileNo) const;
    const std::vector<ParameterInfo>& GetParameterInfos() const;
    void                              ResetDefaults();
    int                               NumInputsRequired();
    bool                              NewInputRequired(const RenderContext& renderContext) const;
    bool                              SupportsResync(const RenderContext& renderContext) const;

private:
    int                       m_activeProfile { 0 };
    std::span<ShaderProfile*> m_shaderProfiles;
};
} // namespace ShaderBeam