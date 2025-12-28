// From Shadertoy https://www.shadertoy.com/view/XfKfWd
// - Improved version coming January 2025
// - See accompanying article https://blurbusters.com/crt
// - To study more about display science & physics, see Research Portal https://blurbusters.com/area51
// adapted for ShaderBeam by mausimus

cbuffer Params : register(b0)
{
    float param_gamma;
    float param_gainVsBlur;
    uint param_scanDirection;
    
    // calculated on CPU side
    float param_effectiveFramesPerHz;
    float param_crtRasterPos;
    float param_crtHzCounter;
};

#define GAMMA                   param_gamma
#define GAIN_VS_BLUR            param_gainVsBlur
#define EFFECTIVE_FRAMES_PER_HZ param_effectiveFramesPerHz

Texture2D<float4> iChannel0 : register(t2);
Texture2D<float4> iChannel1 : register(t3);
Texture2D<float4> iChannel2 : register(t4);
SamplerState iChannel_sampler : register(s2);

/*********************************************************************************************************************/
//
//                     Blur Busters CRT Beam Simulator BFI
//                       With Seamless Gamma Correction
//
//         From Blur Busters Area 51 Display Science, Research & Engineering
//                      https://www.blurbusters.com/area51
//
//             The World's First Realtime Blur-Reducing CRT Simulator
//       Best for 60fps on 240-480Hz+ Displays, Still Works on 120Hz+ Displays
//                 Original Version 2022. Publicly Released 2024.
//
// CREDIT: Teamwork of Mark Rejhon @BlurBusters & Timothy Lottes @NOTimothyLottes
// Gamma corrected CRT simulator in a shader using clever formula-by-scanline trick
// (easily can generate LUTs, for other workflows like FPGAs or Javascript)
// - @NOTimothyLottes provided the algorithm for per-pixel BFI (Variable MPRT, higher MPRT for bright pixels)
// - @BlurBusters provided the algorithm for the CRT electron beam (2022, publicly released for first time)
//
// Contact Blur Busters for help integrating this in your product (emulator, fpga, filter, display firmware, video processor)
//
// This new algorithm has multiple breakthroughs:
//
// - Seamless; no banding*!  (*Monitor/OS configuration: SDR=on, HDR=off, ABL=off, APL=off, gamma=2.4)
// - Phosphor fadebehind simulation in rolling scan.
// - Works on LCDs and OLEDs.
// - Variable per-pixel MPRT. Spreads brighter pixels over more refresh cycles than dimmer pixels.
// - No image retention on LCDs or OLEDs.
// - No integer divisor requirement. Recommended but not necessary (e.g. 60fps 144Hz works!)
// - Gain adjustment (less motion blur at lower gain values, by trading off brightness)
// - Realtime (for retro & emulator uses) and slo-mo modes (educational)
// - Great for softer 60Hz motion blur reduction, less eyestrain than classic 60Hz BFI/strobe.
// - Algorithm can be ported to shader and/or emulator and/or FPGA and/or display firmware.
//
// For best real time CRT realism:
//
// - Reasonably fast performing GPU (many integrated GPUs are unable to keep up)
// - Fastest GtG pixel response (A settings-modified OLED looks good with this algorithm)
// - As much Hz per CRT Hz! (960Hz better than 480Hz better than 240Hz)
// - Integer divisors are still better (just not mandatory)
// - Brightest SDR display with linear response (no ABL, no APL), as HDR boost adds banding
//     (unless you can modify the firmware to make it linear brightness during a rolling scan)
//
// *** IMPORTANT ***
// *** DISPLAY REQUIREMENTS ***
//
// - Best for gaming LCD or OLED monitors with fast pixel response.
// - More Hz per simulated CRT Hz is better (240Hz, 480Hz simulates 60Hz tubes more accurately than 120Hz).
// - OLED (SDR mode) looks better than LCD, but still works on LCD
// - May have minor banding with very slow GtG, asymmetric-GtG (VA LCDs), or excessively-overdriven.
// - Designed for sample & hold displays with excess refresh rate (LCDs and OLEDs);
//     Not intended for use with strobed or impulsed displays. Please turn off your displays' BFI/strobing.
//     This is because we need 100% software control of the flicker algorithm to simulate a CRT beam.
//
// SDR MODE RECOMMENDED FOR NOW (Due to predictable gamma compensation math)
//
// - Best results occur on display configured to standard SDR gamma curve and ABL/APL disabled to go 100% bandfree
// - Please set your display gamma to 2.2 or 2.4, turn off ABL/APL in display settings, and set your OLED to SDR mode.  
// - Will NOT work well with some FALD and MiniLED due to backlight lagbehind effects.
// - Need future API access to OLED ABL/ABL algorithm to compensate for OLED ABL/APL windowing interference with algorithm.
// - This code is heavily commented because of the complexity of the algorithm.
//
/*********************************************************************************************************************/
//
// MIT License
// 
// Copyright 2024 Mark Rejhon (@BlurBusters) & Timothy Lottes (@NOTimothyLottes)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
/*********************************************************************************************************************/

