/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#define THROW(hr, msg) Helpers::Throw(hr, msg)

#define TICKS_PER_SEC 1000.0f

namespace ShaderBeam
{

class Helpers
{
public:
    static winrt::com_ptr<ID3D11Device> CreateD3DDevice();
    static winrt::com_ptr<ID3D11Device> CreateD3DDevice(IDXGIAdapter* adapter);
    static void                         InitQPC();
    static float                        GetTicks();
    static ULONGLONG                    GetQPC();
    static void                         SpinWaitMs(float ms);
    static void                         SpinWaitQPC(unsigned qpc);
    static float                        QPCToTicks(ULONGLONG qpc);
    static float                        QPCToDeltaMs(ULONGLONG qpc);
    static void                         Throw(HRESULT hr, const char* action);
    static std::string                  WCharToString(const wchar_t* text);

private:
    static HRESULT CreateD3DDevice(D3D_DRIVER_TYPE const type, winrt::com_ptr<ID3D11Device>& device);
    static HRESULT CreateD3DDevice(IDXGIAdapter* adapter, D3D_DRIVER_TYPE const type, winrt::com_ptr<ID3D11Device>& device);
};
} // namespace ShaderBeam
