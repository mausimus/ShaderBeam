/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "ShaderManager.h"
#include "Shaders/ShaderList.h"

namespace ShaderBeam
{

ShaderManager::ShaderManager()
{
    m_shaderProfiles = std::span<ShaderProfile*>(s_shaders);
}

void ShaderManager::Create(const RenderContext& renderContext, int profileNo)
{
    if(profileNo >= m_shaderProfiles.size())
        abort();

    m_activeProfile = profileNo;
    m_shaderProfiles[m_activeProfile]->Create(renderContext);
}

void ShaderManager::Render(const RenderContext& renderContext)
{
    m_shaderProfiles[m_activeProfile]->Render(renderContext);
}

void ShaderManager::Destroy()
{
    m_shaderProfiles[m_activeProfile]->Destroy();
}

std::vector<ShaderInfo> ShaderManager::GetShaders() const
{
    std::vector<ShaderInfo> shaders;
    unsigned                no = 0;
    for(const auto& profile : m_shaderProfiles)
    {
        shaders.emplace_back(no++, profile->m_name);
    }
    return shaders;
}

const std::vector<ParameterInfo>& ShaderManager::GetParameterInfos() const
{
    return m_shaderProfiles[m_activeProfile]->m_parameterInfos;
}

void ShaderManager::ResetDefaults()
{
    m_shaderProfiles[m_activeProfile]->ResetDefaults();
}

int ShaderManager::NumInputsRequired()
{
    return m_shaderProfiles[m_activeProfile]->m_numInputs;
}

bool ShaderManager::NewInputRequired(const RenderContext& renderContext) const
{
    return m_shaderProfiles[m_activeProfile]->NewInputRequired(renderContext);
}

} // namespace ShaderBeam