//-------------------------------------------------------------------------------------------------
// Utility Macros

#define clampPixel(a) clamp(a, float3(0.0), float3(1.0))

// Selection Function: Returns 'b' if 'p' is true, else 'a'.
float SelF1(float a, float b, bool p)
{
    return p ? b : a;
}

float mod(float x, float y)
{
    return x - y * floor(x / y);
}

//-------------------------------------------------------------------------------------------------
// sRGB Encoding and Decoding Functions, to gamma correct/uncorrect

// Encode linear color to sRGB. (applies gamma curve)
float linear2srgb(float c)
{
    float3 j = float3(0.0031308 * 12.92, 12.92, 1.0 / GAMMA);
    float2 k = float2(1.055, -0.055);
    return clamp(j.x, c * j.y, pow(c, j.z) * k.x + k.y);
}
float3 linear2srgb(float3 c)
{
#if HARDWARE_SRGB == 1
    return c;
#else
    return float3(linear2srgb(c.r), linear2srgb(c.g), linear2srgb(c.b));
#endif    
}

// Decode sRGB color to linear. (undoes gamma curve)
float srgb2linear(float c)
{
    float3 j = float3(0.04045, 1.0 / 12.92, GAMMA);
    float2 k = float2(1.0 / 1.055, 0.055 / 1.055);
    return SelF1(c * j.y, pow(c * k.x + k.y, j.z), c > j.x);
}
float3 srgb2linear(float3 c)
{
#if HARDWARE_SRGB == 1
    return c;
#else
    return float3(srgb2linear(c.r), srgb2linear(c.g), srgb2linear(c.b));
#endif    
}

//------------------------------------------------------------------------------------------------
// Gets pixel from the unprocessed framebuffer.
//
// Placeholder for accessing the 3 trailing unprocessed frames (for simulating CRT on)
//   - Frame counter represents simulated CRT refresh cycle number.
//   - Always assign numbers to your refresh cycles. For reliability, keep a 3 frame trailing buffer.
//   - We index by frame counter because it is necessary for blending adjacent CRT refresh cycles, 
//      for the phosphor fade algorithm on old frame at bottom, and new frames at top.
//   - Framebuffer to retrieve from should be unscaled (e.g. original game resolution or emulator resolution).
//   - (If you do optional additional processing such as scaling+scanlines+masks, do it post-processing after this stage)
// DEMO version:
//   - We cheat by horizontally shifting shifted pixel reads from a texture.
// PRODUCTION version:
//   - Put your own code to retrieve a pixel from your series of unprocessed frame buffers.
//     IMPORTANT: For integration into firmware/software/emulators/games, this must be executed 
//     at refresh cycle granularity independently of your underlying games' framerate! 
//     There are three independent frequencies involved:
//     - Native Hz (your actual physical display)
//     - Simulated CRT Hz (Hz of simulated CRT tube)
//     - Underlying content frame rate (this shader doesn't need to know; TODO: Unless you plan to simulate VRR-CRT)
//
float3 getPixelFromOrigFrame(float2 uv, float getFromHzNumber, float currentHzCounter)
{
    // We'll offset uv.x by baseShift, and round-off to screen coordinates to avoid seam artifacts
    float age = currentHzCounter - getFromHzNumber;
    if (age < 1)
    {
        return iChannel0.SampleLevel(iChannel_sampler, uv, 0.0).rgb;
    }
    if (age < 2)
    {
        return iChannel1.SampleLevel(iChannel_sampler, uv, 0.0).rgb;
    }
    if (age < 3)
    {
        return iChannel2.SampleLevel(iChannel_sampler, uv, 0.0).rgb;
    }
    return float3(0.0, 0.0, 0.0);
}

