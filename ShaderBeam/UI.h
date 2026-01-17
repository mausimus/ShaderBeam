/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "Common.h"
#include "ShaderManager.h"

struct ImFont;
struct ImGuiStyle;
class RenderContext;

namespace ShaderBeam
{

class UI
{
public:
    UI(Options& options, ShaderManager& shaderManager, RenderContext& renderContext);

    void Start(HWND window, float scale);
    bool Input(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    bool MouseRequired();
    void Render();
    void Stop();

    void SetError(const char* message);
    void SetBenchmark(const BenchmarkResult& result);
    void AddWindow(HWND hWnd);
    bool RenderRequired() const;
    bool Toggle();

    float m_inputFPS { 0 };
    float m_outputFPS { 0 };
    float m_captureLag { 0 };

    std::vector<AdapterInfo> m_adapters;
    std::vector<DisplayInfo> m_displays;
    std::vector<ShaderInfo>  m_shaders;
    std::vector<CaptureInfo> m_captures;
    std::vector<std::string> m_monitorTypes;
    std::vector<InputInfo>   m_inputs;
    std::vector<std::string> m_queuedFrames;
    std::vector<std::string> m_splitScreens;

private:
    void SetStyle(ImGuiStyle& style);
    void RenderBanner();
    void SetApplyRequired();
    void ApplyPendingChanges();
    void ClearPendingChanges();
    void ScanWindows();

    struct
    {
        int  captureAdapterNo;
        int  shaderAdapterNo;
        int  captureDisplayNo;
        HWND captureWindow;
        int  shaderDisplayNo;
        int  subFrames;
        bool hardwareSrgb;
        int  captureMethod;
        int  splitScreen;
        int  monitorType;
        bool autoSync;
        int  maxQueuedFrames;
        bool useHdr;
    } m_pending;

    ImFont*         m_smallFont;
    ImFont*         m_largeFont;
    Options&        m_options;
    ShaderManager&  m_shaderManager;
    RenderContext&  m_renderContext;
    bool            m_ready { false };
    bool            m_applyRequired { false };
    float           m_fontSize { 0 };
    const char*     m_captureGPUs { nullptr };
    const char*     m_renderGPUs { nullptr };
    BenchmarkResult m_benchmarkResult {};
    bool            m_hasBenchmark { false };
    std::string     m_captureWindowName;
    bool            m_hasError { false };
    std::string     m_errorMessage;
};
} // namespace ShaderBeam