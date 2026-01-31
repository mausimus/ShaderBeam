/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "ShaderBeam.h"

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

namespace ShaderBeam
{

ShaderBeam  s_shaderBeam;
GLFWwindow* s_window;
HWND        s_hwnd;
std::mutex  s_mutex;

void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

struct GLFWMessage
{
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
};

std::deque<GLFWMessage> _messages;

void AppMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    {
        std::unique_lock lock(s_mutex);
        _messages.emplace_back(message, wParam, lParam);
    }
    glfwPostEmptyEvent();
}

void ErrorMessage(const char* msg)
{
    #ifdef _WIN32
    MessageBoxA(s_hwnd, msg, SHADERBEAM_TITLE, MB_ICONERROR | MB_OK);
    #else
    error_callback(1, msg);
    #endif
}

static void BringToFront(bool activate)
{
    int flags = GLFW_FLOATING;
    if(activate)
    {
        flags |= GLFW_FOCUSED;
    }
    glfwSetWindowAttrib(s_window, flags, GLFW_TRUE);
}

void ToggleUI()
{
    if(s_shaderBeam.m_ui.Toggle())
    {
        // UI visible
        glfwSetWindowAttrib(s_window, GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);
        BringToFront(true);
    }
    else
    {
        // UI hidden
        glfwSetWindowAttrib(s_window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
    }
}

void Start()
{
    #ifdef _WIN32
    timeBeginPeriod(1);
    Helpers::InitQPC();

    RegisterHotKey(s_hwnd, HOTKEY_TOGGLEUI, MOD_CONTROL | MOD_SHIFT, HOTKEY_TOGGLEUI_KEY);
    RegisterHotKey(s_hwnd, HOTKEY_BRINGTOFRONT, MOD_CONTROL | MOD_SHIFT, HOTKEY_BRINGTOFRONT_KEY);
    //RegisterHotKey(m_options.outputWindow, HOTKEY_STOPSTART, MOD_CONTROL | MOD_SHIFT, HOTKEY_STOPSTART_KEY);
    RegisterHotKey(s_hwnd, HOTKEY_RESTART, MOD_CONTROL | MOD_SHIFT, HOTKEY_RESTART_KEY);
    RegisterHotKey(s_hwnd, HOTKEY_QUIT, MOD_CONTROL | MOD_SHIFT, HOTKEY_QUIT_KEY);

    SetTimer(s_hwnd, 1, 1000, NULL);
    #endif
}

void Stop()
{
    #ifdef _WIN32
    UnregisterHotKey(s_hwnd, HOTKEY_TOGGLEUI);
    UnregisterHotKey(s_hwnd, HOTKEY_BRINGTOFRONT);
    //UnregisterHotKey(m_options.outputWindow, HOTKEY_STOPSTART);
    UnregisterHotKey(s_hwnd, HOTKEY_RESTART);
    UnregisterHotKey(s_hwnd, HOTKEY_QUIT);

    timeEndPeriod(1);

    KillTimer(s_hwnd, 1);
    #endif
}

void MoveToDisplay(int no)
{
    int  count;
    auto monitors = glfwGetMonitors(&count);

    if(count <= no)
        return;

    // assuming order is the same as in Win32....
    const GLFWvidmode* mode = glfwGetVideoMode(monitors[no]);

    int xpos, ypos;
    glfwGetMonitorPos(monitors[no], &xpos, &ypos);
    glfwSetWindowPos(s_window, xpos, ypos);
    glfwSetWindowSize(s_window, mode->width, mode->height);

    float xscale, yscale;
    glfwGetWindowContentScale(s_window, &xscale, &yscale);
    s_shaderBeam.m_options.scale = xscale;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !s_shaderBeam.m_ui.MouseRequired())
    {
        ToggleUI();
    }
}

