# GPU-projects

GPU computing assignments and projects for graduate students.

## Shared base sources

Shared source modules belong in `base/`. The directory intentionally has no
`CMakeLists.txt`; each project manually adds the base files it uses to its own
target and resolves `./base` before falling back to the repository root's
`base/` directory.

## Graphics base sources

`base/graphics` requires OpenGL 4.6, GLAD (with OpenGL 4.6, loader, and
extension support), GLFW, and GLM. The repository's `vcpkg.json` declares
these dependencies. Configure CMake with your vcpkg toolchain, for example:

```sh
cmake --preset <preset> -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
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
