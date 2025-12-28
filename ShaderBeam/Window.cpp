/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"
#include "resource.h"

#include "ShaderBeam.h"

ShaderBeam::ShaderBeam s_shaderBeam;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst; // current instance
WCHAR     szTitle[MAX_LOADSTRING]; // The title bar text
WCHAR     szWindowClass[MAX_LOADSTRING]; // the main window class name

// Forward declarations of functions included in this code module:
ATOM             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    SetProcessDPIAware();

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SHADERBEAM, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if(!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SHADERBEAM));

    MSG msg;

    // Main message loop:
    while(GetMessage(&msg, nullptr, 0, 0))
    {
        if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = WndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SHADERBEAM));
    wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(0x00000000);
    wcex.lpszMenuName  = 0;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm       = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

static void BringToFront(HWND hWnd, bool activate)
{
    UINT flags = SWP_NOMOVE | SWP_NOSIZE;
    if(!activate)
    {
        flags |= SWP_NOACTIVATE;
    }
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, flags);
}

static void MoveToDisplay(HWND hWnd, HMONITOR monitor)
{
    RECT        clientRect;
    MONITORINFO info;
    info.cbSize = sizeof(info);
    GetMonitorInfo(monitor, &info);
    clientRect.top    = info.rcMonitor.top;
    clientRect.left   = info.rcMonitor.left;
    clientRect.right  = info.rcMonitor.right;
    clientRect.bottom = info.rcMonitor.bottom;
    AdjustWindowRect(&clientRect, GetWindowLong(hWnd, GWL_STYLE), GetMenu(hWnd) != 0);
    SetWindowPos(hWnd, HWND_TOPMOST, info.rcMonitor.left, info.rcMonitor.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, SWP_FRAMECHANGED);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_POPUP | WS_OVERLAPPEDWINDOW | WS_EX_WINDOWEDGE, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if(!hWnd)
    {
        return FALSE;
    }

    LONG cur_style = GetWindowLong(hWnd, GWL_STYLE);
    cur_style &= ~WS_OVERLAPPEDWINDOW;
    SetWindowLong(hWnd, GWL_STYLE, cur_style);
    SetWindowDisplayAffinity(hWnd, WDA_EXCLUDEFROMCAPTURE);

    s_shaderBeam.Create(hWnd);
    MoveToDisplay(hWnd, s_shaderBeam.m_ui.m_displays[s_shaderBeam.m_options.shaderDisplayNo].monitor);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    s_shaderBeam.Start();
    SetTimer(hWnd, 1, 1000, NULL);
    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if(message == WM_LBUTTONDOWN && !s_shaderBeam.m_ui.MouseRequired())
    {
        // clicking away of ImGui - hide UI
        PostMessage(hWnd, WM_HOTKEY, HOTKEY_TOGGLEUI, 1);
        return true;
    }
    if(s_shaderBeam.m_ui.Input(hWnd, message, wParam, lParam))
    {
        return true;
    }

    switch(message)
    {
    case WM_TIMER: {
        if(s_shaderBeam.m_options.ui)
            s_shaderBeam.UpdateVsyncRate();
        break;
    }
    case WM_USER_RESTART: {
        s_shaderBeam.Stop();
        MoveToDisplay(hWnd, s_shaderBeam.m_ui.m_displays[s_shaderBeam.m_options.shaderDisplayNo].monitor);
        s_shaderBeam.Start();
        break;
    }
    break;
    case WM_USER_BENCHMARK: {
        s_shaderBeam.RunBenchmark();
        break;
    }
    case WM_HOTKEY: {
        switch(wParam)
        {
        case HOTKEY_QUIT:
            PostMessage(hWnd, WM_DESTROY, 0, 0);
            break;
        case HOTKEY_BRINGTOFRONT:
            BringToFront(hWnd, false);
            break;
        case HOTKEY_RESTART:
            PostMessage(hWnd, WM_USER_RESTART, 0, 0);
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
            if(s_shaderBeam.m_ui.Toggle())
            {
                // UI visible
                LONG ex_style = GetWindowLong(hWnd, GWL_EXSTYLE);
                ex_style &= ~(WS_EX_LAYERED | WS_EX_TRANSPARENT);
                SetWindowLong(hWnd, GWL_EXSTYLE, ex_style);
                BringToFront(hWnd, true);
                SetForegroundWindow(hWnd);
            }
            else
            {
                // UI hidden
                LONG ex_style = GetWindowLong(hWnd, GWL_EXSTYLE);
                SetWindowLong(hWnd, GWL_EXSTYLE, ex_style | WS_EX_LAYERED | WS_EX_TRANSPARENT);
            }
            break;
        }
    }
    break;
    case WM_ERASEBKGND:
    case WM_SIZING:
        return 0;
    case WM_PAINT:
        ValidateRect(hWnd, NULL);
        return 0;
    case WM_DESTROY:
        s_shaderBeam.Stop();
        s_shaderBeam.Destroy();
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
