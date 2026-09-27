# GPU-projects

GPU computing assignments and projects for graduate students.

## Shared base sources

Shared source modules belong in `base/`. The directory intentionally has no
`CMakeLists.txt`; each project manually adds the base files it uses to its own
target and resolves `./base` before falling back to the repository root's
`base/` directory.

## Standalone submission packages

Every project in `projects/` provides a `submit_<name>` target. It creates
`submission/<name>.zip` below that target's build directory, containing the
project's sources and only the base modules listed in its CMake file.
