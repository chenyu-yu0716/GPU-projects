# Project 1 基线评测：RTX 3060 Laptop

本文记录优化开始之前的性能基线，以及对瓶颈的逐段测量。后续每一步优化都应当用
同样的方法重新测量，并和这里的数字对比。

可以尝试的优化方向见 [optimization-candidates.md](optimization-candidates.md)。

## 结论

- 完整动画（1280x720，2x2 抗锯齿，4501 帧）渲染耗时 **2980 秒（约 49.7 分钟）**，
  平均每帧 0.66 秒。RTX 5060 Laptop 上的基线约 37 分钟，3060 慢约 1.34 倍。
- 时间的 **97.5% 花在 kernel 迭代上**。显存回传占 0.3%，CPU 端的格式转换和编码
  合计约 2.2%。
- kernel 内部的瓶颈是 **FP64（double）运算**：FP64 流水线占用率 82–85%，而指令
  发射槽只有 1.92% 的时间在忙。
- warp 内线程空等不是主要问题：32 个线程里平均有 27–32 个处于活跃状态。
- 动画第 78 秒（zoom 约 `1e13`）之后 double 精度不足，画面是条纹；这一段占了
  总渲染时间的 55%。

## 测试环境

| 项目 | 值 |
| --- | --- |
| GPU | NVIDIA GeForce RTX 3060 Laptop（GA106，compute capability 8.6，6144 MB，30 个 SM） |
| CPU | Intel Core i9-12900H（20 线程） |
| 系统 | Ubuntu 24.04.5，WSL2（内核 6.18.40.1） |
| 驱动 / CUDA | 610.88 / CUDA Toolkit 13.3（nvcc 13.3.73） |
| 编译器 | GCC 13.3.0 |
| 构建 | 本地 preset `linux-gcc-release-3060`（Release，`CUDA_ARCHITECTURES native`） |
| 代码版本 | `542e9db` |
| 工具 | Nsight Systems 2026.1.3，Nsight Compute 2026.2.1，ffmpeg 6.1.1 |
| 日期 | 2026-10-08 |

## 测量前提

WSL2 下有两处限制，重现实验前需要知道：

- **交互窗口模式不可用。** WSL 的 OpenGL 由 `llvmpipe` 软件渲染，`cudaGLGetDevices`
  返回 `cudaErrorOperatingSystem`。所以本文全部使用 `project1 --offline` 和
  `project1_video`。
- **`ncu` 需要在 Windows 端授权（本机已开启）。** 授权方法是在 NVIDIA 控制面板里
  启用开发者设置，并允许所有用户访问 GPU 性能计数器。本机已经这样设置过，`ncu`
  可以直接使用。换机器或重装驱动后如果报 `ERR_NVGPUCTRPERM`，需要重新设置一次；
  在 WSL 里用 `sudo` 无效。

## 实验 1：单帧离线渲染

`project1 --offline`，程序自报的渲染时间。

| 参数 | 耗时 |
| --- | --- |
| 默认（1920x1080，2x2，zoom `1.6E22`） | 1.67–1.72 s（3 次） |
| 1280x720，2x2，zoom `1.6E22` | 0.75 s |
| 1280x720，2x2，中心 `-0.5+0i`，zoom 1 | 0.011 s |

默认参数的输出是横条纹，zoom 1 的输出是正确的 Mandelbrot 集。条纹是 double 精度
不足造成的，和显卡无关。

## 实验 2：完整动画渲染

```sh
project1_video --output mandelbrot_3060.mp4
```

4501 帧，总计 2980.2 秒。输出经 `ffprobe` 核对：1280x720、30 fps、4501 帧、
150.03 秒。渲染期间 GPU 占用 100%，显存 1287 MiB，温度 77°C。

按动画时间每 10 秒一段的耗时：

| 动画时间 | 段末 zoom | 段末 maxIter | 本段耗时 | 累计 |
| --- | --- | --- | --- | --- |
| 0–10 s | 2.0e4 | 2081 | 128.7 s | 128.8 s |
| 10–20 s | 3.7e5 | 2625 | 239.4 s | 368.2 s |
| 20–30 s | 7.1e6 | 3169 | 256.2 s | 624.4 s |
| 30–40 s | 1.4e8 | 3713 | 95.1 s | 719.5 s |
| 40–50 s | 2.6e9 | 4257 | 114.4 s | 833.9 s |
| 50–60 s | 4.9e10 | 4801 | 120.1 s | 954.0 s |
| 60–70 s | 9.3e11 | 5345 | 237.0 s | 1191.0 s |
| 70–80 s | 1.8e13 | 5889 | 166.3 s | 1357.3 s |
| 80–90 s | 3.4e14 | 6433 | 156.4 s | 1513.7 s |
| 90–100 s | 6.4e15 | 6977 | 215.4 s | 1729.1 s |
| 100–110 s | 1.2e17 | 7521 | 207.4 s | 1936.5 s |
| 110–120 s | 2.3e18 | 8065 | 230.8 s | 2167.3 s |
| 120–130 s | 4.4e19 | 8609 | 324.8 s | 2492.1 s |
| 130–140 s | 8.4e20 | 9153 | 257.5 s | 2749.6 s |
| 140–150 s | 1.6e22 | 9697 | 230.4 s | 2980.0 s |

