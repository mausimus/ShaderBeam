/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

// From Shadertoy https://www.shadertoy.com/view/XfKfWd
// - Improved version coming January 2025
// - See accompanying article https://blurbusters.com/crt
// - To study more about display science & physics, see Research Portal https://blurbusters.com/area51
// adapted for ShaderBeam by mausimus

#pragma once

#include "SinglePassShaderProfile.h"

namespace ShaderBeam
{

class CRTBeamSimulatorShader : public SinglePassShaderProfile
{
public:
    struct
    {
        float gamma { 2.2f };
        float gainVsBlur { 0.5f };
        int   scanDirection { 1 };

        // CPU-calculated
        float effectiveFramesPerHz { 4.001f };
        float crtRasterPos { 0.0f };
        float crtHzCounter { 0.0f };
    } m_params;

    int   m_fpsDivisor { 1 };
    int   m_lcdAntiRetention { 1 };
    float m_lcdInversionCompensationSlew { 0.001f };

    // derived
    float m_framesPerHz { 4.0f };

    const std::map<int, std::string> m_scanDirections = {
        { 0, "None (Global Refresh)" }, { 1, "Top to Bottom" }, { 2, "Bottom to Top" }, { 3, "Left to Right" }, { 4, "Right to Left" }
    };

    const std::map<int, std::string> m_lcdAntiRetentions = { { 0, "Force Off" }, { 1, "Auto" } };

    CRTBeamSimulatorShader()
    {
        m_numInputs = 3;
        m_name      = "Blur Busters CRT Beam Simulator";
        AddParameter("Gamma", "Your display's gamma value. Necessary to prevent horizontal-bands artifacts.", &m_params.gamma, 0.5f, 5.0f);
        AddParameter("Gain vs Blur",
                     "Brightness-vs-motionblur tradeoff for bright pixel.\n"
                     "- Defacto simulates fast/slow phosphor.\n"
                     "- 1.0 is unchanged brightness (same as non-CRT, but no blur reduction for brightest pixels, only for dimmer piels).\n"
                     "- 0.5 is half brightness spread over fewer frames (creates lower MPRT persistence for darker pixels).",
                     &m_params.gainVsBlur,
                     0.0f,
                     1.0f);
        AddParameter("Scan Direction",
                     "CRT SCAN DIRECTION. Can be useful to counteract an OS rotation of your display\n"
                     "'None' helps remove banding, but may reduce visual quality especially on OLEDs.",
                     &m_params.scanDirection,
                     0,
                     4,
                     m_scanDirections);
        // NB: this is inverted compared to original shader to be more user-friendly
        AddParameter("Slow Motion Mode",
                     "Reduced frame rate mode\n"
                     "- This can be helpful to see individual CRT-simulated frames better (educational!)\n"
                     "- 1.0 is framerate=Hz, 2.0 is framerate being half of Hz, 10 is framerate being 10% of real Hz.\n",
                     &m_fpsDivisor,
                     1,
                     100);
        AddParameter("LCD Anti-retention",
                     "Prevents image retention from BFI interfering with LCD voltage polarity inversion algorithm\n"
                     "- It will cause occasional stutter as it desyncs CRT refresh rate from content refresh rate.\n"
                     "- Auto-disabled on OLEDs and LCDs with odd subframe count.\n"
                     "- Adds one input frame of latency (!)",
                     &m_lcdAntiRetention,
                     0,
                     1,
                     m_lcdAntiRetentions);
        AddParameter("LCD Inversion Compensation",
                     "Strength of LCD Anti-retention\n"
                     "- 0.001 - Normal\n"
                     "- 0.01 - Enhanced",
                     &m_lcdInversionCompensationSlew,
                     0.0f,
                     0.02f);
    }

    void Create(const RenderContext& renderContext)
    {
        m_framesPerHz = (float)renderContext.options.subFrames;

        // macros injected into the shader before compilation with non-adjustable constants (like resolution)
        D3D_SHADER_MACRO macros[2] = {
            { "HARDWARE_SRGB", renderContext.options.hardwareSrgb ? "1" : "0" },
            { NULL, NULL },
        };

        SetShader(L"Shaders\\CRTBeamSimulator.hlsl", macros, renderContext);
        SetParameterBuffer(&m_params, sizeof(m_params), renderContext);
        CreatePipeline(renderContext);
    }

    bool NewInputRequired(const RenderContext& renderContext) const
    {
        if(renderContext.frameNo == 0)
            return renderContext.subFrameNo == 0;

        // with anti-retention on we need to check if the CRT frame counter is changing
        unsigned frameNo        = (renderContext.frameNo * renderContext.options.subFrames) + renderContext.subFrameNo;
        double   effectiveFrame = frameNo / (double)m_fpsDivisor;
        float    crtHzCounter   = (float)floor(effectiveFrame / m_params.effectiveFramesPerHz);
        return crtHzCounter != m_params.crtHzCounter;
    }

    bool AntiRetentionRequired(const RenderContext& renderContext) const
    {
        return m_lcdAntiRetention && renderContext.options.monitorType == MONITOR_LCD && floorf(m_framesPerHz) == m_framesPerHz && (((int)m_framesPerHz) % 2) == 0;
    }

    bool SupportsResync(const RenderContext& renderContext) const
    {
        return !AntiRetentionRequired(renderContext);
    }

    void OverrideInputs(const RenderContext& renderContext, const std::span<ID3D11ShaderResourceView*>& inputs)
    {
        if(!AntiRetentionRequired(renderContext))
        {
            // run shader in frame-ahead mode
            for(int slot = 1; slot < inputs.size(); slot++)
                inputs[slot] = inputs[slot - 1];
        }
    }

    void Render(const RenderContext& renderContext)
    {
        // linear frame number
        unsigned frameNo = (renderContext.frameNo * renderContext.options.subFrames) + renderContext.subFrameNo;

        //-------------------------------------------------------------------------------------------------
        // CRT beam calculations
        // Frame counter, which may be compensated by slo-mo modes (FPS_DIVISOR). Does not need to be integer divisible.
        double effectiveFrame = frameNo / (double)m_fpsDivisor;

        // LCD SAVER (prevent image retention)
        // Adds a slew to FRAMES_PER_HZ when ANTI_RETENTION is enabled and FRAMES_PER_HZ is an exact even integer.
        // We support non-integer FRAMES_PER_HZ, so this is a magically convenient solution
        m_params.effectiveFramesPerHz = (float)m_framesPerHz;
        if(AntiRetentionRequired(renderContext))
        {
            m_params.effectiveFramesPerHz += m_lcdInversionCompensationSlew;
        }

        // NB: have to use doubles here as fmodf and floorf could return inconsistent results on certain edge cases (frame 3910)

        // Normalized raster position [0..1] representing current position of simulated CRT electron beam
        m_params.crtRasterPos = (float)fmod(effectiveFrame, m_params.effectiveFramesPerHz) / m_params.effectiveFramesPerHz;

        // CRT refresh cycle counter
        m_params.crtHzCounter = (float)floor(effectiveFrame / m_params.effectiveFramesPerHz);

        UpdateParameters(renderContext);
        RenderPipeline(renderContext);
    }
};
} // namespace ShaderBeam