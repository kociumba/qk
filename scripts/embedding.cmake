function(qk_embed_file)
    set(options KEEP_ASM)
    set(oneValueArgs TARGET OUTPUT_VAR FILE)
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

    get_filename_component(_base "${ARG_FILE}" NAME_WE)

    if (WIN32)
        set(_obj "${_out_dir}/${_base}.obj")
    else ()
        set(_obj "${_out_dir}/${_base}.o")
    endif ()

    set(_extra_args "")
    if (ARG_KEEP_ASM)
        list(APPEND _extra_args "-k")
    endif ()

    add_custom_command(
            OUTPUT "${_obj}"
            COMMAND "${_embedder_exe}" -o "${_out_dir}" ${_extra_args} "${ARG_FILE}"
            DEPENDS "${ARG_FILE}" qk_embedder
            COMMENT "Embedding ${ARG_FILE} -> ${_obj}"
            VERBATIM
    )

    add_custom_target(${ARG_TARGET}_embed_${_base} DEPENDS "${_obj}")

    target_sources(${ARG_TARGET} PRIVATE "${_obj}")

    if (ARG_OUTPUT_VAR)
        set(${ARG_OUTPUT_VAR} "${_obj}" PARENT_SCOPE)
    endif ()
endfunction()
