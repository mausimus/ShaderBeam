/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "UI.h"
#include "Helpers.h"

#include "imgui.h"
#include "backends\imgui_impl_win32.h"
#include "backends\imgui_impl_dx11.h"

#include "ProggyVectorRegular.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

const char* SHADERBEAM_ABOUT = "                " SHADERBEAM_TITLE "\n"
                               "               (c) 2025 mausimus\n"
                               "     https://github.com/mausimus/ShaderBeam\n"
                               "         Distributed under MIT License\n"
                               "\n"
                               "    Includes Blur Busters CRT Beam Simulator\n"
                               "        by Mark Rejhon @BlurBusters and\n"
                               "        Timothy Lottes @NOTimothyLottes\n"
                               "          https://blurbusters.com/crt\n"
                               "\n"
                               " See GitHub page for usage and tweaking guides!\n\n";

char SHADERBEAM_MESSAGE[] = "Press Ctrl+Shift+? for UI";

char SHADERBEAM_HOTKEYS[] = "Global hotkeys:\n\n"
                            "  Toggle UI    - Ctrl+Shift+?\n"
                            "  Force on top - Ctrl+Shift+?\n"
                            "  Restart      - Ctrl+Shift+?\n"
                            "  Quit         - Ctrl+Shift+?\n\n";

namespace ShaderBeam
{

UI::UI(Options& options, ShaderManager& shaderManager) : m_options(options), m_shaderManager(shaderManager) { }

void UI::Start(HWND window, float scale, winrt::com_ptr<ID3D11Device> device, winrt::com_ptr<ID3D11DeviceContext> context)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(device.get(), context.get());

    ImGuiStyle& style      = ImGui::GetStyle();
    style.Alpha            = 1;
    style.DisabledAlpha    = 1;
    style.FrameRounding    = 0;
    style.FrameBorderSize  = 0;
    style.WindowBorderSize = 4;
    style.WindowRounding   = 0;
    SetStyle(style);
    style.ScaleAllSizes(scale);

    m_fontSize = roundf(16.0f * scale);

    ImGuiIO& io = ImGui::GetIO();
    //io.IniFilename = NULL;

    m_smallFont = io.Fonts->AddFontFromMemoryCompressedBase85TTF(ProggyVectorRegular_compressed_data_base85, m_fontSize);
    m_largeFont = io.Fonts->AddFontFromMemoryCompressedBase85TTF(ProggyVectorRegular_compressed_data_base85, m_fontSize * 3);
    io.Fonts->Build();

    if(strlen(SHADERBEAM_MESSAGE) < 18)
        abort();
    SHADERBEAM_MESSAGE[17] = HOTKEY_TOGGLEUI_KEY;

    if(strlen(SHADERBEAM_HOTKEYS) < 45 + 91)
        abort();
    SHADERBEAM_HOTKEYS[45]      = HOTKEY_TOGGLEUI_KEY;
    SHADERBEAM_HOTKEYS[45 + 30] = HOTKEY_BRINGTOFRONT_KEY;
    SHADERBEAM_HOTKEYS[45 + 60] = HOTKEY_RESTART_KEY;
    SHADERBEAM_HOTKEYS[45 + 90] = HOTKEY_QUIT_KEY;

    ClearPendingChanges();
    m_hasError     = false;
    m_hasBenchmark = false;
    m_errorMessage.clear();
    m_ready = true;
}

bool UI::MouseRequired()
{
    return m_ready && m_options.ui && ImGui::GetIO().WantCaptureMouse;
}

bool UI::Input(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return m_ready && m_options.ui && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}

bool UI::RenderRequired() const
{
    return m_ready && (m_options.ui || m_options.banner);
}

