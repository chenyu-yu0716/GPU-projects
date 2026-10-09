# Prototype render for the project1 bonus plan: perturbation with rebase in
# float64 (numpy), colored with the palette table from palette.cu.
# Run: uv run --with numpy --with pillow python render.py 1.6E22 640 360 out.png
import math, pathlib, re, sys
import numpy as np
from PIL import Image
from decimal import Decimal as D, getcontext
getcontext().prec = 70
FINAL_C = (D("-1.416707803560595223063379502205564140068277553325999761"),
           D("0.000000000000000000000001192699352575212153707731000000"))
zoom = D(sys.argv[1]); W, H = int(sys.argv[2]), int(sys.argv[3]); out = sys.argv[4]
n = int(min(256 + 128 * math.log2(float(zoom)), 20000))
zr = D(0); zi = D(0); orbit = []
for i in range(n + 1):
    r = complex(float(zr), float(zi)); orbit.append(r)
    if abs(r) ** 2 > 256: break
    zr, zi = zr * zr - zi * zi + FINAL_C[0], 2 * zr * zi + FINAL_C[1]
orbit = np.array(orbit); L = len(orbit)
ps = float(D(4) / (zoom * D(H)))
xs = (np.arange(W, dtype=np.float32) + np.float32(0.5)).astype(np.float64) - 0.5 * W
ys = (np.arange(H, dtype=np.float32) + np.float32(0.5)).astype(np.float64) - 0.5 * H
dc = (xs[None, :] * ps + 1j * ys[:, None] * ps).ravel()
idx = np.arange(dc.size); d = np.zeros_like(dc); m = np.zeros(dc.size, dtype=np.int64)
smooth = np.full(dc.size, np.nan, dtype=np.float32)
for i in range(n):
    z = orbit[m] + d
    r2 = z.real * z.real + z.imag * z.imag
    esc = r2 > 256.0
    if esc.any():
        smooth[idx[esc]] = (i + 1 - np.log2(0.5 * np.log(r2[esc].astype(np.float32)))).astype(np.float32)
        keep = ~esc; idx = idx[keep]; d = d[keep]; m = m[keep]; z = z[keep]; r2 = r2[keep]; dcl = dc[idx]
        if idx.size == 0: break
    else:
        dcl = dc[idx]
    reb = (r2 < d.real * d.real + d.imag * d.imag) | (m == L - 1)
    d = np.where(reb, z, d); m = np.where(reb, 0, m)
    d = (2 * orbit[m] + d) * d + dcl
    m = m + 1
src = (pathlib.Path(__file__).resolve().parents[2] / "projects/project1/src/palette.cu").read_text()
cols = np.array([[float(v) for v in t] for t in re.findall(r"float3\{([\d.]+)f, ([\d.]+)f, ([\d.]+)f\}", src)], dtype=np.float32)
assert cols.shape == (256, 3), cols.shape
u = smooth / np.float32(1022.395737721); u = u - np.floor(u)
texel = u * 256 - 0.5; left = np.floor(texel); w = (texel - left)[:, None]
i0 = np.where(left < 0, left + 256, left); i0 = np.nan_to_num(i0).astype(int); i1 = (i0 + 1) % 256
rgb = cols[i0] + w * (cols[i1] - cols[i0]); rgb[np.isnan(smooth)] = 0
img = (np.clip(rgb, 0, 1) * 255 + 0.5).astype(np.uint8).reshape(H, W, 3)[::-1]
Image.fromarray(img).save(out)
print(f"orbit {L}, maxIter {n}, inside {np.isnan(smooth).mean():.3f}")
