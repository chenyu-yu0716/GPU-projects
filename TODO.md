# TODO

## Project 1: Mandelbrot

- [ ] **解决深度放大的精度问题**（作业可选 bonus）
  - 现象：zoom 超过约 `1e13` 后，`double` 精度不够，画面逐渐变成色块和横条纹；动画 150 秒中大约从第 78 秒开始出现。
  - 做法：perturbation。CPU 用高精度在动画终点算一条 reference orbit，kernel 里每个像素只迭代相对它的偏移，偏移过大时 rebase。
  - 计划：[notes/project1-bonus-plan.html](notes/project1-bonus-plan.html)（用浏览器打开；有三个决定待确认）。页面源文件和 Python 原型在 `notes/project1-bonus-plan/`。
  - 本轮不做：series approximation 和任何加速，bonus 完成后再考虑。

- [ ] **优化渲染速度**
  - 基线（RTX 5060 Laptop，1280x720，2x2 抗锯齿）：整段动画平均约 0.5 秒一帧，`project1_video` 渲染 4501 帧约需 37 分钟。
  - 先测量再优化：确认时间花在 kernel 迭代上，还是花在回传 CPU 和编码上。
  - 待评估的方向：
    - `maxIter` 公式的系数是否偏大（`computeMaxIterations`，目前 `256 + 128 * log2(zoom)`）。
    - 集合内部的像素每个都跑满 `maxIter`：可以做周期检测，或对主心形和 period-2 圆做解析判断后直接跳过。
    - block 大小（目前 16x16）以及同一 block 内迭代次数差异大造成的浪费。
    - 2x2 抗锯齿是 4 倍开销：可以只对边界像素做多采样。
    - `project1_video` 中 `float4` 转 RGBA 字节目前在 CPU 上逐像素做，可以挪到 GPU。