- 每段耗时并不随 `maxIter` 单调增长（10–30 秒比 30–60 秒慢一倍多），说明耗时
  主要取决于画面里有多少像素跑满 `maxIter`，而不只是 `maxIter` 本身。
- 第 78 秒时累计 1330.1 秒。之后的 72 秒动画用了 1650 秒，占总时间的 55%，而
  这一段的画面已经不正确。

### 估算方法的验证

正式渲染前先用低分辨率采样估算了总时间，记录在这里供以后复用：

- 采样：`project1_video --width 640 --height 360 --fps 3`，451 帧，76.5 秒。
- 按帧数（4501/451）和像素数（4 倍）线性折算：约 3054 秒。
- 实际 2980 秒，偏差 2.4%。

所以用 1/4 分辨率、1/10 帧率的采样可以在 1–2 分钟内估出完整渲染时间，误差在
几个百分点以内。

## 实验 3：逐段时间分解（Nsight Systems）

```sh
nsys profile --trace=cuda,osrt --sample=none --cpuctxsw=none \
    -o nsys_video project1_video --fps 1 --output v_fps1.mp4
nsys stats --report cuda_gpu_kern_sum --report cuda_gpu_mem_time_sum \
    --report cuda_api_sum --report osrt_sum nsys_video.nsys-rep
```

1280x720、2x2 抗锯齿、1 fps，共 151 帧，覆盖整段动画。

| 阶段 | 累计 | 占比 | 平均每帧 |
| --- | --- | --- | --- |
| `renderMandelbrotSetKernel` | 106.0 s | 97.5% | 702 ms |
| 显存回传（Device-to-Host memcpy） | 0.33 s | 0.3% | 2.2 ms |
| 其余（`float4` 转 RGBA、写入 ffmpeg 管道等） | 约 2.4 s | 2.2% | 约 16 ms |

- 占比的分母是 `nsys` 记录的 kernel 时间跨度 108.74 秒。
- kernel 单帧耗时：最小 91 ms，中位数 752 ms，最大 1231 ms。
- `cudaMemcpy` 的 API 调用时间是 106.5 秒，几乎等于 kernel 时间。这是因为 kernel
  异步启动，`cudaMemcpy` 在等它跑完；真正的拷贝只有 2.2 ms。
- `fwrite` 累计 0.81 秒（约 5 ms/帧）。

**未解决的出入：** 程序自报的总时间是 99.0 秒，`nsys` 记录的跨度是 108.7 秒，
相差约 10%，原因没有查清。占比是在 `nsys` 同一套时钟里算的，不受影响；绝对
毫秒数应当保守看待。

## 实验 4：kernel 内部剖析（Nsight Compute）

### `ncu` 用法简介

`nsys` 回答“时间花在哪个阶段”，`ncu`（Nsight Compute 的命令行）回答“某一次
kernel 启动内部发生了什么”。它的基本形式是把要剖析的程序和参数接在 `ncu` 的
选项后面：

```sh
ncu [ncu 选项] <程序> [程序参数]
```

`ncu` 会拦截程序里的 kernel 启动，把被选中的那次重放多遍，每遍采集一组硬件
计数器。所以剖析状态下的 kernel 耗时比正常运行长，不能当作性能数字用，只看
各项指标的比例。

本实验用到的选项：

| 选项 | 作用 |
| --- | --- |
| `--section <名称>` | 只采集指定的 section，可重复写多个。不写时采集 `default` 集合 |
| `--launch-skip N` | 跳过前 N 次 kernel 启动，从第 N+1 次开始剖析 |
| `--launch-count N` | 只剖析 N 次启动 |
| `--kill yes` | 剖析完指定次数后直接结束程序，不等它跑完 |
| `-o <文件名>` | 把结果存成 `<文件名>.ncu-rep`；已有同名文件时加 `-f` 覆盖 |

本实验选的 section：

| section | 看什么 |
| --- | --- |
| `SpeedOfLight` | 总览：SM 和内存的吞吐率相对理论上限的百分比 |
| `ComputeWorkloadAnalysis` | 各条运算流水线（FP64、ALU 等）的占用率，IPC，Issue Slots Busy |
| `SchedulerStats` | 调度器每个周期有多少 warp 可发射、实际发射多少 |
| `WarpStateStats` | warp 在等什么，每条指令平均等多少周期 |
| `InstructionStats` | 执行的指令数，FP64 指令已融合 / 未融合的数量 |
| `Occupancy` | 理论和实际 occupancy，以及受什么资源限制 |
| `LaunchStats` | grid / block 大小，每线程寄存器数 |

