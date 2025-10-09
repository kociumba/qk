cmake_minimum_required(VERSION 3.28.3..3.31)
project(qk LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CMakeDependentOption)

option(QK_SHARED_LIB "builds qk as a shard lib instead of a static lib" OFF)
option(QK_BUILD_TESTS "builds the tests for qk" ON)
option(QK_BUILD_EXAMPLES "builds the examples for qk" ON)
option(QK_USE_EXCEPT "builds qk with c++ exceptions turned on" OFF)
option(QK_USE_SANITIZER "build debug builds of qk with sanitizers enabled" OFF)

option(QK_ENABLE_IPC "includes qk ipc in the built lib" ON)
option(QK_ENABLE_FILEPATH "includes qk filepath in the built lib" ON)
option(QK_ENABLE_EVENTS "include the qk event bus in the built lib" ON)
option(QK_ENABLE_THREADING "include the qk goroutine implementation in the built lib" ON)
option(QK_ENABLE_RUNTIME_UTILS "includes various runtime tricks for hooking/patching/manipulation of binaries" ON)
option(QK_ENABLE_TRAITS "include the various trait/impl like utilities in the built lib" ON)
option(QK_ENABLE_TRAITS_EXTRA "include extra traits that require reflection for their default implementations" ON)
option(QK_ENABLE_EMBEDDING "include utilities for embedding/reading data in binary files" ON)
option(QK_ENABLE_UTILS "include miscellaneous utilities that are too small to have their own modules" ON)

cmake_dependent_option(QK_NEEDS_REFLECTION "defines if qk needs reflection to work based on the enabled features"
        ON
        QK_ENABLE_EVENTS
        QK_ENABLE_TRAITS_EXTRA
        OFF)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(QK_SYSTEM windows)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(QK_SYSTEM unix)
endif ()

# disable runtime utils on unix for now
if (QK_SYSTEM STREQUAL "unix")
    set(QK_ENABLE_RUNTIME_UTILS OFF)
endif ()

if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(QK_BUILD_TESTS OFF) # disable tests when included using add_subdirectory or fetch_content
    set(QK_BUILD_EXAMPLES OFF)
endif ()

function(qk_add_test_source path_to_source)
    if (QK_BUILD_TESTS)
        target_sources(tests PUBLIC ${path_to_source})
    endif ()
endfunction()

set(QK_SOURCES
        qk/ipc/ipc.cpp
        qk/ipc/ipc.h
        qk/api.h
        qk/filepath/filepath.cpp
        qk/filepath/filepath.h
        qk/events/events.cpp
        qk/events/events.h
        qk/threading/gorutines.cpp
        qk/threading/gorutines.h
        qk/runtime/${QK_SYSTEM}/process.cpp
        qk/runtime/${QK_SYSTEM}/process.h
        qk/runtime/memory.cpp
        qk/runtime/memory.h
        qk/runtime/runtime.h
        qk/traits/traits.h
        qk/traits/traits_extra.h
        qk/binary/embedding.cpp
        qk/binary/embedding.h
        qk/utils/utils.h
)

if (QK_SHARED_LIB OR BUILD_SHARED_LIBS)
    add_library(qk SHARED ${QK_SOURCES})
    target_compile_definitions(qk
            PRIVATE QK_EXPORTS
            PUBLIC QK_BUILD_SHARED
    )
else ()
    add_library(qk STATIC ${QK_SOURCES})
endif ()

target_include_directories(qk INTERFACE include)
target_compile_options(qk PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-frtti>
        $<$<CXX_COMPILER_ID:MSVC>:/GR>)

if (NOT QK_USE_EXCEPT)
    target_compile_options(qk PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-exceptions>
            $<$<CXX_COMPILER_ID:MSVC>:/EHs->)
endif ()

set(SANITIZER_FLAGS "")
#set(SANITIZER_FLAGS $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fsanitize=address,undefined>
#        $<$<CXX_COMPILER_ID:MSVC>:/fsanitize=address>)

if (QK_USE_SANITIZER)
    target_compile_options(qk PRIVATE
            $<$<CONFIG:Debug>:${SANITIZER_FLAGS}>)
    target_link_options(qk PRIVATE
            $<$<CONFIG:Debug>:${SANITIZER_FLAGS}>)
