function(qk_embed_file)
    set(options KEEP_ASM)
    set(oneValueArgs TARGET FILE COMPRESSION)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ARG_TARGET)
        message(FATAL_ERROR "qk_embed_file requires TARGET=<target>")
    endif ()
    if (NOT TARGET ${ARG_TARGET})
        message(FATAL_ERROR "Target ${ARG_TARGET} does not exist")
    endif ()
    if (NOT ARG_FILE)
        message(FATAL_ERROR "qk_embed_file requires FILE=<file>")
    endif ()

    if (NOT TARGET qk_embedder)
        message(FATAL_ERROR "qk_embedder target not found (did you enable QK_ENABLE_EMBEDDING?)")
    endif ()

    set(_embedder_exe "$<TARGET_FILE:qk_embedder>")

    set(_out_dir "${CMAKE_CURRENT_BINARY_DIR}/embedded")
    file(MAKE_DIRECTORY "${_out_dir}")

    get_filename_component(_filepath "${ARG_FILE}" ABSOLUTE)
    string(REGEX REPLACE "^.*/([^/]+)\\.[^.]*$" "\\1" _base "${_filepath}")

    string(REGEX REPLACE "[^a-zA-Z0-9]" "_" _sanitized_base "${_base}")

    if (_sanitized_base MATCHES "^[0-9]")
        set(_sanitized_base "_${_sanitized_base}")
    endif ()

    if (WIN32)
        set(_obj "${_out_dir}/${_sanitized_base}.obj")
    else ()
        set(_obj "${_out_dir}/${_sanitized_base}.o")
    endif ()

    set(_extra_args "")
    if (ARG_KEEP_ASM)
        list(APPEND _extra_args "-k")
    endif ()

    if (ARG_COMPRESSION)
        if (ARG_COMPRESSION STREQUAL "none")
            list(APPEND _extra_args "-c:none")
        elseif (ARG_COMPRESSION STREQUAL "speed")
            list(APPEND _extra_args "-c:speed")
        elseif (ARG_COMPRESSION STREQUAL "default")
            list(APPEND _extra_args "-c")
        elseif (ARG_COMPRESSION STREQUAL "compression")
            list(APPEND _extra_args "-c:compression")
        else ()
            message(FATAL_ERROR "Invalid compression level: ${ARG_COMPRESSION}. Valid: none, speed, default, compression")
        endif ()
    endif ()

    add_custom_command(
            OUTPUT "${_obj}"
            COMMAND "${_embedder_exe}" -o "${_out_dir}" ${_extra_args} "${ARG_FILE}"
            DEPENDS "${ARG_FILE}" qk_embedder
            COMMENT "Embedding ${ARG_FILE} -> ${_obj}"
            VERBATIM
    )

    add_custom_target(${ARG_TARGET}_embed_${_sanitized_base} DEPENDS "${_obj}")

    target_sources(${ARG_TARGET} PRIVATE "${_obj}")

    if (ARG_OUTPUT_VAR)
        set(${ARG_OUTPUT_VAR} "${_obj}" PARENT_SCOPE)
    endif ()
endfunction()
