cmake_minimum_required(VERSION 3.16)

project(dx12test LANGUAGES CXX)

# We only care about 1 config type (Release)
set(CMAKE_CONFIGURATION_TYPES Release)

# We dont want NDEBUG... we want asserts
string(REPLACE "/DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})

# C++17 required
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Unity (Jumbo) Build
set(CMAKE_UNITY_BUILD TRUE)

# Global compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")          # Program Database
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FI\"pch.h\"") # Force #include pch.h
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")          # Multi-Processor compilation
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")          # Level 4 warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")          # Warnings as errors
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast")     # Fast floating point
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")   # AVX2 BABY

# Disable some warnings...
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4100") # unreferenced formal parameter
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127") # conditional expression is constant
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4189") # local variable is initialized but not referenced
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244") # conversion from 'T1' to 'T2', possible loss of data
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267") # conversion from 'T1' to 'T2', possible loss of data
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4324") # structure was padded due to alignment specifier
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4702") # unreachable code

# Global linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG:FASTLINK") # Fast Linking

set(ROOT_DIR               "${CMAKE_CURRENT_LIST_DIR}")
set(BIN_DIR                "${CMAKE_CURRENT_LIST_DIR}/bin")
set(SRC_DIR                "${CMAKE_CURRENT_LIST_DIR}/src")
set(EXTERN_DIR             "${CMAKE_CURRENT_LIST_DIR}/extern")
set(SHADERCOMPILER_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/shadercompiler")

# all binaries for all config types (if any) go to the "bin" folder
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_CURRENT_LIST_DIR}/bin")
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_CURRENT_LIST_DIR}/bin")
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_CURRENT_LIST_DIR}/bin")
endforeach()

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

# Gather src files
file(GLOB_RECURSE ENGINE_SRC         "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.h" "${SRC_DIR}/*.hpp" "${SRC_DIR}/*.inl")
file(GLOB_RECURSE D3D12MA_SRC        "${EXTERN_DIR}/d3d12/*.*")
file(GLOB_RECURSE IMGUI_SRC          "${EXTERN_DIR}/imgui/*.*")
file(GLOB_RECURSE MICROPROFILE_SRC   "${EXTERN_DIR}/microprofile/*.*")
file(GLOB_RECURSE SIMPLEMATH_SRC     "${EXTERN_DIR}/simplemath/*.*")
file(GLOB_RECURSE SHADERCOMPILER_SRC "${SHADERCOMPILER_SRC_DIR}/*.cpp" "${SHADERCOMPILER_SRC_DIR}/*.h" "${SHADERCOMPILER_SRC_DIR}/*.hpp" "${SHADERCOMPILER_SRC_DIR}/*.inl")
file(GLOB         UTILS_FILES        "${SRC_DIR}/system/utils.*")

# Main Engine Proj src files to compile
set(ALL_ENGINE_SRC ${ENGINE_SRC})
set(ALL_ENGINE_SRC ${ALL_ENGINE_SRC} ${D3D12MA_SRC})
set(ALL_ENGINE_SRC ${ALL_ENGINE_SRC} ${IMGUI_SRC})
set(ALL_ENGINE_SRC ${ALL_ENGINE_SRC} ${MICROPROFILE_SRC})
set(ALL_ENGINE_SRC ${ALL_ENGINE_SRC} ${SIMPLEMATH_SRC})

# ShaderCompiler Proj src files to compile
set(ALL_SHADERCOMPILER_SRC ${SHADERCOMPILER_SRC})
set(ALL_SHADERCOMPILER_SRC ${ALL_SHADERCOMPILER_SRC} ${SIMPLEMATH_SRC})
set(ALL_SHADERCOMPILER_SRC ${ALL_SHADERCOMPILER_SRC} ${UTILS_FILES})

################################################################################

# Main Engine Proj
add_executable(dx12test WIN32 ${ALL_ENGINE_SRC})
target_compile_definitions(dx12test PUBLIC BBE_ENGINE)

# Insert proper filters in VS IDE to reflect folder structure
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

# Engine specific libs
target_link_libraries(dx12test PUBLIC d3d12)
target_link_libraries(dx12test PUBLIC d3dcompiler)
target_link_libraries(dx12test PUBLIC dxgi)
target_link_libraries(dx12test PUBLIC dxguid)
target_link_libraries(dx12test PUBLIC extern/lib/winpixeventruntime)

# Engine specific include dirs
target_include_directories(dx12test PUBLIC "${EXTERN_DIR}/imgui")

################################################################################

# Shader Compiler proj
add_executable(shadercompiler ${ALL_SHADERCOMPILER_SRC})
target_compile_definitions(shadercompiler PUBLIC BBE_SHADERCOMPILER)

# Shader Compiler specific pch
target_compile_options(shadercompiler PUBLIC "/FIshadercompiler_pch.h")

# Shader Compiler specific include dirs
target_include_directories(shadercompiler PUBLIC ${SHADERCOMPILER_SRC_DIR})
