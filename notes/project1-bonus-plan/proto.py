# Prototype for the project1 bonus plan: perturbation with rebase in float64,
# compared with a 70-digit truth at random pixels of animation frames.
# Run: python3 proto.py [samples] [t1,t2,...]   (standard library only)
import math, random, sys
from decimal import Decimal as D, getcontext
getcontext().prec = 70

START_C = (D("-1.4184"), D(0)); START_Z = D(1035)
FINAL_C = (D("-1.416707803560595223063379502205564140068277553325999761"),
           D("0.000000000000000000000001192699352575212153707731000000"))
FINAL_Z = D("1.6E22")
DURATION = 150.0
W, H = 640, 360
BAIL = 256.0

def max_iter(zoom):
    return int(min(256 + 128 * math.log2(max(float(zoom), 1.0)), 20000))

def camera(t):
    n = min(max(t / DURATION, 0.0), 1.0)
    if n <= 0: zoom = START_Z
    elif n >= 1: zoom = FINAL_Z
    else:
        lz = math.log10(float(START_Z)) + (math.log10(float(FINAL_Z)) - math.log10(float(START_Z))) * n
        zoom = D(10.0 ** lz)
    decay = (START_Z / zoom) * D((1 - n) * (1 - n))
    c = (FINAL_C[0] + (START_C[0] - FINAL_C[0]) * decay, FINAL_C[1] + (START_C[1] - FINAL_C[1]) * decay)
    return c, zoom

def reference_orbit(center, n):
    orbit = []; zr = D(0); zi = D(0)
    for i in range(n + 1):
        r = complex(float(zr), float(zi)); orbit.append(r)
        if r.real * r.real + r.imag * r.imag > BAIL: break
        zr, zi = zr * zr - zi * zi + center[0], 2 * zr * zi + center[1]
    return orbit

def truth(c, n):
    zr = D(0); zi = D(0)
    for i in range(n):
        r2 = zr * zr + zi * zi
        if r2 > 256: return i, float(r2)
        zr, zi = zr * zr - zi * zi + c[0], 2 * zr * zi + c[1]
    return n, None

def perturb(orbit, dc, n):
    L = len(orbit); d = 0j; m = 0; rebases = 0
    for i in range(n):
        z = orbit[m] + d
        r2 = z.real * z.real + z.imag * z.imag
        if r2 > BAIL: return i, r2, rebases
        if r2 < d.real * d.real + d.imag * d.imag or m == L - 1:
            d = z; m = 0; rebases += 1
        d = (2 * orbit[m] + d) * d + dc
        m += 1
    return n, None, rebases

def direct(c, n):
    z = 0j
    for i in range(n):
        r2 = z.real * z.real + z.imag * z.imag
        if r2 > BAIL: return i, r2
        z = z * z + c
    return n, None

def smooth(i, r2):
    return None if r2 is None else i + 1 - math.log2(0.5 * math.log(r2))

random.seed(1)
TS = [float(v) for v in sys.argv[2].split(",")] if len(sys.argv) > 2 else [0, 30, 60, 78, 100, 130, 150]
samples = int(sys.argv[1]) if len(sys.argv) > 1 else 40
orbit_final = reference_orbit(FINAL_C, max_iter(FINAL_Z))
print(f"reference orbit at the final center: {len(orbit_final)} entries, "
      f"escaped: {abs(orbit_final[-1])**2 > BAIL}")
print(f"{samples} random samples for each frame at {W}x{H}; wrong = smooth iteration differs from truth by > 0.05")
print("   t     zoom  maxIter  delta/viewH  rebase/px | perturbation    | direct")
print("                                               | wrong  worst    | wrong  worst")
for t in TS:
    c, zoom = camera(t); n = max_iter(zoom)
    ps_hp = D(4) / (zoom * D(H)); ps = float(ps_hp)
    delta = (float(c[0] - FINAL_C[0]), float(c[1] - FINAL_C[1]))
    cd = (float(c[0]), float(c[1]))
    bad_p = bad_d = 0; worst_p = worst_d = 0.0; reb = 0
    for _ in range(samples):
        x = random.randrange(W); y = random.randrange(H)
        ox = (x + 0.5 - 0.5 * W); oy = (y + 0.5 - 0.5 * H)
        ct = (c[0] + D(ox) * ps_hp, c[1] + D(oy) * ps_hp)
        ti, tr = truth(ct, n); ts = smooth(ti, tr)
        pi, pr, rb = perturb(orbit_final, complex(delta[0] + ox * ps, delta[1] + oy * ps), n); psm = smooth(pi, pr)
        di, dr = direct(complex(cd[0] + ox * ps, cd[1] + oy * ps), n); dsm = smooth(di, dr)
        reb += rb
        for kind, s in (("p", psm), ("d", dsm)):
            if (s is None) != (ts is None): err = float("inf")
            elif s is None: err = 0.0
            else: err = abs(s - ts)
            if kind == "p": worst_p = max(worst_p, err); bad_p += err > 0.05
            else: worst_d = max(worst_d, err); bad_d += err > 0.05
    print(f"{t:4.0f}  {float(zoom):7.1e}  {n:7d}  {abs(complex(*delta))/(ps*H):11.3f}  {reb/samples:9.1f} | "
          f"{bad_p:5d}  {worst_p:7.1e}  | {bad_d:5d}  {worst_d:7.1e}")
