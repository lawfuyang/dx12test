cmake_minimum_required(VERSION 3.16)

project(dx12test LANGUAGES CXX)

# We only care about 1 config type (Release)
set(CMAKE_CONFIGURATION_TYPES Release)

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Unity (Jumbo) Build
set(CMAKE_UNITY_BUILD TRUE)

# Global compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FI\"pch.h\"")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")

# disable IRRITATING type conversion warning
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267")

set(ROOT_DIR   "${CMAKE_CURRENT_LIST_DIR}")
set(BIN_DIR    "${CMAKE_CURRENT_LIST_DIR}/bin")
set(SRC_DIR    "${CMAKE_CURRENT_LIST_DIR}/src")
set(EXTERN_DIR "${CMAKE_CURRENT_LIST_DIR}/extern")

# all binaries for all config types (if any) go to the "bin" folder
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_CURRENT_LIST_DIR}/bin")
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_CURRENT_LIST_DIR}/bin")
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_CURRENT_LIST_DIR}/bin")
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# global libs
link_directories(${ROOT_DIR})
link_libraries(ws2_32)
link_libraries(runtimeobject)
link_libraries(extern/lib/spdlog)

# global include dirs
include_directories(${ROOT_DIR})
include_directories(${SRC_DIR})
include_directories(${EXTERN_DIR})
include_directories("${EXTERN_DIR}/spdlog")

# Insert proper filters in VS IDE to reflect folder structure
file(GLOB_RECURSE CORE_SRC         "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.h" "${SRC_DIR}/*.hpp" "${SRC_DIR}/*.inl")
file(GLOB_RECURSE D3D12MA_SRC      "${EXTERN_DIR}/d3d12/*.*")
file(GLOB_RECURSE IMGUI_SRC        "${EXTERN_DIR}/imgui/*.*")
file(GLOB_RECURSE MICROPROFILE_SRC "${EXTERN_DIR}/microprofile/*.*")
file(GLOB_RECURSE SIMPLEMATH_SRC   "${EXTERN_DIR}/simplemath/*.*")

# Main Engine Proj
add_executable(dx12test WIN32 ${CORE_SRC} ${D3D12MA_SRC} ${IMGUI_SRC} ${MICROPROFILE_SRC} ${SIMPLEMATH_SRC})
target_compile_definitions(dx12test PUBLIC BBE_ENGINE)

foreach(dir ${SRC_DIR} ${EXTERN_DIR})
    file(GLOB_RECURSE _source_list "${dir}/*.*")
        
    foreach(_source IN ITEMS ${_source_list})
        get_filename_component(_source_path "${_source}" PATH)
        
        string(FIND ${_source_path} "extern" found)
        if(NOT found EQUAL -1)
            string(REPLACE "${dir}" "extern" _group_path "${_source_path}")
        else()
            string(REPLACE "${dir}" "" _group_path "${_source_path}")
        endif()
        
        string(REPLACE "/" "\\" _group_path "${_group_path}")
        source_group("${_group_path}" FILES "${_source}")
    endforeach()
endforeach()

set_property(TARGET dx12test PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)

# Engine specific libs
target_link_libraries(dx12test PUBLIC d3d12)
target_link_libraries(dx12test PUBLIC d3dcompiler)
target_link_libraries(dx12test PUBLIC dxgi)
target_link_libraries(dx12test PUBLIC dxguid)
target_link_libraries(dx12test PUBLIC extern/lib/winpixeventruntime)

# Engine specific include dirs
target_include_directories(dx12test PUBLIC "${EXTERN_DIR}/imgui")

################################################################################

set(SC_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/shadercompiler")

file(GLOB_RECURSE SRC_CORE_SRC "${SC_SRC_DIR}/*.cpp" "${SC_SRC_DIR}/*.h" "${SC_SRC_DIR}/*.hpp" "${SC_SRC_DIR}/*.inl")
file(GLOB UTILS_FILES "${SRC_DIR}/system/utils.*")

# Shader Compiler proj
add_executable(shadercompiler ${SRC_CORE_SRC} ${SIMPLEMATH_SRC} ${UTILS_FILES})
target_compile_definitions(shadercompiler PUBLIC BBE_SHADERCOMPILER)

set_property(TARGET shadercompiler PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)

# Shader Compiler specific pch
target_compile_options(shadercompiler PUBLIC "/FIshadercompiler_pch.h")

# Shader Compiler specific include dirs
target_include_directories(shadercompiler PUBLIC ${SC_SRC_DIR})
