# qk

*qk is A library for c++*

Essentially it's all semi useful ideas I have in a single package.
While the modules are designed from my perspective(game/graphics/systems programming), they can definitely be utilized
in any scenario.

> [!WARNING]
> This library is extremely opinionated, but I find that most of the opinions can be universally understood as good
> practices by using ones brain.

The library is designed to be used through cmake `add_subdirectory()` or `fetch_content()`
and since all the ideas and modules are so decoupled from each other, the library is designed to be highly modular.

If a module or features is not needed it can be excluded from the compilation with all of its dependencies.

Support for build systems like mason or zig, may be added at some point in the future.

## Quickstart

The simplest and recommended way to use qk is to use `fetch_content()` in cmake:

```cmake
include(FetchContent)
FetchContent_Declare(
        qk
        DOWNLOAD_EXTRACT_TIMESTAMP OFF
        GIT_REPOSITORY https://github.com/kociumba/qk
        GIT_TAG # for now it is recommended to pin a commit hash here
        SYSTEM)
set(FETCHCONTENT_QUIET NO)
FetchContent_MakeAvailable(qk)

add_executable(example)
target_link_libraries(example PUBLIC qk)
```

There are [examples](/examples) if you want to see the usage of modules, the examples are still being written.
They have plenty of comments explaining what is happening, and can be run if you have the repo cloned or set
`QK_BUILD_EXAMPLES=ON`.

## Modules

qk currently includes 6 modules (traits_extra is treated as an extension of the traits module):

| module       | default | description                                                                                                                                                                                            | cmake option              | macro              |
|--------------|---------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|---------------------------|--------------------|
| events       | `ON`    | provides a type safe event bus where types are used as events (requires external reflection, may be changed to only RTTI)                                                                              | `QK_ENABLE_EVENTS`        | `QK_EVENTS`        |
| filepath     | `ON`    | this is a port of the go `path/filepath` package, it is tested against the behaviour of the original go code (not all functions are ported, the current subset is the internal `filepathlite` package) | `QK_ENABLE_FILEPATH`      | `QK_FILEPATH`      |
| ipc          | `ON`    | provides an opinionated non-blocking ipc framework, meant for use mostly in game-loop like scenarios                                                                                                   | `QK_ENABLE_IPC`           | `QK_IPC`           |
| runtime      | `ON`    | provides various tools for manipulating processes and binaries at runtime (currently only implemented for windows)                                                                                     | `QK_ENABLE_RUNTIME_UTILS` | `QK_RUNTIME_UTILS` |
| threading    | `ON`    | this is a non 1 to 1 port of the go threading model with goroutines and channels (beware that the current goroutine implementation uses real threads instead of green threads)                         | `QK_ENABLE_THREADING`     | `QK_THREADING`     |
| traits       | `ON`    | implements a few rust style traits using concepts and static base implementations                                                                                                                      | `QK_ENABLE_TRAITS`        | `QK_TRAITS`        |
| traits_extra | `ON`    | this is an extension of the traits module, that implements traits requiring external reflection for default implementations                                                                            | `QK_ENABLE_TRAITS_EXTRA`  | `QK_TRAITS_EXTRA`  |

if using qk without cmake, the macros corresponding to each module need to be defined (most easily in the compile
command) to generate the implementations and definitions for the module.

> [!TIP]
> The ipc module uses nng as a dependency, disabling it will speed up compilation drastically.

Any dependencies used by qk will be fetched using `fetch_content()`, the reflection lib used when a module requires it
is [reflect](https://github.com/qlibs/reflect).
If a dependency like nng can be found on the system, that version will be used instead.

Most of the modules are cross-platform, the only difference being the runtime module which is currently only implemented
for windows due to its heavy dependency on platform api's.

aside from the module options qk exports a few more cmake options:

| option              | default                                  | description                                                                                                          |
|---------------------|------------------------------------------|----------------------------------------------------------------------------------------------------------------------|
| `QK_SHARED_LIB`     | `OFF`                                    | qk will be built as a shared lib, the same behaviour as setting `BUILD_SHARED_LIBS`                                  |
| `QK_BUILD_TESTS`    | `OFF` when qk is imported `ON` otherwise | the catch2 test suit will be built along side qk and a few supporting applications                                   |
| `QK_USE_EXCEPT`     | `OFF`                                    | determines whether qk should be build with exceptions enabled, should be left `OFF` since qk does not use exceptions |
| `QK_BUILD_EXAMPLES` | `OFF` when qk is imported `ON` otherwise | examples will be added as runnable executable targets in cmake                                                       |

## Code style

qk does not use c++ exceptions, oop and virtual dispatch and requires c++23 and up.

| compiler | minimum required version |
|----------|--------------------------|
| gcc      | 14.x                     |
| clang    | 18.x                     |
| msvc     | 19.43 (VS 2022 17.13+)   |

Most qk modules and features are designed to have as little performance impact as possible or shift it onto comp time
instead of impacting runtime.

Many api's are simple procedural c style functions, some apis don't even use methods instead prioritizing user data
ownership with companion functions to structs.

All parts of qk are public api, you can poke around in the internals all you want, you should use common sense to know
what to modify and what not to.

## Bugs, issues and contributing

To report bugs or request ideas please do so by opening an issue on [github](https://github.com/kociumba/qk). I will
evaluate the issues and determine if I am willing to implement the request.
Bugs will of course be prioritised.

If I am not willing to implement a feature or a module myself and a pull request implementing it is submitted it will be
considered for merging.

Code should be formatted using the style outlined in `.clang-format`.

## License

qk is licensed under the MIT License. See [LICENSE](LICENSE) for details.
