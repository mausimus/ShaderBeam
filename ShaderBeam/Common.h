/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

namespace ShaderBeam
{

#define SHADERBEAM_TITLE "ShaderBeam v0.2.1"

#define MIN_SUBFRAMES 1
#define MAX_SUBFRAMES 16
#define MAX_INPUTS 5
#define AUTOSYNC_INTERVAL 2

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

// The part of the output the shader applies to: a rectangle with rounded corners whose edges can
// curve like a CRT tube's. With u and v running from -1 to 1 across the rectangle, the left and
// right edges are where u * (1 + curvatureX * (u*u + v*v)) / (1 + curvatureX) = +-1, and the top
// and bottom edges likewise with v and curvatureY (crt-pi's barrel distortion). So the edges touch
// the rectangle at their middles and curve inwards towards the corners; 0 keeps them straight.
struct EffectArea
{
    float left { 0.0f }, top { 0.0f }, right { 1.0f }, bottom { 1.0f }; // fractions of the output
    float radius { 0.0f };                                             // fraction of the output height
    float curvatureX { 0.0f }, curvatureY { 0.0f };

    bool IsRectangle() const { return radius <= 0.0f && curvatureX <= 0.0f && curvatureY <= 0.0f; }
};

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
    unsigned                          no;
    std::string                       name;
    std::string                       hint;
    ParameterType                     type;
    const std::map<int, std::string>& dropdown;
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

struct InputInfo
{
    HWND        hwnd;
    std::string name;
    bool        fullscreen;
    unsigned    captureMethod;
    unsigned    captureDisplayNo;
};

class CaptureBase;

struct CaptureInfo
{
    unsigned                     no;
    std::string                  name;
    std::shared_ptr<CaptureBase> api;
};

struct BenchmarkResult
{
    float totalFPS { 0 };
    float copyFPS { 0 };
    float renderFPS { 0 };
    float presentFPS { 0 };
};

constexpr int MONITOR_LCD  = 0;
constexpr int MONITOR_OLED = 1;

class ShaderManager;

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
    bool hardwareSrgb { false };
    int  monitorType { MONITOR_LCD };
    bool autoSync { true };
    bool useHdr { false };
    int  maxQueuedFrames { 0 };
    bool rememberSettings { true };

    // Effect area (see EffectArea). The shader applies inside it; outside it the captured image is
    // shown unchanged, e.g. the bezel a CRT shader draws around its screen.
    float areaLeft { 0.0f };
    float areaTop { 0.0f };
    float areaRight { 1.0f };
    float areaBottom { 1.0f };
    float areaRadius { 0.0f };
    float areaCurvatureX { 0.0f };
    float areaCurvatureY { 0.0f };

    // internal options
    bool     exclusive { false };
    unsigned wgcBuffers { 16 };
    unsigned gpuThreadPriority { 29 };

    // derived
    HWND        outputWindow { 0 };
    HMONITOR    captureMonitor { 0 };
    HWND        captureWindow { 0 };
    unsigned    outputWidth { 0 };
    unsigned    outputHeight { 0 };
    float       vsyncDuration { 0 };
    float       vsyncRate { 0 };
    unsigned    swapChainBuffers { 0 };
    bool        crossAdapter { false };
    DXGI_FORMAT format { DXGI_FORMAT_B8G8R8A8_UNORM };

    void Save(const ShaderManager& shaderManager) const;
    void Load(ShaderManager& shaderManager);

    // The effect area in use.
    EffectArea GetEffectArea() const;
};
} // namespace ShaderBeam
