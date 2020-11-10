#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define MICROPROFILE_ENABLED 1
#define MICROPROFILE_GPU_TIMERS_D3D12 1
#define MICROPROFILE_WEBSERVER_MAXFRAMES 50

// ImGuiFileDialog uses ImGuiListClipper's old ctor
// #define IMGUI_DISABLE_OBSOLETE_FUNCTIONS

#if !defined(BBE_SHADERCOMPILER)
    #define USE_PIX
#endif

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "runtimeobject.lib")

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

// PIX
#pragma comment(lib, "extern/lib/winpixeventruntime.lib")

// SPD Log
#pragma comment(lib, "extern/lib/spdlog.lib")

#pragma warning(disable : 4267)

// uncomment to disable all asserts
//#define NDEBUG

// C Standard Lib
#include <time.h>
#include <stdio.h>
#include <assert.h>

// C++ STL
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
#include <shared_mutex>
#include <functional>
#include <fstream>
#include <bitset>
#include <map>

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

#if !defined(BBE_SHADERCOMPILER)
    // D3D12MA
    #include <extern/d3d12/D3D12MemAlloc.h>

    // IMGUI
    #include <extern/imgui/imgui.h>
    #include <extern/imgui/ImGuiFileDialog.h>
#endif

// PIX
#include <extern/pix/pix3.h>

// TaskFlow task threading lib
#include <extern/taskflow/taskflow.hpp>

// Boost
#include <extern/boost/container/small_vector.hpp>
#include <extern/boost/circular_buffer.hpp>
#include <extern/boost/pool/object_pool.hpp>
#include <extern/boost/container_hash/hash.hpp>
#include <extern/boost/static_string.hpp>
#include <extern/boost/preprocessor.hpp>
#include <extern/boost/uuid/uuid.hpp>
#include <extern/boost/uuid/uuid_generators.hpp>
#include <extern/boost/uuid/uuid_io.hpp>

// Cereal serialization lib
#define CEREAL_SERIALIZE_FUNCTION_NAME Serialize
#include <cereal/types/vector.hpp> // allow Cereal to serialize std::vector
#include <cereal/types/string.hpp> // allow Cereal to serialize std::string
#include <cereal/archives/binary.hpp> // Binary I/O
#include <cereal/archives/json.hpp> // JSON I/O

// SPD Log
#include <extern/spdlog/spdlog/spdlog.h>
#include <extern/spdlog/spdlog/sinks/basic_file_sink.h>

// Math
#include <extern/simplemath/SimpleMath.h>

// Microprofile
#include <extern/microprofile/microprofile.h>

// Arg Parse
#include <extern/argparse/argparse.h>

// typedefs
using WindowHandle = uint64_t;

template <typename T, uint32_t N>
using InplaceArray = boost::container::small_vector<T, N>;

template<std::size_t N>
using StaticString = boost::static_strings::static_string<N>;

template<std::size_t N>
using StaticWString = boost::static_strings::static_wstring<N>;

template <typename T>
using ObjectPool = boost::object_pool<T>;

template <typename T>
using CircularBuffer = boost::circular_buffer<T>;

using ObjectID = boost::uuids::uuid;
using ClassID = uint32_t;

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
#include <system/serializer.h>