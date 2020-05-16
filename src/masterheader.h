#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#pragma comment(lib, "ws2_32.lib")

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#pragma comment(lib, "extern/pix/winpixeventruntime.lib")

#pragma comment(lib, "extern/spdlog/spdlog.lib")

#pragma comment(lib, "extern/tbb/tbb.lib")
#pragma comment(lib, "extern/tbb/tbbbind.lib")
#pragma comment(lib, "extern/tbb/tbbmalloc.lib")
#pragma comment(lib, "extern/tbb/tbbproxy.lib")

#pragma warning(disable : 4267)

// uncomment to disable all asserts
//#define NDEBUG

// C/C++ Standard Library
#include <time.h>
#include <stdio.h>
#include <assert.h>
#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <queue>
#include <algorithm>
#include <array>
#include <numeric>
#include <mutex>
#include <functional>

// windows
#include <inttypes.h>
#include <wrl.h>
#include <windows.h>
#include <winuser.h>

// DirectX
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>

#include <extern/d3d12/d3dx12.h>

#include <extern/taskflow/taskflow.hpp>

#include <extern/boost/container/small_vector.hpp>
#include <extern/boost/circular_buffer.hpp>
#include <extern/boost/pool/object_pool.hpp>
#include <extern/boost/container_hash/hash.hpp>
#include <extern/boost/static_string.hpp>

// typedefs
using WindowHandle = uint64_t;

template <typename T, uint32_t N>
using InplaceArray = boost::container::small_vector<T, N>;

template<std::size_t N>
using StaticString = boost::static_strings::static_string<N>;

template<std::size_t N>
using StaticWString = boost::static_strings::static_wstring<N>;

template <typename T>
using CircularBuffer = boost::circular_buffer<T>;

// ComPtr namespace
using Microsoft::WRL::ComPtr;

#include <system/math.h>
#include <system/utils.h>
#include <system/memcpy.h>
#include <system/timer.h>
#include <system/system.h>
#include <system/logger.h>
#include <system/keyboard.h>
#include <system/mouse.h>
#include <system/profiler.h>
