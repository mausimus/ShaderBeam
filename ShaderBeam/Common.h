/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

namespace ShaderBeam
{

#define SHADERBEAM_TITLE "ShaderBeam v0.2"

#define MIN_SUBFRAMES 1
#define MAX_SUBFRAMES 16
#define MAX_INPUTS 5

#define WM_USER_RESTART (WM_USER)
#define WM_USER_BENCHMARK (WM_USER + 1)
#define WM_USER_NOWINDOW (WM_USER + 2)

#define HOTKEY_TOGGLEUI 0
#define HOTKEY_BRINGTOFRONT 1
#define HOTKEY_STOPSTART 2
#define HOTKEY_RESTART 3
#define HOTKEY_QUIT 4

#define HOTKEY_TOGGLEUI_KEY 'B'
#define HOTKEY_BRINGTOFRONT_KEY 'G'
#define HOTKEY_STOPSTART_KEY 'S'
#define HOTKEY_RESTART_KEY 'A'
#define HOTKEY_QUIT_KEY 'Q'

//#define RGB_TEST

struct AdapterInfo
{
    unsigned                     no;
    std::string                  name;
    winrt::com_ptr<IDXGIAdapter> adapter;
    LUID                         luid;
};

struct DisplayInfo
{
    unsigned    no;
    unsigned    width;
    unsigned    height;
    std::string name;
    HMONITOR    monitor;
};

struct ShaderInfo
{
    unsigned    no;
    std::string name;
};

enum ParameterType
{
    ParamFloat,
    ParamInt
};

struct ParameterInfo
{
    unsigned      no;
    std::string   name;
    std::string   hint;
    ParameterType type;
    union
    {
        struct
        {
            float* value;
            float  min;
            float  max;
            float  def;
        } fp;
        struct
        {
            int* value;
            int  min;
            int  max;
            int  def;
        } ip;
    } p;
};

struct WindowInfo
{
    HWND        hwnd;
    std::string name;
    bool        fullscreen;
};

class CaptureBase;

struct CaptureInfo
{
    unsigned                     no;
    std::string                  name;
    std::shared_ptr<CaptureBase> api;
};

constexpr int MONITOR_LCD  = 0;
constexpr int MONITOR_OLED = 1;

struct Options
{
    // UI options
    bool ui { true };
    bool banner { true };
    bool charts { false };
    int  shaderProfileNo { 0 };
    int  captureAdapterNo { 0 };
    int  shaderAdapterNo { 0 };
    int  captureDisplayNo { 0 };
    int  shaderDisplayNo { 0 };
    int  subFrames { 0 };
    int  captureMethod { 0 };
    int  splitScreen { 0 };
    int  hardwareSrgb { 0 };
    int  monitorType { MONITOR_LCD };
    int  autoSyncInterval { 2 };

    // internal options
    bool     exclusive { false };
    unsigned swapChainBuffers { 3 };
    unsigned maxQueuedFrames { 0 };
    unsigned wgcBuffers { 16 };
    unsigned gpuThreadPriority { 29 };

    // derived
    HWND     outputWindow { 0 };
    HMONITOR captureMonitor { 0 };
    HWND     captureWindow { 0 };
    unsigned outputWidth { 0 };
    unsigned outputHeight { 0 };
    float    vsyncDuration { 0 };
    float    vsyncRate { 0 };
    bool     crossAdapter { false };
};
}; // namespace ShaderBeam