static void ShowHelpMarker(const char* desc)
{
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if(ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(450.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static void ShowAlertMarker(const char* desc)
{
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.0f, 1.0f), "(!)");
    if(ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(450.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
void UI::Render()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    if(m_options.ui)
    {
        auto viewport = ImGui::GetMainViewport()->Size;

        ImVec2 mainWindowSize(36 * m_fontSize, 41 * m_fontSize);
        ImVec2 mainWindowPos(2 * m_fontSize, (viewport.y - mainWindowSize.y) / 2.0f);
        if(mainWindowPos.y < 0)
            mainWindowPos.y = 0;
        ImGui::SetNextWindowSize(mainWindowSize, ImGuiCond_Always);
        ImGui::SetNextWindowPos(mainWindowPos, ImGuiCond_FirstUseEver);
        if(ImGui::Begin("ShaderBeam"))
        {
            ImVec2 popupSize = ImVec2(m_fontSize * 24, m_fontSize * 15);
            ImGui::SetNextWindowSize(popupSize, ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2((viewport.x - popupSize.x) / 2, (viewport.y - popupSize.y) / 2), ImGuiCond_Always);
            if(ImGui::Button("About"))
            {
                ImGui::OpenPopup("About");
            }
            if(ImGui::BeginPopup("About"))
            {
                ImGui::Text(SHADERBEAM_ABOUT);
                if(ImGui::Button("Close"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine();

            ImGui::SetNextWindowSize(popupSize, ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2((viewport.x - popupSize.x) / 2, (viewport.y - popupSize.y) / 2), ImGuiCond_Always);
            ImGui::SameLine();
            if(ImGui::Button("Hotkeys"))
            {
                ImGui::OpenPopup("hotkeys");
            }
            if(ImGui::BeginPopup("hotkeys"))
            {
                ImGui::Text(SHADERBEAM_HOTKEYS);
                if(ImGui::Button("Close"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::SameLine();
            if(ImGui::Button("Benchmark"))
                PostMessage(m_options.outputWindow, WM_USER_BENCHMARK, 0, 0);

            ImGui::SameLine();
            if(ImGui::Button("Restart"))
                PostMessage(m_options.outputWindow, WM_USER_RESTART, 0, 0);

            ImGui::SameLine();
            if(ImGui::Button("Quit"))
                PostMessage(m_options.outputWindow, WM_DESTROY, 0, 0);

            ImGui::SameLine();
            ImGui::Checkbox("Show Banner", &m_options.banner);
            ShowHelpMarker("Helps you confirm if ShaderBeam is in front of the game. Press 'Force on top' hotkey when it isn't.");

            ////////////////////////////////////////////////

            ImGui::SeparatorText("");

            ImGui::Text(" Display Hz: %7.02f", m_options.vsyncRate);
            if(m_options.vsyncRate < 90)
                ShowAlertMarker("Monitor refresh rate is too low! Run ShaderBeam on a high-refresh display, 240 Hz+ recommended!");
            else
                ShowHelpMarker("As reported by DWM for composition timing, may oscillate slightly.");
            ImGui::SameLine();
            ImGui::Text("                    Rendered FPS: %7.02f", m_outputFPS);
            if(m_outputFPS < m_options.vsyncRate * 0.95f)
                ShowAlertMarker("Your Shader GPU is not keeping up! Try lowering resolution or refresh rate.");
            else
                ShowHelpMarker("Frames sent for presentation. Should be same as Display Hz - if it isn't, your Shader GPU isn't keeping up.");

            ImGui::Text("Content FPS: %7.02f", m_options.vsyncRate / m_options.subFrames);
            ShowHelpMarker("Frame-limit your game to this (or slightly lower) value, including decimals, using RTSS.");
            ImGui::SameLine();
            ImGui::Text("                    Captured FPS: %7.02f", m_inputFPS);
            ShowHelpMarker("Provided by capture API.\nCan vary, ideally should be same as Content FPS (if it's higher, frame-limit your game to Content FPS using RTSS).");

            ImGui::Text("Capture Lag: %7.02f ms", m_captureLag);
            ShowHelpMarker("Time reported by Windows Capture.");

            ImGui::SeparatorText("Render Parameters");

            if(ImGui::InputInt("SubFrames", &m_pending.subFrames))
            {
                if(m_pending.subFrames < MIN_SUBFRAMES)
                    m_pending.subFrames = MIN_SUBFRAMES;
                if(m_pending.subFrames > MAX_SUBFRAMES)
                    m_pending.subFrames = MAX_SUBFRAMES;
                SetApplyRequired();
            }
            if(m_pending.subFrames == 1)
                ShowAlertMarker("The effects won't be visible with just one subframe!");
            else
                ShowHelpMarker("Number of display frames per content frame, determines Content FPS. Defaults to Display Hz / 60 and you normally don't need to change it.");

            if(ImGui::BeginCombo("Monitor Type", m_monitorTypes[m_pending.monitorType].c_str(), 0))
            {
                for(int m = 0; m < m_monitorTypes.size(); m++)
                {
                    auto selected = m == m_pending.monitorType;
                    if(ImGui::Selectable(m_monitorTypes[m].c_str(), selected))
                    {
                        m_pending.monitorType = m;
                        SetApplyRequired();
                    }
                    if(selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            if(ImGui::BeginCombo("Shader Display", m_displays[m_pending.shaderDisplayNo].name.c_str(), 0))
            {
                for(const auto& display : m_displays)
                {
                    auto selected = display.no == m_pending.shaderDisplayNo;
                    if(ImGui::Selectable(display.name.c_str(), selected))
                    {
                        m_pending.shaderDisplayNo = display.no;
                        SetApplyRequired();
                    }
                    if(selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ShowHelpMarker("Display to output to (your high refresh one).");

            if(ImGui::BeginCombo("Capture Display", m_displays[m_pending.captureDisplayNo].name.c_str(), 0))
            {
                for(const auto& display : m_displays)
                {
                    auto selected = display.no == m_pending.captureDisplayNo;
                    if(ImGui::Selectable(display.name.c_str(), selected))
                    {
                        m_pending.captureDisplayNo = display.no;
                        SetApplyRequired();
                    }
                    if(selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ShowHelpMarker("Display to capture (usually same as Shader Display).");

            if(ImGui::BeginCombo("Capture Window", m_windows[m_pending.captureWindowNo].name.c_str(), 0))
            {
                for(const auto& window : m_windows)
                {
                    auto selected = window.no == m_pending.captureWindowNo;
                    if(ImGui::Selectable(window.name.c_str(), selected))
                    {
                        m_pending.captureWindowNo = window.no;
                        SetApplyRequired();
                    }
                    if(selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ShowHelpMarker("Window to capture.");

            /*if(!m_pending.captureWindowNo)
                ImGui::BeginDisabled();*/
            ImGui::SliderInt("Auto-sync interval", &m_options.autoSyncInterval, 0, 30);
            /*if(!m_pending.captureWindowNo)
                ImGui::EndDisabled();*/
            ShowHelpMarker("Interval (in seconds) to attempt to re-sync capture to content-limited input, reduces latency.");

            if(ImGui::BeginCombo("Shader GPU", m_adapters[m_pending.shaderAdapterNo].name.c_str(), 0))
            {
                for(const auto& adapter : m_adapters)
                {
                    auto selected = adapter.no == m_pending.shaderAdapterNo;
                    if(ImGui::Selectable(adapter.name.c_str(), selected))
                    {
                        m_pending.shaderAdapterNo = adapter.no;
                        SetApplyRequired();
                    }
                    if(selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ShowHelpMarker("GPU used for rendering. If you have another dGPU or iGPU, try using it here for best experience.\nSometimes DXGI splits physical cards into multiple "
                           "virutal ones so duplicates are possible.");

            if(ImGui::BeginCombo("Capture GPU", m_adapters[m_pending.captureAdapterNo].name.c_str(), 0))
            {
                for(const auto& adapter : m_adapters)
                {
                    auto selected = adapter.no == m_pending.captureAdapterNo;
                    if(ImGui::Selectable(adapter.name.c_str(), selected))
                    {
                        m_pending.captureAdapterNo = adapter.no;
                        SetApplyRequired();
                    }
                    if(selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ShowHelpMarker(
                "GPU used for capture. This should be your primary/gaming GPU.\nSometimes DXGI splits physical cards into multiple virutal ones so duplicates are possible.");

            if(ImGui::SliderInt("Hardware sRGB", &m_pending.hardwareSrgb, 0, 1))
            {
                SetApplyRequired();
            }
            ShowHelpMarker("Reduces shader cost up to ~30% but with some quality loss and inability to adjust gamma (fixed at ~2.2). For slow iGPUs only.");

            if(ImGui::BeginCombo("Capture API", m_captures[m_pending.captureMethod].name.c_str(), 0))
            {
                for(const auto& capture : m_captures)
                {
                    auto selected = capture.no == m_pending.captureMethod;
                    if(ImGui::Selectable(capture.name.c_str(), selected))
                    {
                        m_pending.captureMethod = capture.no;
                        SetApplyRequired();
                    }
                    if(selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ShowHelpMarker("Windows Graphics Capture is recommended, but you can also try Desktop Duplication (limited support).");

            if(ImGui::SliderInt("Split-screen", &m_pending.splitScreen, 0, 2))
            {
                SetApplyRequired();
            }
            ShowHelpMarker("Enable split-screen mode (0: off, 1: vertical, 2: horizontal).");

            bool applyRequired = m_applyRequired;
            if(applyRequired)
            {
                ImVec4 color = ImVec4(0.7f, 0.4f, 0.1f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, color);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);
            }
            if(ImGui::Button("Apply Changes"))
            {
                ApplyPendingChanges();
                PostMessage(m_options.outputWindow, WM_USER_RESTART, 0, 0);
            }
            if(applyRequired)
            {
                ImGui::PopStyleColor(3);
            }

            //////////////////////////////////////////

            ImGui::SeparatorText("Shader");

            if(ImGui::BeginCombo("Type", m_shaders[m_options.shaderProfileNo].name.c_str(), 0))
            {
                for(const auto& shader : m_shaders)
                {
                    auto selected = shader.no == m_options.shaderProfileNo;
                    if(ImGui::Selectable(shader.name.c_str(), selected))
                    {
                        if(shader.no != m_options.shaderProfileNo)
                        {
                            m_options.shaderProfileNo = shader.no;
                            PostMessage(m_options.outputWindow, WM_USER_RESTART, 0, 0);
                        }
                    }
                    if(selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ShowHelpMarker("Shader effect to use.");

            ImGui::PushItemWidth(m_fontSize * 16);
            for(const auto& p : m_shaderManager.GetParameterInfos())
            {
                switch(p.type)
                {
                case ParameterType::ParamFloat:
                    ImGui::SliderFloat(p.name.c_str(), p.p.fp.value, p.p.fp.min, p.p.fp.max);
                    break;
                case ParameterType::ParamInt:
                    ImGui::SliderInt(p.name.c_str(), p.p.ip.value, p.p.ip.min, p.p.ip.max);
                    break;
                default:
                    abort();
                }
                ShowHelpMarker(p.hint.c_str());
            }
            ImGui::PopItemWidth();

            if(m_shaderManager.GetParameterInfos().size())
            {
                if(ImGui::Button("Reset parameters"))
                {
                    m_shaderManager.ResetDefaults();
                }
            }

            ////////////////////////////////////////////////////

            if(m_hasBenchmark)
            {
                m_hasBenchmark = false;
                ImGui::OpenPopup("Benchmark");
            }
            if(ImGui::BeginPopupModal("Benchmark"))
            {
                ImGui::Text("Shader GPU benchmark result:\n%.0f FPS", m_benchmarkFPS);
                if(ImGui::Button("Close"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if(m_hasError)
            {
                m_hasError = false;
                ImGui::OpenPopup("Error");
            }
            if(ImGui::BeginPopupModal("Error"))
            {
                ImGui::Text("FATAL ERROR");
                ImGui::Text(m_errorMessage.c_str());
                if(ImGui::Button("Close"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }
    if(m_options.banner)
    {
        RenderBanner();
    }
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void UI::Stop()
{
    m_ready = false;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    m_smallFont   = nullptr;
    m_largeFont   = nullptr;
    m_captureGPUs = nullptr;
    m_renderGPUs  = nullptr;
}

bool UI::Toggle()
{
    m_options.ui = !m_options.ui;
    return m_options.ui;
}

void UI::SetStyle(ImGuiStyle& style)
{
    style.Colors[ImGuiCol_Text]                  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.05f, 0.22f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.16f, 0.48f, 0.29f, 0.54f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.10f, 0.68f, 0.39f, 0.45f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.78f, 0.59f, 0.67f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.16f, 0.48f, 0.29f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.78f, 0.49f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.04f, 0.48f, 0.22f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.06f, 0.78f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.06f, 0.38f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.16f, 0.68f, 0.30f, 0.75f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.06f, 0.98f, 0.50f, 0.75f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.98f, 0.59f, 0.31f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.16f, 0.68f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.20f, 0.75f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.10f, 0.75f, 0.40f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.10f, 0.75f, 0.40f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.26f, 0.98f, 0.59f, 0.20f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.98f, 0.59f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.98f, 0.59f, 0.95f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.98f, 0.59f, 0.35f);
    style.Colors[ImGuiCol_UnsavedMarker]         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_NavCursor]             = ImVec4(0.26f, 0.98f, 0.59f, 1.00f);
    style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    style.Colors[ImGuiCol_Separator]             = ImVec4(0.10f, 0.30f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_InputTextCursor]       = style.Colors[ImGuiCol_Text];
    style.Colors[ImGuiCol_TextLink]              = style.Colors[ImGuiCol_HeaderActive];
}

void UI::RenderBanner()
{
    auto drawList = ImGui::GetForegroundDrawList();
    auto size     = ImGui::GetMainViewport()->Size;
    auto boxStart = ImVec2(m_fontSize, size.y - m_fontSize * 7);
    auto boxEnd   = ImVec2(m_fontSize, size.y - m_fontSize);
    boxEnd.x += m_largeFont->CalcTextSizeA(m_fontSize * 3, size.x, 0, "ShaderBeam").x + m_fontSize * 4;

    drawList->AddRectFilled(boxStart, boxEnd, IM_COL32(0, 0, 0, 255));
    boxStart.x += m_fontSize * 2;
    boxStart.y += m_fontSize;
    ImGui::PushFont(m_largeFont);
    drawList->AddText(boxStart, IM_COL32(64, 255, 128, 255), "ShaderBeam");
    ImGui::PopFont();
    boxStart.y += m_fontSize * 3;
    drawList->AddText(boxStart, IM_COL32(64, 255, 128, 255), SHADERBEAM_MESSAGE);
}

void UI::SetApplyRequired()
{
    m_applyRequired = true;
}

void UI::ClearPendingChanges()
{
    m_applyRequired            = false;
    m_pending.captureAdapterNo = m_options.captureAdapterNo;
    m_pending.shaderAdapterNo  = m_options.shaderAdapterNo;
    m_pending.captureDisplayNo = m_options.captureDisplayNo;
    m_pending.captureWindowNo  = m_options.captureWindowNo;
    m_pending.shaderDisplayNo  = m_options.shaderDisplayNo;
    m_pending.subFrames        = m_options.subFrames;
    m_pending.hardwareSrgb     = m_options.hardwareSrgb;
    m_pending.captureMethod    = m_options.captureMethod;
    m_pending.splitScreen      = m_options.splitScreen;
    m_pending.monitorType      = m_options.monitorType;
}

void UI::ApplyPendingChanges()
{
    m_options.captureAdapterNo = m_pending.captureAdapterNo;
    m_options.shaderAdapterNo  = m_pending.shaderAdapterNo;
    m_options.captureDisplayNo = m_pending.captureDisplayNo;
    m_options.captureWindowNo  = m_pending.captureWindowNo;
    m_options.shaderDisplayNo  = m_pending.shaderDisplayNo;
    m_options.subFrames        = m_pending.subFrames;
    m_options.hardwareSrgb     = m_pending.hardwareSrgb;
    m_options.captureMethod    = m_pending.captureMethod;
    m_options.splitScreen      = m_pending.splitScreen;
    m_options.monitorType      = m_pending.monitorType;
}

void UI::SetBenchmark(float fps)
{
    m_benchmarkFPS = fps;
    m_hasBenchmark = true;
}

void UI::SetError(const char* message)
{
    m_hasError     = true;
    m_errorMessage = message;
}

} // namespace ShaderBeam