/*
ShaderBeam: shader effect overlay
Copyright (C) 2025 mausimus (mausimus.net)
https://github.com/mausimus/ShaderBeam
MIT License
*/

#pragma once

#ifdef _WIN32
#include "targetver.h"
#endif
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#ifdef _WIN32
#include <comdef.h>
#include <timeapi.h>
#include <dwmapi.h>
#endif

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>

#ifdef _WIN32
#include <tchar.h>
#endif

#include <vector>
#include <span>
#include <array>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <filesystem>
#include <map>
#include <deque>
#include <mutex>
#include <utility>
#include <thread>
#include <cmath>
#include <cfloat>

#ifdef _WIN32
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#endif

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>

#ifndef _WIN32
#define WPARAM int
#define LPARAM int
#include "com_ptr.h"
#define WINRT_ASSERT(a)
#define OutputDebugStringA(msg) printf("%s\n", msg)
#endif