`project1_video` 每帧启动一次 kernel，所以在 `--fps 1` 下 `--launch-skip N` 选中
的就是动画第 N 秒的帧。配合 `--launch-count 1 --kill yes`，采完这一帧就退出，
不必渲染完整段动画。

查看结果和其他常用命令：

```sh
ncu --import ncu_t20.ncu-rep                    # 在终端打印已保存的报告
ncu --import ncu_t20.ncu-rep --page raw --csv   # 导出全部原始指标为 CSV
ncu-ui ncu_t20.ncu-rep                          # 图形界面
ncu --list-sections                             # 列出可用的 section
ncu --list-sets                                 # 列出预定义的 section 集合
ncu --set full -o <文件名> <程序> [程序参数]    # 采集全部 section（更慢）
```

### 本实验的命令

```sh
# 动画第 20 秒和第 60 秒的帧（1 fps 下第 N 次 kernel 启动即第 N 秒）
ncu --section SpeedOfLight --section Occupancy --section WarpStateStats \
    --section InstructionStats --section SchedulerStats \
    --section ComputeWorkloadAnalysis --section LaunchStats \
    --launch-skip 20 --launch-count 1 --kill yes \
    -o ncu_t20 project1_video --fps 1 --output v_ncu.mp4

# 结尾帧：离线渲染器的默认中心和 zoom 与动画最后一帧相同
ncu <同上的 --section 参数> -o ncu_t150 \
    project1 --offline --width 1280 --height 720 --output t150.png
```

第 60 秒的帧把第一条命令里的 `--launch-skip 20` 和 `-o ncu_t20` 换成
`--launch-skip 60` 和 `-o ncu_t60`。

### 结果

每帧都是 1280x720、2x2 抗锯齿，block 16x16，grid 3600 个 block。

| 指标 | 第 20 秒 | 第 60 秒 | 第 150 秒 |
| --- | --- | --- | --- |
| kernel 耗时（剖析状态下） | 1.18 s | 0.90 s | 1.01 s |
| Compute (SM) Throughput | 84.96% | 81.89% | 84.63% |
| FP64 流水线占用率 | 85.0% | 81.9% | 84.6% |
| Issue Slots Busy | 1.92% | 1.92% | 1.92% |
| Executed IPC（活跃周期） | 0.08 | 0.08 | 0.08 |
| Warp Cycles Per Issued Instruction | 494.4 | 392.7 | 493.0 |
| 其中等待解耦数学运算队列的周期 | 346.5 | 260.9 | 342.9 |
| Avg. Active Threads Per Warp | 31.72 | 27.40 | 31.92 |
| Avg. Not Predicated Off Threads Per Warp | 29.28 | 25.30 | 29.47 |
| 执行指令数 | 3.83e9 | 2.61e9 | 3.33e9 |
| FP64 指令：已融合 / 未融合 | 2.93e8 / 2.05e9 | 1.93e8 / 1.35e9 | 2.55e8 / 1.78e9 |
| `ncu` 估计的 FP64 融合加速上限 | 37.17% | 35.82% | 37.02% |
| Registers Per Thread | 42 | 42 | 42 |
| 理论 / 实际 Occupancy | 83.33% / 79.84% | 83.33% / 66.76% | 83.33% / 79.54% |
| Memory Throughput | 1.02% | — | — |

怎么读这些数字：

- **FP64 是瓶颈。** 调度器 98% 的周期里没有可发射的 warp，warp 平均等 400–500 个
  周期才能发出下一条指令，其中约 70% 的等待是在排 FP64 运算的队。三帧几乎一样，
  说明这是全程的问题。
- **warp 发散的损失很小。** 活跃线程数接近满值 32。损失最大的是第 60 秒的帧，
  约 14%。
- **约 87% 的 FP64 指令没有融合成 FMA。** `ncu` 给出的 36–37% 是估计的上限，不是
  实测收益。
- **内存不是瓶颈。** Memory Throughput 约 1%，kernel 几乎是纯计算。
- **Occupancy 受寄存器限制。** 每线程 42 个寄存器使每个 SM 最多放 5 个 block
  （按 warp 数算可以放 6 个）。在 FP64 排队的情况下，提高 occupancy 预计帮助
  有限。

**没采到的数据：** 原计划采第 130 秒的帧，但那次 `ncu` 运行卡住了（GPU 占用
降到 1%，约十分钟没有进展），原因没有查。改用结尾帧代替。

## 原始数据

原始报告没有放进仓库（二进制文件，且 `out/` 已被忽略）。本机上的位置：

```text
out/eval-3060/
├── video_3060.log       实验 2 的完整进度日志
├── nsys_video.nsys-rep  实验 3，可用 nsys-ui 或 nsys stats 打开
├── ncu_t20.ncu-rep      实验 4，可用 ncu-ui 或 ncu --import 打开
├── ncu_t60.ncu-rep
└── ncu_t150.ncu-rep
```

实验 2 的视频在 `out/mandelbrot_3060.mp4`。
