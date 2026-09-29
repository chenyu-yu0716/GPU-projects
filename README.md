# GPU-projects

GPU computing assignments and projects for graduate students.

## Shared base sources

Shared source modules belong in `base/`. The directory intentionally has no
`CMakeLists.txt`; each project manually adds the base files it uses to its own
target and resolves `./base` before falling back to the repository root's
`base/` directory.

## Graphics base sources

`base/graphics` requires OpenGL 4.6, GLAD (with OpenGL 4.6, loader, and
extension support), GLFW, and GLM.

## Dependency providers

The project supports both vcpkg and pre-installed dependencies. The committed
`CMakePresets.json` contains provider-neutral Windows and Linux presets, so it
does not assume a particular vcpkg installation path.

For vcpkg, set `VCPKG_ROOT` to any bootstrapped vcpkg checkout, copy
`CMakeUserPresets.json.example` to `CMakeUserPresets.json`, and select the
matching vcpkg preset. The user preset file is intentionally ignored by Git.
The repository's `vcpkg.json` then declares the required dependencies and their
baseline. The template provides Visual Studio, Linux GCC, and Linux Clang
variants.

For pre-installed dependencies, use a normal preset such as
`windows-vs-debug` or `linux-gcc-debug`. If the packages are outside CMake's
default search paths, set `CMAKE_PREFIX_PATH` in `CMakeUserPresets.json` or in
the configure environment.

For example, a vcpkg configure command is:

```sh
cmake --preset linux-gcc-debug-vcpkg
```

A project that manually adds graphics source files must also declare and link
the dependencies in its own `CMakeLists.txt`:

```cmake
find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

target_link_libraries(<target> PRIVATE glad::glad glfw glm::glm)
```

## Standalone submission packages

Every project in `projects/` provides a `submit_<name>` target. It creates
`submission/<name>.zip` below that target's build directory, containing the
project's sources and only the base modules listed in its CMake file.
