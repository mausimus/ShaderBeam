/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#include "stdafx.h"

#include "Helpers.h"

namespace ShaderBeam
{

HRESULT Helpers::CreateD3DDevice(D3D_DRIVER_TYPE const type, winrt::com_ptr<ID3D11Device>& device)
{
    WINRT_ASSERT(!device);

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    return D3D11CreateDevice(nullptr, type, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, device.put(), nullptr, nullptr);
}

HRESULT Helpers::CreateD3DDevice(IDXGIAdapter* adapter, D3D_DRIVER_TYPE const type, winrt::com_ptr<ID3D11Device>& device)
{
    WINRT_ASSERT(!device);

    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    return D3D11CreateDevice(adapter, type, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION, device.put(), nullptr, nullptr);
}

winrt::com_ptr<ID3D11Device> Helpers::CreateD3DDevice(IDXGIAdapter* adapter)
{
    if(adapter == nullptr)
        return CreateD3DDevice();

    winrt::com_ptr<ID3D11Device> device;
    HRESULT                      hr = CreateD3DDevice(adapter, adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, device);
    if(DXGI_ERROR_UNSUPPORTED == hr)
    {
        hr = CreateD3DDevice(nullptr, D3D_DRIVER_TYPE_WARP, device);
    }

    winrt::check_hresult(hr);
    return device;
}

winrt::com_ptr<ID3D11Device> Helpers::CreateD3DDevice()
{
    winrt::com_ptr<ID3D11Device> device;
    HRESULT                      hr = CreateD3DDevice(D3D_DRIVER_TYPE_HARDWARE, device);
    if(DXGI_ERROR_UNSUPPORTED == hr)
    {
        hr = CreateD3DDevice(D3D_DRIVER_TYPE_WARP, device);
    }

    winrt::check_hresult(hr);
    return device;
}

static LARGE_INTEGER freq { .QuadPart = 0 };
static LARGE_INTEGER startTicks { .QuadPart = 0 };

void Helpers::InitQPC()
{
    #ifdef _WIN32
    QueryPerformanceFrequency(&freq);
    if(!QueryPerformanceCounter(&startTicks))
    {
        throw std::runtime_error("Unable to query performance counter");
    }
    #endif
}

float Helpers::GetTicks()
{
    #ifdef _WIN32
    LARGE_INTEGER ticks;
    if(!QueryPerformanceCounter(&ticks) || freq.QuadPart == 0)
    {
        throw std::runtime_error("Unable to query performance counter");
    }

    return (float)((ticks.QuadPart - startTicks.QuadPart) / (freq.QuadPart / TICKS_PER_SEC));
    #else
    return 0;
    #endif
}

ULONGLONG Helpers::GetQPC()
{
    #ifdef _WIN32
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
    #else
    return 0;
    #endif
}

float Helpers::QPCToTicks(ULONGLONG qpc)
{
    return (float)((qpc - startTicks.QuadPart) / (freq.QuadPart / TICKS_PER_SEC));
}

float Helpers::QPCToDeltaMs(ULONGLONG qpc)
{
    return (float)(qpc / (freq.QuadPart / TICKS_PER_SEC));
}

void Helpers::SpinWaitMs(float ms)
{
    auto end = GetTicks() + ms;
    while(GetTicks() < end)
    {
        // spin
    }
}

void Helpers::SpinWaitQPC(unsigned qpc)
{
    auto end = GetQPC() + qpc;
    while(GetQPC() < end)
    {
        // spin
    }
}

void Helpers::Throw(HRESULT hr, const char* action)
{
    if(FAILED(hr))
    {
        #ifdef _WIN32
        _com_error err(hr);
        auto       error = err.ErrorMessage();        
        throw std::runtime_error(action + std::string("\r\n") + Helpers::WCharToString(error));
        #else
        throw std::runtime_error(action);
        #endif
    }
}

std::string Helpers::WCharToString(const wchar_t* text)
{
    #ifdef _WIN32
    char utfString[256];
    utfString[255] = 0;
    WideCharToMultiByte(CP_UTF8, 0, text, -1, utfString, 255, NULL, NULL);
    return std::string(utfString);
    #else
    return std::string("Not supported");
    #endif
}

} // namespace ShaderBeam