endif ()

if (QK_SYSTEM STREQUAL "windows")
    target_compile_definitions(qk PUBLIC QK_WINDOWS QK_SYSTEM=${QK_SYSTEM})
elseif (QK_SYSTEM STREQUAL "unix")
    target_compile_definitions(qk PUBLIC QK_UNIX QK_SYSTEM=${QK_SYSTEM})
endif ()

if (QK_BUILD_TESTS)
    find_package(Catch2 QUIET)
    if (NOT Catch2_FOUND)
        include(FetchContent)
        FetchContent_Declare(
                Catch2
                GIT_REPOSITORY https://github.com/catchorg/Catch2
                GIT_TAG v3.10.0
                SYSTEM
        )
        FetchContent_GetProperties(Catch2)
        if (NOT Catch2_POPULATED)
            set(FETCHCONTENT_QUIET NO)
            FetchContent_MakeAvailable(Catch2)
        endif ()
    endif ()

    add_executable(tests)
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain qk)

    target_compile_options(tests PRIVATE ${SANITIZER_FLAGS})
    target_link_options(tests PRIVATE ${SANITIZER_FLAGS})

    if (QK_SYSTEM STREQUAL "windows")
        add_executable(init_gl WIN32 tests/extra/init_gl.c)
        target_link_libraries(init_gl PUBLIC user32
                gdi32
                opengl32)
    endif ()

    list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
    include(CTest)
    include(Catch)
    catch_discover_tests(tests)

    add_test(
            NAME go_test_filepath
            COMMAND go test filepath_test.go
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
endif ()

if (QK_NEEDS_REFLECTION)
    include(FetchContent)
    # no check for existence here since the download is just 3 files
    FetchContent_Declare(
            reflect_src
            DOWNLOAD_EXTRACT_TIMESTAMP OFF
            GIT_REPOSITORY https://github.com/qlibs/reflect
            GIT_TAG v1.2.6
            SYSTEM)
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(reflect_src)

    #    FetchContent_Declare(
    #            mp_src
    #            DOWNLOAD_EXTRACT_TIMESTAMP OFF
    #            GIT_REPOSITORY https://github.com/qlibs/mp
    #            GIT_TAG v2.0.11
    #            SYSTEM)
    #    set(FETCHCONTENT_QUIET NO)
    #    FetchContent_MakeAvailable(mp_src)

    target_sources(qk PUBLIC ${reflect_src_SOURCE_DIR}/reflect #[=[ ${mp_src_SOURCE_DIR}/mp ]=])
    target_include_directories(qk PUBLIC ${reflect_src_SOURCE_DIR} #[=[ ${mp_src_SOURCE_DIR} ]=])
endif ()

if (QK_ENABLE_IPC)
    set(NNG_TESTS OFF)
    find_package(nng QUIET)
    if (NOT nng_FOUND)
        include(FetchContent)
        FetchContent_Declare(
                nng
                DOWNLOAD_EXTRACT_TIMESTAMP OFF
                GIT_REPOSITORY https://github.com/nanomsg/nng
                GIT_TAG v1.11
                SYSTEM)
        FetchContent_GetProperties(nng)
        if (NOT nng_POPULATED)
            set(FETCHCONTENT_QUIET NO)
            FetchContent_MakeAvailable(nng)
        endif ()
    endif ()

    target_link_libraries(qk PUBLIC nng)
    target_compile_definitions(qk PUBLIC QK_IPC)
    qk_add_test_source(tests/ipc_test.cpp)
endif ()

if (QK_ENABLE_FILEPATH)
    target_compile_definitions(qk PUBLIC QK_FILEPATH)
    qk_add_test_source(tests/filepath_test.cpp)
endif ()

if (QK_ENABLE_EVENTS)
    target_compile_definitions(qk PUBLIC QK_EVENTS)
    qk_add_test_source(tests/events_test.cpp)
endif ()

if (QK_ENABLE_THREADING)
    find_package(Threads REQUIRED)

    target_compile_definitions(qk PUBLIC QK_THREADING)
    target_link_libraries(qk PUBLIC Threads::Threads)
    qk_add_test_source(tests/threading_test.cpp)
endif ()

if (QK_ENABLE_RUNTIME_UTILS)
    target_compile_definitions(qk PUBLIC QK_RUNTIME_UTILS)
    qk_add_test_source(tests/runtime_${QK_SYSTEM}.cpp)
endif ()

if (QK_ENABLE_TRAITS)
    target_compile_definitions(qk PUBLIC QK_TRAITS)
    qk_add_test_source(tests/traits_test.cpp)
endif ()

if (QK_ENABLE_TRAITS_EXTRA)
    target_compile_definitions(qk PUBLIC QK_TRAITS_EXTRA)
endif ()

if (QK_ENABLE_EMBEDDING)
    find_program(QK_NASM_EXECUTABLE nasm NO_CACHE)

    if (QK_NASM_EXECUTABLE)
        message(STATUS "Using system NASM: ${QK_NASM_EXECUTABLE}")
    else ()
        message(STATUS "Using vendored NASM...")

        if (WIN32)
            set(_nasm_name "nasm.exe")
        elseif (APPLE)
            set(_nasm_name "nasm_macos")
        elseif (UNIX)
            set(_nasm_name "nasm_linux")
        else ()
            message(FATAL_ERROR "Unsupported platform for vendored NASM.")
        endif ()

        set(QK_NASM_EXECUTABLE
                "${CMAKE_CURRENT_SOURCE_DIR}/vendor/nasm/${_nasm_name}"
        )

        if (NOT EXISTS "${QK_NASM_EXECUTABLE}")
            message(FATAL_ERROR
                    "Vendored NASM not found at ${QK_NASM_EXECUTABLE}. "
                    "Please provide prebuilt binaries under vendor/nasm/ or install NASM on your system"
            )
        endif ()
    endif ()

    find_package(zlib CONFIG COMPONENTS static QUIET)
    if (NOT zlib_FOUND)
        include(FetchContent)
        set(ZLIB_BUILD_TESTING OFF CACHE BOOL "Disable zlib tests" INTERNAL)
        set(ZLIB_INSTALL OFF CACHE BOOL "Disable zlib install target" INTERNAL)
        set(ZLIB_BUILD_SHARED OFF CACHE BOOL "Build zlib as a static library" INTERNAL)
        set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "Disable zlib examples" INTERNAL)
        FetchContent_Declare(
                zlib
                DOWNLOAD_EXTRACT_TIMESTAMP OFF
                GIT_REPOSITORY https://github.com/madler/zlib
                GIT_TAG v1.3.1
                SYSTEM
                CMAKE_ARGS
                -DZLIB_BUILD_TESTING=OFF
                -DZLIB_INSTALL=OFF
                -DZLIB_BUILD_SHARED=OFF
                -DZLIB_BUILD_EXAMPLES=OFF)
        FetchContent_GetProperties(zlib)
        if (NOT zlib_POPULATED)
            set(FETCHCONTENT_QUIET NO)
            FetchContent_MakeAvailable(zlib)
        endif ()
    endif ()

    target_link_libraries(qk PUBLIC zlibstatic)
    target_compile_definitions(qk PUBLIC QK_HAS_ZLIB)

    add_executable(qk_embedder tools/embedder/embedder.cpp)
    target_link_libraries(qk_embedder PUBLIC qk zlibstatic)

    include(scripts/embedding.cmake)

    target_compile_definitions(qk PUBLIC QK_EMBEDDING QK_NASM="${QK_NASM_EXECUTABLE}")
    qk_add_test_source(tests/embedding_test.cpp)
    qk_embed_file(
            TARGET tests
            FILE ${CMAKE_SOURCE_DIR}/tests/embed_me.txt
            KEEP_ASM
            COMPRESSION compression
    )
endif ()

if (QK_ENABLE_UTILS)
    target_compile_definitions(qk PUBLIC QK_UTILS)
    qk_add_test_source(tests/utils_test.cpp)
endif ()

# examples

file(GLOB EXAMPLE_SOURCES "${CMAKE_SOURCE_DIR}/examples/*.cpp")

foreach (SOURCE_FILE IN LISTS EXAMPLE_SOURCES)
    get_filename_component(TARGET_NAME "${SOURCE_FILE}" NAME_WE)

    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} PUBLIC qk)
endforeach ()
