cmake_minimum_required(VERSION 3.25)

project(gpu_projects VERSION 1.0 LANGUAGES CXX CUDA)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "utility")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(WIN32)
    add_compile_definitions(NOMINMAX _USE_MATH_DEFINES)
endif()

find_package(CUDAToolkit REQUIRED)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/ProjectSubmission.cmake")

# Each example owns its target definition in its CMakeLists.txt.
function(add_cuda_example example_name)
    set(example_dir "${CMAKE_CURRENT_SOURCE_DIR}/examples/${example_name}")

    if(EXISTS "${example_dir}/CMakeLists.txt")
        add_subdirectory("${example_dir}")
    else()
        message(STATUS "Skipping example '${example_name}': no CMakeLists.txt")
        return()
    endif()

    if(TARGET "${example_name}")
        set_target_properties("${example_name}" PROPERTIES FOLDER "examples")
    endif()
endfunction()

set(EXAMPLES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples")
if(EXISTS "${EXAMPLES_DIR}")
    file(GLOB example_names CONFIGURE_DEPENDS LIST_DIRECTORIES true
         RELATIVE "${EXAMPLES_DIR}" "${EXAMPLES_DIR}/*")
    foreach(example_name IN LISTS example_names)
        if(IS_DIRECTORY "${EXAMPLES_DIR}/${example_name}")
            add_cuda_example("${example_name}")
        endif()
    endforeach()
endif()

# Each project owns its CMake target definition. The target name matching the
# directory name is placed in the project folder when supported by that project.
set(PROJECTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/projects")
if(EXISTS "${PROJECTS_DIR}")
    file(GLOB project_names CONFIGURE_DEPENDS LIST_DIRECTORIES true
         RELATIVE "${PROJECTS_DIR}" "${PROJECTS_DIR}/*")
    foreach(project_name IN LISTS project_names)
        set(project_dir "${PROJECTS_DIR}/${project_name}")
        if(IS_DIRECTORY "${project_dir}" AND EXISTS "${project_dir}/CMakeLists.txt")
            add_subdirectory("${project_dir}")
            if(TARGET "${project_name}")
                set_target_properties("${project_name}" PROPERTIES FOLDER "projects")
            endif()
        endif()
    endforeach()
endif()

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
if(CLANG_FORMAT_EXECUTABLE)
    file(GLOB_RECURSE PROJECTS_SOURCE_FILES CONFIGURE_DEPENDS
         "${EXAMPLES_DIR}/*.cpp"
         "${EXAMPLES_DIR}/*.cu"
         "${EXAMPLES_DIR}/*.h"
         "${EXAMPLES_DIR}/*.hpp"
         "${EXAMPLES_DIR}/*.cuh"
         "${CMAKE_CURRENT_SOURCE_DIR}/base/*.cpp"
         "${CMAKE_CURRENT_SOURCE_DIR}/base/*.cu"
         "${CMAKE_CURRENT_SOURCE_DIR}/base/*.h"
         "${CMAKE_CURRENT_SOURCE_DIR}/base/*.hpp"
         "${CMAKE_CURRENT_SOURCE_DIR}/base/*.cuh"
         "${PROJECTS_DIR}/*.cpp"
         "${PROJECTS_DIR}/*.cu"
         "${PROJECTS_DIR}/*.h"
         "${PROJECTS_DIR}/*.hpp"
         "${PROJECTS_DIR}/*.cuh")
    if(PROJECTS_SOURCE_FILES)
        add_custom_target(format-projects
            COMMAND "${CLANG_FORMAT_EXECUTABLE}" -i -style=file ${PROJECTS_SOURCE_FILES}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMENT "Formatting project source files"
            VERBATIM)
        set_target_properties(format-projects PROPERTIES FOLDER "utility")
    endif()
endif()