bool ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_TIMER: {
        if(s_shaderBeam.m_options.ui)
            s_shaderBeam.UpdateVsyncRate();
        break;
    }
    case WM_USER_RESTART: {
        s_shaderBeam.Stop();
        MoveToDisplay(s_shaderBeam.m_ui.m_displays[s_shaderBeam.m_options.shaderDisplayNo].no);
        s_shaderBeam.Start();
        break;
    }
    break;
    case WM_USER_BENCHMARK: {
        s_shaderBeam.RunBenchmark();
        break;
    }
    case WM_USER_NOWINDOW: {
        s_shaderBeam.m_options.captureWindow = NULL;
        AppMessage(WM_USER_RESTART, 0, 0);
        break;
    }
    case WM_USER_QUIT: {
        glfwSetWindowShouldClose(s_window, 1);
        break;
    }
    case WM_HOTKEY: {
        switch(wParam)
        {
        case HOTKEY_QUIT:
            AppMessage(WM_USER_QUIT, 0, 0);
            break;
        case HOTKEY_BRINGTOFRONT:
            BringToFront(false);
            break;
        case HOTKEY_RESTART:
            AppMessage(WM_USER_RESTART, 0, 0);
            break;
        case HOTKEY_STOPSTART:
            if(s_shaderBeam.m_active)
            {
                s_shaderBeam.Stop();
            }
            else
            {
                s_shaderBeam.Start();
            }
            break;
        case HOTKEY_TOGGLEUI:
            ToggleUI();
            break;
        }
    }
    break;
    default:
        return false;
    }
    return true;
}

#ifdef _WIN32
WNDPROC OriginalWndProc = NULL;

LRESULT CALLBACK CustomWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(ProcessMessage(message, wParam, lParam))
    {
        return 0;
    }
    return CallWindowProc(OriginalWndProc, hWnd, message, wParam, lParam);
}
#endif

void Run()
{
    #ifdef _WIN32
    SetProcessDPIAware();
    #endif

    if(!glfwInit())
    {
        // Initialization failed
        abort();
    }
    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_FALSE);

    s_window = glfwCreateWindow(640, 480, "ShaderBeam", NULL, NULL);
    if(!s_window)
    {
        // Window or OpenGL context creation failed
        abort();
    }

    glfwSetMouseButtonCallback(s_window, mouse_button_callback);

    #ifdef _WIN32
    s_hwnd          = glfwGetWin32Window(s_window);
    OriginalWndProc = (WNDPROC)GetWindowLongPtr(s_hwnd, GWLP_WNDPROC);
    if(OriginalWndProc == NULL)
    {
        abort();
    }
    SetWindowLongPtr(s_hwnd, GWLP_WNDPROC, (LONG_PTR)CustomWndProc);
    #else
    s_hwnd = (HWND)s_window;
    #endif

    s_shaderBeam.Create(s_hwnd, s_window);

    MoveToDisplay(s_shaderBeam.m_ui.m_displays[s_shaderBeam.m_options.shaderDisplayNo].no);

    #ifdef _WIN32
    SetWindowDisplayAffinity(s_hwnd, WDA_EXCLUDEFROMCAPTURE);
#endif

    BringToFront(true);

    s_shaderBeam.Start();
    Start();

    while(!glfwWindowShouldClose(s_window))
    {
        std::vector<GLFWMessage> msgs;
        {
            std::unique_lock lock(s_mutex);
            msgs.insert(msgs.begin(), _messages.begin(), _messages.end());
            _messages.clear();
        }
        for(const auto& msg : msgs)
        {
            ProcessMessage(msg.message, msg.lParam, msg.wParam);
        }

        glfwWaitEvents();
    }

    s_shaderBeam.Stop();
    s_shaderBeam.Destroy();
    Stop();

    glfwDestroyWindow(s_window);
    glfwTerminate();
}

} // namespace ShaderBeam

#ifdef _WIN32
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    ShaderBeam::Run();
    return 0;
}
#else
int main()
{
    ShaderBeam::Run();
    return 0;
}
#endif