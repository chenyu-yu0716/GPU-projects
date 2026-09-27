include_guard(GLOBAL)
include(CMakeParseArguments)

function(add_project_submission_target)
    cmake_parse_arguments(PARSE_ARGV 0 project_submission ""
                          "NAME;SOURCE_DIR"
                          "BASE_MODULES")

    if(NOT project_submission_NAME)
        message(FATAL_ERROR "add_project_submission_target requires NAME")
    endif()
    if(NOT project_submission_SOURCE_DIR)
        message(FATAL_ERROR "add_project_submission_target requires SOURCE_DIR")
    endif()

    get_filename_component(project_submission_source_dir
                           "${project_submission_SOURCE_DIR}" ABSOLUTE)
    if(NOT EXISTS "${project_submission_source_dir}/CMakeLists.txt")
        message(FATAL_ERROR "Submission source directory has no CMakeLists.txt: ${project_submission_source_dir}")
    endif()

    set(project_submission_base_dir "${project_submission_source_dir}/base")
    if(NOT EXISTS "${project_submission_base_dir}")
        get_filename_component(project_submission_base_dir
                               "${project_submission_source_dir}/../../base" ABSOLUTE)
    endif()

    if(project_submission_BASE_MODULES)
        if(NOT EXISTS "${project_submission_base_dir}")
            message(FATAL_ERROR "Unable to find base/ for ${project_submission_NAME}")
        endif()
        foreach(project_submission_base_module IN LISTS project_submission_BASE_MODULES)
            if(project_submission_base_module MATCHES "(^|/)\\.\\.(/|$)")
                message(FATAL_ERROR "Base module paths must not leave base/: ${project_submission_base_module}")
            endif()
            if(NOT IS_DIRECTORY "${project_submission_base_dir}/${project_submission_base_module}")
                message(FATAL_ERROR
                        "Base module '${project_submission_base_module}' does not exist in ${project_submission_base_dir}")
            endif()
        endforeach()
    endif()

    set(PROJECT_SUBMISSION_SOURCE_DIR "${project_submission_source_dir}")
    set(PROJECT_SUBMISSION_BASE_DIR "${project_submission_base_dir}")
    set(PROJECT_SUBMISSION_BASE_MODULES "${project_submission_BASE_MODULES}")
    set(PROJECT_SUBMISSION_STAGING_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/submission/${project_submission_NAME}")
    set(project_submission_archive
        "${CMAKE_CURRENT_BINARY_DIR}/submission/${project_submission_NAME}.zip")
    set(project_submission_script
        "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/package-${project_submission_NAME}.cmake")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/submission")

    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PackageSubmission.cmake.in"
                   "${project_submission_script}" @ONLY)

    add_custom_target("submit_${project_submission_NAME}"
        COMMAND "${CMAKE_COMMAND}" -P "${project_submission_script}"
        COMMAND "${CMAKE_COMMAND}" -E tar cfv "${project_submission_archive}" --format=zip
                "${project_submission_NAME}"
        BYPRODUCTS "${project_submission_archive}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/submission"
        COMMENT "Creating standalone source package for ${project_submission_NAME}"
        VERBATIM)

    set_target_properties("submit_${project_submission_NAME}" PROPERTIES FOLDER "utility")
endfunction()
