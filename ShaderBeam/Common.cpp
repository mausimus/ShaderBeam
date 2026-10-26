#include "stdafx.h"

#include "Common.h"
#include "ShaderManager.h"
#include "ini.h"

#define LOAD_BOOL(ini, var) var = ini.has(#var) ? ini.get(#var) == "1" : var;
#define LOAD_INT(ini, var) var = ini.has(#var) ? std::stoi(ini.get(#var)) : var;
#define LOAD_STRING(ini, var) var = ini.has(#var) ? ini.get(#var) : var;
#define LOAD_FLOAT(ini, var) var = ini.has(#var) ? std::stof(ini.get(#var)) : var;

#define SAVE_VERSION(ini) ini["version"] = "2";
#define SAVE_BOOL(ini, var) ini[#var] = var ? "1" : "0";
#define SAVE_INT(ini, var) ini[#var] = std::to_string(var);
#define SAVE_STRING(ini, var) ini[#var] = var;
#define SAVE_FLOAT(ini, var) ini[#var] = std::to_string(var);

#define SETTINGS_FILE "ShaderBeam.ini"

namespace ShaderBeam
{
void Options::Save(const ShaderManager& shaderManager) const
{
    try
    {
        mINI::INIFile      file(std::filesystem::path(SETTINGS_FILE));
        mINI::INIStructure ini;

        if(!rememberSettings)
        {
            ini.remove("settings");
            ini.remove("shader");
        }

        auto& s = ini["settings"];
        SAVE_VERSION(s)

        SAVE_BOOL(s, rememberSettings);
        if(rememberSettings)
        {
            SAVE_BOOL(s, ui)
            SAVE_BOOL(s, banner)
            SAVE_INT(s, shaderProfileNo)
            SAVE_INT(s, captureAdapterNo)
            SAVE_INT(s, shaderAdapterNo)
            SAVE_INT(s, captureDisplayNo)
            SAVE_INT(s, shaderDisplayNo)
            SAVE_INT(s, captureMethod)
            SAVE_INT(s, splitScreen)
            SAVE_BOOL(s, hardwareSrgb)
            SAVE_INT(s, monitorType)
            SAVE_BOOL(s, autoSync)
            SAVE_BOOL(s, useHdr)
            SAVE_INT(s, maxQueuedFrames)
            SAVE_FLOAT(s, areaLeft)
            SAVE_FLOAT(s, areaTop)
            SAVE_FLOAT(s, areaRight)
            SAVE_FLOAT(s, areaBottom)
            SAVE_FLOAT(s, areaRadius)
            SAVE_FLOAT(s, areaCurvatureX)
            SAVE_FLOAT(s, areaCurvatureY)

            auto& shader = ini["shader"];
            for(const auto& param : shaderManager.GetParameterInfos())
            {
                switch(param.type)
                {
                case ParamInt:
                    if(*param.p.ip.value != param.p.ip.def)
                        shader[param.name] = std::to_string(*param.p.ip.value);
                    break;
                case ParamFloat:
                    if(*param.p.fp.value != param.p.fp.def)
                        shader[param.name] = std::to_string(*param.p.fp.value);
                    break;
                }
            }
        }

        file.write(ini);
    }
    catch(...)
    { }
}

void Options::Load(ShaderManager& shaderManager)
{
    try
    {
        mINI::INIFile      file(std::filesystem::path(SETTINGS_FILE));
        mINI::INIStructure ini;

        file.read(ini);
        if(ini.has("settings"))
        {
            auto s = ini.get("settings");
            LOAD_BOOL(s, rememberSettings);
            if(rememberSettings)
            {
                LOAD_BOOL(s, ui)
                LOAD_BOOL(s, banner)
                LOAD_INT(s, shaderProfileNo)
                LOAD_INT(s, captureAdapterNo)
                LOAD_INT(s, shaderAdapterNo)
                LOAD_INT(s, captureDisplayNo)
                LOAD_INT(s, shaderDisplayNo)
                LOAD_INT(s, captureMethod)
                LOAD_INT(s, splitScreen)
                LOAD_BOOL(s, hardwareSrgb)
                LOAD_INT(s, monitorType)
                LOAD_BOOL(s, autoSync)
                LOAD_BOOL(s, useHdr)
                LOAD_INT(s, maxQueuedFrames)
                LOAD_FLOAT(s, areaLeft)
                LOAD_FLOAT(s, areaTop)
                LOAD_FLOAT(s, areaRight)
                LOAD_FLOAT(s, areaBottom)
                LOAD_FLOAT(s, areaRadius)
                LOAD_FLOAT(s, areaCurvatureX)
                LOAD_FLOAT(s, areaCurvatureY)
            }

            if(ini.has("shader") && shaderProfileNo <= shaderManager.GetShaders().size())
            {
                auto& shader = ini["shader"];
                for(const auto& param : shaderManager.GetParameterInfos(shaderProfileNo))
                {
                    if(shader.has(param.name))
                    {
                        switch(param.type)
                        {
                        case ParamInt:
                            *param.p.ip.value = std::stoi(shader[param.name]);
                            break;
                        case ParamFloat:
                            *param.p.fp.value = std::stof(shader[param.name]);
                            break;
                        }
                    }
                }
            }
        }
    }
    catch(...)
    { }
}

EffectArea Options::GetEffectArea() const
{
    return EffectArea { areaLeft, areaTop, areaRight, areaBottom, areaRadius, areaCurvatureX, areaCurvatureY };
}
} // namespace ShaderBeam