# Notes on `base/` utilities

`base/` is course support code and must not be modified for the assignments.
These notes hold the comments and proposed fixes that would otherwise live
there.

## `base/common/utility.h`: `print` / `println`

Formatted printing to stdout, e.g. `println("device {} has {} MB", id, mem)`.
Stand-ins for C++23 `std::print` / `std::println`, which C++20 lacks.

```cpp
template <typename... Args> void print(std::format_string<Args...> pattern, Args&&... args) {
    std::cout << std::format(pattern, std::forward<Args>(args)...);
}
```

- `typename... Args` is a pack of zero or more types, deduced per call.
- `Args&&... args` takes each argument by forwarding reference, so both
  variables and temporaries are accepted without copying.
- `std::forward<Args>(args)...` passes each argument on unchanged.
- `std::format_string<Args...>` checks the pattern against the argument
  types at compile time.

`print()` prints the formatted text without a trailing newline; `println()` is
the same, followed by a newline.

## `base/gpu/utility.h`: `logCudaErrorImpl` / `checkCudaImpl`

Both helpers describe the error with `cudaGetLastError()` instead of `status`.
`cudaGetLastError()` also resets the thread's last-error state, so
`CHECK_CUDA(cudaGetLastError())` reads it twice and prints "no error"; only
`CHECK_CUDA(cudaPeekAtLastError())` reports a failed kernel launch correctly.

The proposed versions below describe `status` and reset the state explicitly.

```cpp
inline void logCudaErrorImpl(cudaError_t status, char const* cudaCall) {
    if (status != cudaSuccess) {
        std::cerr << std::format("[CUDA] {} failed({}): {}\n",
                                 cudaCall,
                                 cudaGetErrorName(status),
                                 cudaGetErrorString(status));

        // Reset the last-error state so that it is not reported again later.
        static_cast<void>(cudaGetLastError());
    }
}
```

```cpp
inline void checkCudaImpl(cudaError_t status,
                          char const* cudaCall,
                          std::source_location const location = std::source_location::current()) {
    if (status != cudaSuccess) {
        std::cerr << std::format("[CUDA] {} failed at {}:{}: {}\n",
                                 cudaCall,
                                 location.file_name(),
                                 location.line(),
                                 cudaGetErrorString(status));

        // Reset the last-error state so that it is not reported again later.
        static_cast<void>(cudaGetLastError());

        throw std::runtime_error(
            std::format("CUDA runtime error({}): {}", static_cast<unsigned int>(status), cudaGetErrorName(status)));
    }
}
```
