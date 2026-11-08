# Project 2 开发指南

作业要求见 `ASSIGNMENT.md`，使用方式见 `README.md`。

## 范围

- 必做：OptiX 路径追踪、OpenGL 交互显示、离线 PNG、渐进采样、报告。
- Bonus：OptiX denoising / temporal denoising，已实现。
- 项目代码和资源留在 `projects/project2`；只有通用功能才放进根目录 `base/`，且不能破坏其他项目。
- 保留 `project2`、`submit_project2` 目标和现有 CMake preset。

## 实现状态

- 已完成：`src/optix_programs.cu` 的天空盒采样与 throughput 加权、三种材质散射和 triangle/sphere closest-hit。
- 已完成：`src/optix_renderer.cpp` 的每像素资源分配、launch params、逐 sample launch、累积和 frame/sample 状态。
- 已完成 Bonus：`TEMPORAL_AOV` denoiser，albedo/normal guide 沿镜面路径追到第一个 Lambertian 表面或天空；raygen 写 surface position、flow 和 trustworthiness；renderer 保存上一帧输出、internal guide 和相机作为 history。resize 或某帧未降噪时 `resetTemporalHistory`。`--denoiser` 默认开启，验证 raw 输出时使用 `--denoiser off`。

## 不变量

- Hitgroup record 0/1/2 对应三个 Lucy 实例（共享法线指针，各自 `Material`），3 对应 sphere 实例（每球材质数组）。顺序不能打乱。
- pipeline `maxTraceDepth` 为 1，bounce（最多 16 次）在 raygen 内循环完成。
- 小球布局使用 `std::mt19937` 种子 42，不要改，否则画面无法对比。
- `Scene` 的 GPU buffer 必须比 renderer 和 OptiX 调用活得更久。
- 累积和 raw output 始终是线性浮点；sRGB 解码只在天空盒采样处，编码只在 OpenGL 显示或 PNG 输出处。
- CUDA 只在 OpenGL buffer map 期间写入，OpenGL 只在 unmap 后读取。
- 修改 `include/launch_params.h`（host/device 共享 ABI）后，C++ 目标和 OptiX IR 都要重新编译。
- 降噪结果只写 `pixels`，raw 平均值留在 accumulation，不能覆盖。guide 只在降噪开启时写入，所以 viewer 打开降噪时要把 sample 清零。
- 中文 MSVC 下 Ninja 不识别头文件依赖，修改任何头文件后必须 `--clean-first` 全量重编，否则类布局不一致会导致堆损坏崩溃。
- 资源路径由可执行文件位置推导；PNG 的相对输出路径则相对于当前工作目录，不要改。
- 相机状态（`camera.*`）与控制逻辑（`camera_controller.*`）保持分离。

## 构建与运行

在仓库根目录、Developer PowerShell for VS 中执行：

```powershell
cmake --preset windows-vs-release-vcpkg
cmake --build --preset windows-vs-release-vcpkg --target project2

$exe = ".\out\build\windows-vs-release-vcpkg\projects\project2\project2.exe"
& $exe                                    # 交互模式
& $exe --offline --width 320 --height 180 --samples 64 --denoiser off --output sample-64.png
```

Debug 使用 `windows-vs-debug-vcpkg`，Linux 使用 `linux-gcc-{release,debug}-vcpkg`。不依赖 OpenGL 的构建加 `-DPROJECT2_OFFLINE_ONLY=ON` 重新配置，此时只支持 `--offline`；恢复需显式设为 `OFF`。

## 打包

```powershell
cmake --build --preset windows-vs-release-vcpkg --target submit_project2
# 产物：out/build/<preset>/projects/project2/submission/project2.zip
```

- 打包只复制源码，不会编译；ZIP 生成成功不代表代码能编译或渲染正确。
- 包含 `CMakeLists.txt`、`include/`、`src/`、`report/`，以及 `PROJECT2_BASE_FILES` 白名单中的 `base/` 文件。
- 报告截图必须放在 `report/`，顶层 `screenshots/` 不会打包。
- 新用到的 `base/` 文件要加入 `PROJECT2_BASE_FILES`；新增 `src/`、`include/` 文件要加入 CMake 源文件列表。
- 不要提交 `assets/`、构建产物、生成的 `project2.optixir` 或机器相关文件。
