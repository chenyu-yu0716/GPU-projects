cmake_minimum_required(VERSION 3.25)

project(project1_video LANGUAGES CXX CUDA)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_EXTENSIONS OFF)

    find_package(CUDAToolkit REQUIRED)
endif()

find_path(BOOST_MULTIPRECISION_INCLUDE_DIR
          NAMES boost/multiprecision/cpp_dec_float.hpp REQUIRED)

# The video renderer reuses the online compute path of project1 without its
# OpenGL viewer, so it builds the shared project1 sources directly.
get_filename_component(PROJECT1_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../project1" ABSOLUTE)
get_filename_component(PROJECT1_VIDEO_BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../base" ABSOLUTE)

set(PROJECT1_VIDEO_SHARED_SOURCES
    "${PROJECT1_DIR}/src/mandelbrot.cu"
    "${PROJECT1_DIR}/src/mandelbrot_compute.cpp"
    "${PROJECT1_DIR}/src/palette.cu")

file(GLOB_RECURSE PROJECT1_VIDEO_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE PROJECT1_VIDEO_HEADERS CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

add_executable(project1_video
    ${PROJECT1_VIDEO_SOURCES}
    ${PROJECT1_VIDEO_HEADERS}
    ${PROJECT1_VIDEO_SHARED_SOURCES})
set_target_properties(project1_video PROPERTIES CUDA_ARCHITECTURES native)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PROJECT1_VIDEO_SOURCES} ${PROJECT1_VIDEO_HEADERS})
source_group(TREE "${PROJECT1_DIR}" PREFIX "project1" FILES ${PROJECT1_VIDEO_SHARED_SOURCES})
target_include_directories(project1_video PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(project1_video PRIVATE "${PROJECT1_DIR}/include")
target_include_directories(project1_video PRIVATE "${PROJECT1_VIDEO_BASE_DIR}")
target_include_directories(project1_video SYSTEM PRIVATE "${BOOST_MULTIPRECISION_INCLUDE_DIR}")
target_link_libraries(project1_video PRIVATE CUDA::cudart)
set_source_files_properties(
    "${PROJECT1_DIR}/src/mandelbrot.cu"
    PROPERTIES COMPILE_OPTIONS "--expt-relaxed-constexpr")
set_target_properties(project1_video PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
