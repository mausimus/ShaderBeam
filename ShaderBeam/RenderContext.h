/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "Common.h"

namespace ShaderBeam
{

class RenderContextBase
{
public:
    RenderContextBase(const Options& options) : options(options) { }

    int              frameNo { 0 };
    int              subFrameNo { 0 };
    std::vector<int> inputSlots;

    const Options& options;

    // renderer
    virtual void Create(const AdapterInfo& adapter) = 0;
    virtual void BeginFrame()                       = 0;
    virtual void DrawSplitScreen()                  = 0;
    virtual void BeginUI()                          = 0;
    virtual void EndFrame()                         = 0;
    virtual void Present(bool vsync)                = 0;
    virtual void WaitTillIdle()                     = 0;
    virtual void CreateInputs(int inputsRequired)   = 0;
    virtual void Destroy()                          = 0;

    // ImGui
    virtual void ImGuiInit()     = 0;
    virtual void ImGuiNewFrame() = 0;
    virtual void ImGuiRender()   = 0;
    virtual void ImGuiShutdown() = 0;

    virtual void BlackFrame() const  = 0;
    virtual void Passthrough() const = 0;

    virtual void SubmitInput(int slotNo, void* data, unsigned size) = 0;
};

} // namespace ShaderBeam

#include "RenderContextD3D11.h"