//-------------------------------------------------------------------------------------------------
// CRT Rolling Scan Simulation With Phosphor Fade + Brightness Redistributor Algorithm
//
// New variable 'per-pixel MPRT' algorithm that mimics CRT phosphor decay too.
// - We emit as many photons as possible as early as possible, and if we can't emit it all (e.g. RGB 255)
//   then we continue emitting in the next refresh cycle until we've hit our target (gamma-compensated).
// - This is a clever trick to keep CRT simulation brighter but still benefit motion clarity of most colors.
//   Besides, real CRT tubes behave roughly similar too! (overexcited phosphor take longer to decay)
// - This also concurrently produces a phosphor-fade style behavior.
// - Win-win!
//
// Parameters:
// - c2: total brightness * framesPerHz per channel.
// - crtRasterPos: normalized raster position [0..1] representing current scan line
// - phaseOffset: fractional start of the brightness interval [0..1] (0.0 at top, 1.0 at bottom).
// - framesPerHz: Number of frames per Hz. (Does not have to be integer divisible!)
//
float3 getPixelFromSimulatedCRT(float2 uv, float crtRasterPos, float crtHzCounter, float framesPerHz, float tubePos)
{
    // Get pixels from three consecutive refresh cycles
    float3 pixelPrev2 = srgb2linear(getPixelFromOrigFrame(uv, crtHzCounter - 2.0, crtHzCounter));
    float3 pixelPrev1 = srgb2linear(getPixelFromOrigFrame(uv, crtHzCounter - 1.0, crtHzCounter));
    float3 pixelCurr = srgb2linear(getPixelFromOrigFrame(uv, crtHzCounter, crtHzCounter));

    float3 result = float3(0.0, 0.0, 0.0);

    // Compute "photon budgets" for all three cycles
    float brightnessScale = framesPerHz * GAIN_VS_BLUR;
    float3 colorPrev2 = pixelPrev2 * brightnessScale;
    float3 colorPrev1 = pixelPrev1 * brightnessScale;
    float3 colorCurr = pixelCurr * brightnessScale;

    // Process each color channel independently
    for (int ch = 0; ch < 3; ch++)
    {
        // Get brightness lengths for all three cycles
        float Lprev2 = colorPrev2[ch];
        float Lprev1 = colorPrev1[ch];
        float Lcurr = colorCurr[ch];
        
        if (Lprev2 <= 0.0 && Lprev1 <= 0.0 && Lcurr <= 0.0)
        {
            result[ch] = 0.0;
            continue;
        }
        
        // TODO: Optimize to use only 2 frames.
        // Unfortunately I need all 3 right now because if I only do 2,
        // I get artifacts at either top OR bottom edge (can't eliminate both)
        // What I may do is use a phase offset (e.g. input framebuffer chain
        // rotates forward in middle of emulated CRT Hz), as a workaround, and
        // see if that solves the problem and reduces the queue to 2.
        // (Will attempt later)

        // Convert normalized values to frame space
        float tubeFrame = tubePos * framesPerHz;
        float fStart = crtRasterPos * framesPerHz;
        float fEnd = fStart + 1.0;

        // Define intervals for all three trailing refresh cycles
        float startPrev2 = tubeFrame - framesPerHz;
        float endPrev2 = startPrev2 + Lprev2;

        float startPrev1 = tubeFrame;
        float endPrev1 = startPrev1 + Lprev1;

        float startCurr = tubeFrame + framesPerHz; // Fix seam for top edge
        float endCurr = startCurr + Lcurr;
        
        // Calculate overlaps for all three cycles
#define INTERVAL_OVERLAP(Astart, Aend, Bstart, Bend) max(0.0, min(Aend, Bend) - max(Astart, Bstart))
        float overlapPrev2 = INTERVAL_OVERLAP(startPrev2, endPrev2, fStart, fEnd);
        float overlapPrev1 = INTERVAL_OVERLAP(startPrev1, endPrev1, fStart, fEnd);
        float overlapCurr = INTERVAL_OVERLAP(startCurr, endCurr, fStart, fEnd);

        // Sum all overlaps for final brightness
        float temp = overlapPrev2 + overlapPrev1 + overlapCurr;
        if (ch == 0)
        {
            result.x = temp;
        }
        if (ch == 1)
        {
            result.y = temp;
        }
        if (ch == 2)
        {
            result.z = temp;
        }
    }

    return linear2srgb(result);
}

