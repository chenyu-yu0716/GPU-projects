cmake_minimum_required(VERSION 3.25)

project(project1_mandelbrot LANGUAGES CXX CUDA)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_EXTENSIONS OFF)

    find_package(CUDAToolkit REQUIRED)
endif()

find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

set(PROJECT1_BASE_FILES
    cpu/timer.h
    cpu/window.cpp
    cpu/window.h
    gpu/timer.h
    gpu/utility.h
    graphics/gl_utility.h
    graphics/graphics_context.cpp
    graphics/graphics_context.h
    graphics/rhi_resource.h
    graphics/sampler.cpp
    graphics/sampler.h
    graphics/shader_module.cpp
    graphics/shader_module.h
    graphics/texture/texture.cpp
    graphics/texture/texture.h
    graphics/texture/texture2d.cpp
    graphics/texture/texture2d.h
    graphics/texture/texture_utility.h)

set(PROJECT1_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/base")
if(NOT IS_DIRECTORY "${PROJECT1_BASE_DIR}")
    get_filename_component(PROJECT1_BASE_DIR
                           "${CMAKE_CURRENT_SOURCE_DIR}/../../base" ABSOLUTE)
endif()

set(PROJECT1_BASE_SOURCES)
set(PROJECT1_BASE_HEADERS)
foreach(PROJECT1_BASE_FILE IN LISTS PROJECT1_BASE_FILES)
    set(PROJECT1_BASE_FILE_PATH "${PROJECT1_BASE_DIR}/${PROJECT1_BASE_FILE}")
    if(NOT EXISTS "${PROJECT1_BASE_FILE_PATH}")
        message(FATAL_ERROR "Unable to find base file: ${PROJECT1_BASE_FILE}")
    endif()

    if(PROJECT1_BASE_FILE_PATH MATCHES "\\.(c|cc|cpp|cxx|cu)$")
        list(APPEND PROJECT1_BASE_SOURCES "${PROJECT1_BASE_FILE_PATH}")
    else()
        list(APPEND PROJECT1_BASE_HEADERS "${PROJECT1_BASE_FILE_PATH}")
    endif()
endforeach()

file(GLOB_RECURSE PROJECT1_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cu")
file(GLOB_RECURSE PROJECT1_HEADERS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hh"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hxx"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.cuh"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.inl")

add_executable(project1 ${PROJECT1_SOURCES} ${PROJECT1_HEADERS})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PROJECT1_SOURCES} ${PROJECT1_HEADERS})
target_sources(project1 PRIVATE ${PROJECT1_BASE_SOURCES} ${PROJECT1_BASE_HEADERS})
source_group(TREE "${PROJECT1_BASE_DIR}" PREFIX "base" FILES ${PROJECT1_BASE_SOURCES} ${PROJECT1_BASE_HEADERS})
target_include_directories(project1 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(project1 PRIVATE "${PROJECT1_BASE_DIR}")
target_link_libraries(project1 PRIVATE CUDA::cudart glad::glad glfw glm::glm)

if(COMMAND add_project_submission_target)
    add_project_submission_target(
        NAME project1
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
        BASE_FILES ${PROJECT1_BASE_FILES})
endif()
