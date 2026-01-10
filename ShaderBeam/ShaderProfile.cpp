/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "ShaderProfile.h"

namespace ShaderBeam
{

void ShaderProfile::Passthrough(const RenderContext& renderContext)
{
    renderContext.deviceContext->CopyResource(renderContext.outputTexture.get(), renderContext.inputTextures[renderContext.inputSlots[0]].get());
}

void ShaderProfile::AddParameter(const char* name, const char* description, float* value, float min, float max)
{
    m_parameterInfos.push_back(ParameterInfo {
        .no   = (unsigned)m_parameterInfos.size(),
        .name = name,
        .hint = description,
        .type = ParameterType::ParamFloat,
        .p    = { .fp = { .value = value, .min = min, .max = max, .def = *value } },
    });
}

void ShaderProfile::AddParameter(const char* name, const char* description, int* value, int min, int max)
{
    m_parameterInfos.push_back(ParameterInfo {
        .no   = (unsigned)m_parameterInfos.size(),
        .name = name,
        .hint = description,
        .type = ParameterType::ParamInt,
        .p    = { .ip = { .value = value, .min = min, .max = max, .def = *value } },
    });
}

void ShaderProfile::ResetDefaults()
{
    for(auto& pi : m_parameterInfos)
    {
        switch(pi.type)
        {
        case ParameterType::ParamFloat:
            *pi.p.fp.value = pi.p.fp.def;
            break;
        case ParameterType::ParamInt:
            *pi.p.ip.value = pi.p.ip.def;
            break;
        default:
            abort();
        }
    }
}

bool ShaderProfile::NewInputRequired(const RenderContext& renderContext) const
{
    return renderContext.subFrameNo == 0;
}

bool ShaderProfile::SupportsResync(const RenderContext& renderContext) const
{
    return true;
}

} // namespace ShaderBeam