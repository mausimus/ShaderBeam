/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "Common.h"
#include "RenderContext.h"

namespace ShaderBeam
{

class ShaderProfile
{
public:
    virtual void Create(const RenderContext& renderContext) = 0;
    virtual void Render(const RenderContext& renderContext) = 0;
    virtual void Destroy()                                  = 0;

    std::string                m_name;
    std::vector<ParameterInfo> m_parameterInfos;
    int                        m_numInputs { 1 };

    void         ResetDefaults();
    virtual bool NewInputRequired(const RenderContext& renderContext) const;
    virtual bool SupportsResync(const RenderContext& renderContext) const;

protected:
    std::map<int, std::string> m_empty;

    void AddParameter(const char* name, const char* description, float* value, float min, float max);
    void AddParameter(const char* name, const char* description, int* value, int min, int max);
    void AddParameter(const char* name, const char* description, int* value, int min, int max, const std::map<int, std::string>& dropdown);
};
} // namespace ShaderBeam