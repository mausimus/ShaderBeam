/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "SimpleBFIShader.h"
#include "CRTBeamSimulatorShader.h"

namespace ShaderBeam
{

static ShaderProfile* s_shaders[] = {
    new CRTBeamSimulatorShader(),
    new SimpleBFIShader(),
};
}