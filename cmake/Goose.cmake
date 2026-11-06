include_guard(GLOBAL)

# The C ABI emitted by Goose is versioned with its runtime. Install the exact
# release with tools/bootstrap-goose.py rather than accepting arbitrary goose
# programs from PATH.
set(THYFOS_GOOSE_RUNTIME "4d3da949e563")

function(add_thyfos_goose_library source_directory)
    if(TARGET thyfos_goose)
        return()
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Goose v2026.3 requires a 64-bit target. Build this OS port with ARCH=amd64.")
    endif()

    set(GOOSE_COMPILER "${GOOSE_COMPILER}" CACHE FILEPATH "Host Goose v2026.3 compiler")
    if(NOT GOOSE_COMPILER AND DEFINED ENV{GOOSE_COMPILER})
        set(GOOSE_COMPILER "$ENV{GOOSE_COMPILER}" CACHE FILEPATH "Host Goose v2026.3 compiler" FORCE)
    endif()
    if(NOT GOOSE_COMPILER)
        find_program(_THYFOS_GOOSE_COMPILER NAMES goose NO_CMAKE_FIND_ROOT_PATH)
        if(_THYFOS_GOOSE_COMPILER)
            set(GOOSE_COMPILER "${_THYFOS_GOOSE_COMPILER}" CACHE FILEPATH "Host Goose v2026.3 compiler" FORCE)
        endif()
    endif()
    if(NOT GOOSE_COMPILER)
        message(FATAL_ERROR "Goose v2026.3 is required. Run tools/bootstrap-goose.py and set GOOSE_COMPILER to its printed path.")
    endif()
    execute_process(
        COMMAND "${GOOSE_COMPILER}" version
        RESULT_VARIABLE goose_result
        OUTPUT_VARIABLE goose_version
        ERROR_VARIABLE goose_error
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT goose_result EQUAL 0 OR NOT goose_version STREQUAL "goose runtime ${THYFOS_GOOSE_RUNTIME}")
        message(FATAL_ERROR "Expected Goose v2026.3 (runtime ${THYFOS_GOOSE_RUNTIME}); ${GOOSE_COMPILER} returned '${goose_version}' ${goose_error}")
    endif()

    get_filename_component(source_directory "${source_directory}" ABSOLUTE)
    set(generated_directory "${CMAKE_CURRENT_BINARY_DIR}/generated-goose")
    set(generated_c "${generated_directory}/thyfos_goose.c")
    set(generated_header "${generated_directory}/thyfos_goose.h")
    file(GLOB goose_sources CONFIGURE_DEPENDS "${source_directory}/*.goose")
    add_custom_command(
        OUTPUT "${generated_c}" "${generated_header}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${generated_directory}"
        COMMAND "${GOOSE_COMPILER}" --standalone --header "${generated_header}"
            -o "${generated_c}" "${source_directory}/core.goose"
        DEPENDS ${goose_sources} "${GOOSE_COMPILER}"
        WORKING_DIRECTORY "${source_directory}"
        COMMENT "Compiling Goose v2026.3 application core"
        VERBATIM)
    add_library(thyfos_goose STATIC "${generated_c}" "${generated_header}")
    target_include_directories(thyfos_goose PUBLIC "${generated_directory}")
    set_target_properties(thyfos_goose PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS YES)
    if(UNIX)
        find_package(Threads REQUIRED)
        target_link_libraries(thyfos_goose PUBLIC Threads::Threads m)
    endif()
endfunction()
