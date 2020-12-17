#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#define _SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING // for concurrency::concurrent_unordered_set
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING // for std::wstring_convert

#if !defined(BBE_SHADERCOMPILER)
    #define BBE_USE_PROFILER
    //#define BBE_USE_GPU_PROFILER
#endif

// uncomment to disable all asserts
//#define NDEBUG

#if !defined(BBE_SHADERCOMPILER)
    #define USE_PIX
#endif

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "runtimeobject.lib")

#if !defined(BBE_SHADERCOMPILER)
    #pragma comment(lib, "d3d12.lib")
    #pragma comment(lib, "d3dcompiler.lib")
    #pragma comment(lib, "dxgi.lib")
    #pragma comment(lib, "dxguid.lib")

    // PIX
    #pragma comment(lib, "extern/lib/winpixeventruntime.lib")
#endif

// SPD Log
#pragma comment(lib, "extern/lib/spdlog.lib")

#pragma warning(disable : 4267)

// C Standard Lib
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

// C++ STL
#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <numeric>
#include <queue>
#include <set>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>
#include <codecvt>

// windows
#include <concurrent_queue.h>
#include <concurrent_unordered_map.h>
#include <concurrent_unordered_set.h>
#include <concurrent_vector.h>
#include <windows.h>
#include <winuser.h>
#include <wrl.h>

#if !defined(BBE_SHADERCOMPILER)
    // DirectX
    #include <d3d12.h>
    #include <d3d12sdklayers.h>
    #include <d3dcompiler.h>
    #include <dxgi1_6.h>
    #include <extern/d3d12/d3dx12.h>
#endif

#if !defined(BBE_SHADERCOMPILER)
    // D3D12MA
    #include <extern/d3d12/D3D12MemAlloc.h>

    // IMGUI
    #define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
    #include <extern/imgui/imgui.h>
    #include <extern/imgui/ImGuiFileDialog.h>

    // Cereal serialization lib
    #define CEREAL_SERIALIZE_FUNCTION_NAME Serialize
    #include <extern/cereal/types/vector.hpp> // allow Cereal to serialize std::vector
    #include <extern/cereal/types/string.hpp> // allow Cereal to serialize std::string
    #include <extern/cereal/archives/binary.hpp> // Binary I/O
    #include <extern/cereal/archives/json.hpp> // JSON I/O

    // Arg Parse
    #include <extern/argparse/argparse.h>
#endif // #if defined(BBE_SHADERCOMPILER)

// PIX
#include <extern/pix/pix3.h>

// TaskFlow task threading lib
#include <extern/taskflow/taskflow.hpp>

// Boost
#include <extern/boost/circular_buffer.hpp>
#include <extern/boost/container/flat_map.hpp>
#include <extern/boost/container/flat_set.hpp>
#include <extern/boost/container/small_vector.hpp>
#include <extern/boost/container/static_vector.hpp>
#include <extern/boost/container_hash/hash.hpp>
#include <extern/boost/pool/object_pool.hpp>
#include <extern/boost/preprocessor.hpp>
#include <extern/boost/static_string.hpp>
#include <extern/boost/uuid/uuid.hpp>
#include <extern/boost/uuid/uuid_generators.hpp>
#include <extern/boost/uuid/uuid_io.hpp>

// SPD Log
#include <extern/spdlog/spdlog/spdlog.h>
#include <extern/spdlog/spdlog/sinks/basic_file_sink.h>

// Math
#include <extern/simplemath/SimpleMath.h>

// Microprofiler
#if defined(BBE_USE_PROFILER)
    #define MICROPROFILE_ENABLED 1
    #define MICROPROFILE_WEBSERVER_MAXFRAMES 50

    #if defined(BBE_USE_GPU_PROFILER)
        #define MICROPROFILE_GPU_TIMERS_D3D12 1
    #else
        #define MICROPROFILE_GPU_TIMERS 0
    #endif
#else
    #define MICROPROFILE_ENABLED 0
#endif
#include <extern/microprofile/microprofile.h>

// typedefs
using WindowHandle = uint64_t;

#if !defined(BBE_SHADERCOMPILER)
    using DXGISwapChain            = IDXGISwapChain4;
    using D3D12Device              = ID3D12Device8;
    using D3D12PipelineLibrary     = ID3D12PipelineLibrary1;
    using D3D12GraphicsCommandList = ID3D12GraphicsCommandList6;
    using D3D12Fence               = ID3D12Fence1;
    using D3D12Resource            = ID3D12Resource;
#endif

template <typename T>
using ConcurrentVector = concurrency::concurrent_vector<T>;

template <typename KeyType, typename ValueType>
using ConcurrentUnorderedMap = concurrency::concurrent_unordered_map<KeyType, ValueType>;

template <typename T>
using ConcurrentUnorderedSet = concurrency::concurrent_unordered_set<T>;

template <typename T, uint32_t N>
using InplaceArray = boost::container::small_vector<T, N>;

template <typename T, uint32_t N>
using FixedSizeArray = boost::container::static_vector<T, N>;

template <typename KeyType, typename ValueType>
using FlatMap = boost::container::flat_map<KeyType, ValueType>;

template <typename KeyType>
using FlatSet = boost::container::flat_set<KeyType>;

template <typename KeyType, typename ValueType, std::size_t N>
using InplaceFlatMap = boost::container::flat_map<KeyType, ValueType, std::less<KeyType>, InplaceArray<std::pair<KeyType, ValueType>, N>>;

template <typename ValueType, uint32_t N>
using InplaceFlatSet = boost::container::flat_set< ValueType, std::less<ValueType>, InplaceArray<ValueType, N>>;

template <typename KeyType, typename ValueType, std::size_t N>
using FixedSizeFlatMap = boost::container::flat_map<KeyType, ValueType, std::less<KeyType>, boost::container::static_vector<std::pair<KeyType, ValueType>, N>>;

template <typename ValueType, uint32_t N>
using FixedSizeFlatSet = boost::container::flat_set< ValueType, std::less<ValueType>, boost::container::static_vector<ValueType, N>>;

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
#include <system/timer.h>
#include <system/system.h>
#include <system/logger.h>
#include <system/profiler.h>
#include <system/criticalsection.h>

#if !defined(BBE_SHADERCOMPILER)
    #include <system/memcpy.h>
    #include <system/serializer.h>
    #include <system/keyboard.h>
    #include <system/mouse.h>
#endif