struct VSIn
{
    uint vertexId : SV_VertexID;
};
                                      
struct VSOut
{
    float2 vTexCoord : TEXCOORD0;
    float tubePos : TEXCOORD1;
    float4 pos : SV_Position;
};

struct PSIn
{
    float2 vTexCoord : TEXCOORD0;
    float tubePos : TEXCOORD1;
};

struct PSOut
{
    float4 FragColor : SV_Target0;
};

//-------------------------------------------------------------------------------------------------
// Main Image Function
//
PSOut PSmain(PSIn input)
{
    PSOut output;
    
    // uv: Normalized coordinates ranging from (0,0) at the bottom-left to (1,1) at the top-right.
    float2 uv = input.vTexCoord;

    //-----------------------------------------------------------------------------------------
    // Get CRT simulated version of pixel
    output.FragColor = float4(getPixelFromSimulatedCRT(uv, param_crtRasterPos, param_crtHzCounter, EFFECTIVE_FRAMES_PER_HZ, input.tubePos), 1.0);
    return output;
}

//-------------------------------------------------------------------------------------------------
// Vertex Shader
//
VSOut VSmain(VSIn input)
{
    VSOut output;
    
    float tubePos;
    float2 vTexCoord;

    // hardcoded triangle
    if (input.vertexId == 0)
    {
        output.pos = float4(0.0, 2.0, 0.0, 1.0);
        vTexCoord = float2(0.5, -0.5);
    }
    else if (input.vertexId == 2)
    {
        output.pos = float4(-3.0, -1.0, 0.0, 1.0);
        vTexCoord = float2(-1.0, 1.0);
    }
    else if (input.vertexId == 1)
    {
        output.pos = float4(3.0, -1.0, 0.0, 1.0);
        vTexCoord = float2(2.0, 1.0);
    }
    
    // precalculate tubePos
    if (int(param_scanDirection) == 1)
    {
        tubePos = vTexCoord.y;
    }
    else
    {
        if (int(param_scanDirection) == 2)
        {
            tubePos = 1.0f - vTexCoord.y;
        }
        else
        {
            if (int(param_scanDirection) == 3)
            {
                tubePos = vTexCoord.x;
            }
            else
            {
                if (int(param_scanDirection) == 4)
                {
                    tubePos = 1.0f - vTexCoord.x;
                }
                else
                {
                    tubePos = 0.0f;
                }
            }
        }
    }
    
    output.tubePos = tubePos;
    output.vTexCoord = vTexCoord;
    return output;
}

//-------------------------------------------------------------------------------------------------
// Credits Reminder:
// Please credit BLUR BUSTERS & TIMOTHY LOTTES if this algorithm is used in your project/product.
// Hundreds of hours of research was done on related work that led to this algorithm.
//-------------------------------------------------------------------------------------------------
