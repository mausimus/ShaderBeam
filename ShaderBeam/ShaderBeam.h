/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#include "Common.h"
#include "Renderer.h"
#include "ShaderManager.h"
#include "Watcher.h"
#include "RenderThread.h"

namespace ShaderBeam
{

class ShaderBeam
{
public:
    ShaderBeam();

    void Create(HWND hwnd, GLFWwindow* window);
    void Start();
    void Stop();
    void Destroy();

    void RunBenchmark();
    void UpdateVsyncRate();

    UI      m_ui;
    Options m_options;
    bool    m_active { false };

private:
    Watcher       m_watcher;
    Renderer      m_renderer;
    RenderThread  m_renderThread;
    ShaderManager m_shaderManager;

    std::vector<AdapterInfo> GetAdapters();
    std::vector<DisplayInfo> GetDisplays();
    void                     DefaultOptions();

    static std::vector<winrt::com_ptr<IDXGIAdapter2>> EnumerateAdapters();
};
} // namespace ShaderBeam