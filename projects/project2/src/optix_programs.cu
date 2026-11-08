#include <cstdint>
#include <launch_params.h>
#include <math.h>
#include <optix_device.h>

extern "C" {
__constant__ LaunchParams params;
}

static constexpr uint32_t kMaxBounces = 16;
static constexpr float kSkyIllumination = 1.4f;
static constexpr float kTanHalfFovY = 0.5773502692f;

// Reprojected surface points farther apart than this fraction of their camera
// distance are treated as disoccluded.
static constexpr float kReprojectionTolerance = 0.02f;

struct Ray {
    float3 origin;
    float3 direction;
};

struct Hit {
    float3 normal;
    Material material;
    float distance;
    uint32_t surfaceId;
    bool found;
};

// Basic vector operations used by the ray programs.
static __device__ float3 operator+(float3 a, float3 b) {
    return float3{a.x + b.x, a.y + b.y, a.z + b.z};
}

static __device__ float3 operator-(float3 a, float3 b) {
    return float3{a.x - b.x, a.y - b.y, a.z - b.z};
}

static __device__ float3 operator*(float3 a, float scalar) {
    return float3{a.x * scalar, a.y * scalar, a.z * scalar};
}

static __device__ float3 operator*(float3 a, float3 b) {
    return float3{a.x * b.x, a.y * b.y, a.z * b.z};
}

static __device__ float3 operator*(float scalar, float3 a) {
    return a * scalar;
}

static __device__ float4 operator+(float4 a, float4 b) {
    return float4{a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

static __device__ float4 operator-(float4 a, float4 b) {
    return float4{a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

static __device__ float4 operator*(float4 a, float scalar) {
    return float4{a.x * scalar, a.y * scalar, a.z * scalar, a.w * scalar};
}

static __device__ float4 operator*(float scalar, float4 a) {
    return a * scalar;
}

static __device__ float dot(float3 a, float3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static __device__ float3 cross(float3 a, float3 b) {
    return float3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static __device__ float3 unit(float3 a) {
    return a * rsqrtf(fmaxf(dot(a, a), 1e-30f));
}

// Random directions for the material scattering routines.
static __device__ uint32_t randomBits(uint32_t& s) {
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}

static __device__ float random01(uint32_t& s) {
    return (randomBits(s) >> 8) * (1.0f / 16777216.0f);
}

static __device__ float3 randomUnit(uint32_t& s) {
    float z = 2 * random01(s) - 1;
    float angle = 6.283185307f * random01(s);
    float r = sqrtf(fmaxf(0, 1 - z * z));
    return float3{r * __cosf(angle), r * __sinf(angle), z};
}

static __device__ float3 cosineHemisphere(float3 n, uint32_t& s) {
    float3 tangent = unit(cross(fabsf(n.y) < .99f ? float3{0, 1, 0} : float3{1, 0, 0}, n));
    float3 bitangent = cross(n, tangent);
    float r = sqrtf(random01(s)), a = 6.283185307f * random01(s);
    return unit(tangent * (r * __cosf(a)) + bitangent * (r * __sinf(a)) + sqrtf(fmaxf(0, 1 - r * r)) * n);
}

// Convert sampled skybox color from sRGB to linear.
static __device__ float3 srgbToLinear(float3 color) {
    return float3{
        color.x <= 0.04045f ? color.x / 12.92f : powf((color.x + 0.055f) / 1.055f, 2.4f),
        color.y <= 0.04045f ? color.y / 12.92f : powf((color.y + 0.055f) / 1.055f, 2.4f),
        color.z <= 0.04045f ? color.z / 12.92f : powf((color.z + 0.055f) / 1.055f, 2.4f),
    };
}

// Read and update the hit payload passed through optixTrace.
static __device__ Hit* payloadHit() {
    uint64_t p = (static_cast<uint64_t>(optixGetPayload_1()) << 32) | optixGetPayload_0();
    return reinterpret_cast<Hit*>(p);
}

static __device__ void storeHit(float3 normal, Material material, uint32_t surfaceId) {
    Hit* h = payloadHit();
    h->normal = unit(normal);
    h->material = material;
    h->distance = optixGetRayTmax();
    h->surfaceId = surfaceId;
    h->found = true;
}

static __device__ Hit trace(Ray const& ray) {
    Hit hit{};
    uint64_t pointer = reinterpret_cast<uint64_t>(&hit);
    uint32_t low = static_cast<uint32_t>(pointer);
    uint32_t high = static_cast<uint32_t>(pointer >> 32);
    optixTrace(params.scene,
               ray.origin,
               ray.direction,
               .001f,
               1e16f,
               0,
               OptixVisibilityMask(255),
               OPTIX_RAY_FLAG_NONE,
               0,
               1,
               0,
               low,
               high);
    return hit;
}

// Project a world-space point, or a sky direction, into the previous camera's
// pixel coordinates using the same mapping as the primary rays.
static __device__ bool projectToPreviousPixel(float3 position, bool isSky, float2& pixel) {
    float3 const direction = isSky ? position : position - params.previousCameraPosition;
    float const depth = dot(direction, params.previousFront);
    if (depth <= 1e-6f) {
        return false;
    }

    float const aspect = float(params.width) / float(params.height);
    float const cameraPlaneX = dot(direction, params.previousRight) / depth;
    float const cameraPlaneY = dot(direction, params.previousUp) / depth;
    pixel.x = (cameraPlaneX / (aspect * kTanHalfFovY) + 1) * 0.5f * params.width;
    pixel.y = (cameraPlaneY / kTanHalfFovY + 1) * 0.5f * params.height;
    return pixel.x >= 0 && pixel.x < params.width && pixel.y >= 0 && pixel.y < params.height;
}

// Store the primary surface and its screen-space motion since the previous frame.
// Flow is trusted only where the previous frame saw the same surface there.
static __device__ void writeMotionGuides(uint32_t pixel, float2 samplePosition, float3 position, uint32_t surfaceId) {
    params.surfacePositions[pixel] = float4{position.x, position.y, position.z, float(surfaceId)};

    float2 flow{0, 0};
    float trustworthiness = 0;
    float2 previousPixel;
    bool const isSky = surfaceId == 0;
    if (params.computeFlow && projectToPreviousPixel(position, isSky, previousPixel)) {
        // OptiX flow points from the previous pixel position to the current one.
        flow = float2{samplePosition.x - previousPixel.x, samplePosition.y - previousPixel.y};

        uint32_t const previousIndex = uint32_t(previousPixel.y) * params.width + uint32_t(previousPixel.x);
        float4 const previous = params.previousSurfacePositions[previousIndex];
        bool sameSurface = previous.w == float(surfaceId);
        if (sameSurface && !isSky) {
            float3 const offset = float3{previous.x, previous.y, previous.z} - position;
            float3 const view = position - params.cameraPosition;
            sameSurface = dot(offset, offset) < kReprojectionTolerance * kReprojectionTolerance * dot(view, view);
        }
        trustworthiness = sameSurface ? 1.0f : 0.0f;
    }
    params.flow[pixel] = flow;
    params.flowTrustworthiness[pixel] = trustworthiness;
}

// Trace one path and incrementally average its radiance per pixel.
extern "C" __global__ void __raygen__render() {
    uint3 index = optixGetLaunchIndex();
    uint32_t pixel = index.y * params.width + index.x;

    // Seed a per-pixel random sequence that changes with the frame.
    uint32_t randomState = (pixel + 1) * 747796405u ^ (params.frameIndex + 1) * 2891336453u;
    if (randomState == 0) {
        // xorshift32 stays at zero if its state starts at zero.
        randomState = 1;
    }

    // Jitter the sample within the pixel before mapping it to the camera plane.
    float2 const samplePosition{index.x + random01(randomState), index.y + random01(randomState)};
    float const cameraPlaneX =
        (2 * samplePosition.x / float(params.width) - 1) * float(params.width) / float(params.height) * kTanHalfFovY;

    float const cameraPlaneY = (2 * samplePosition.y / float(params.height) - 1) * kTanHalfFovY;

    // Construct the primary ray from the camera position and basis vectors.
    Ray ray{params.cameraPosition, unit(params.front + cameraPlaneX * params.right + cameraPlaneY * params.up)};

    // Track the path throughput and the resulting sample radiance.
    float3 pathThroughput{1, 1, 1};
    float3 radiance{0, 0, 0};

    // Denoiser guides follow mirror and glass bounces until the first diffuse
    // surface or the sky, so they describe what specular surfaces show. A
    // primary ray that hits the sky keeps a zero normal.
    float3 guideAlbedo{0, 0, 0};
    float3 guideNormal{0, 0, 0};
    bool guidesComplete = false;

    // Follow the path through up to kMaxBounces material bounces.
    for (uint32_t bounceIndex = 0; bounceIndex < kMaxBounces; ++bounceIndex) {
        Hit const hit = trace(ray);
        if (!hit.found) {
            float4 const sky = texCubemap<float4>(params.sky, ray.direction.x, ray.direction.y, ray.direction.z);
            float3 const skyColor = srgbToLinear(float3{sky.x, sky.y, sky.z});
            radiance = radiance + pathThroughput * skyColor * kSkyIllumination;
            if (!guidesComplete) {
                guideAlbedo = pathThroughput * skyColor;
            }
            if (params.writeGuides && bounceIndex == 0) {
                writeMotionGuides(pixel, samplePosition, ray.direction, 0);
            }
            break;
        }

        float3 const hitPoint = ray.origin + ray.direction * hit.distance;
        if (params.writeGuides && bounceIndex == 0) {
            writeMotionGuides(pixel, samplePosition, hitPoint, hit.surfaceId);
        }

        float3 const surfaceNormal = hit.normal;
        bool const isFrontFace = dot(ray.direction, surfaceNormal) < 0;
        float3 const orientedNormal = isFrontFace ? surfaceNormal : -1.0f * surfaceNormal;

        if (!guidesComplete) {
            // The throughput so far is the tint of the specular surfaces along the way.
            guideAlbedo = pathThroughput * hit.material.albedo;
            guideNormal = orientedNormal; // AOV denoiser models expect world-space normals.
            guidesComplete = hit.material.type == Lambertian;
        }

        pathThroughput = pathThroughput * hit.material.albedo;

        float3 scatteredDirection;
        if (hit.material.type == Lambertian) {
            scatteredDirection = cosineHemisphere(orientedNormal, randomState);
        }
        else if (hit.material.type == Metal) {
            float3 const reflected = ray.direction - 2.0f * dot(ray.direction, orientedNormal) * orientedNormal;
            scatteredDirection = unit(reflected + hit.material.fuzz * randomUnit(randomState));
            if (dot(scatteredDirection, orientedNormal) <= 0.0f) {
                break;
            }
        }
        else if (hit.material.type == Dielectric) {
            float3 const unitDirection = unit(ray.direction);
            float const refractionRatio = isFrontFace ? 1.0f / hit.material.ior : hit.material.ior;
            float const cosTheta = fminf(dot(-1.0f * unitDirection, orientedNormal), 1.0f);
            float const sinTheta = sqrtf(fmaxf(0.0f, 1.0f - cosTheta * cosTheta));
            float r0 = (1.0f - refractionRatio) / (1.0f + refractionRatio);
            r0 *= r0;
            float const reflectance = r0 + (1.0f - r0) * powf(1.0f - cosTheta, 5.0f);
            float const refractedSinTheta = refractionRatio * sinTheta;

            if (refractedSinTheta > 1.0f || random01(randomState) < reflectance) {
                scatteredDirection = unit(unitDirection - 2.0f * dot(unitDirection, orientedNormal) * orientedNormal);
            }
            else {
                float const factor =
                    refractionRatio * cosTheta - sqrtf(fmaxf(0.0f, 1.0f - refractedSinTheta * refractedSinTheta));
                scatteredDirection = unit(refractionRatio * unitDirection + factor * orientedNormal);
            }
        }
        else {
            // Unknown material type: treat the path as absorbed.
            break;
        }

        ray.origin = hitPoint + scatteredDirection * .001f;
        ray.direction = scatteredDirection;
    }

    // Update the running average and write the current sample.
    float4 old = params.sample ? params.accumulation[pixel] : float4{0, 0, 0, 1};
    float inv = 1.0f / (params.sample + 1);
    float4 current{radiance.x, radiance.y, radiance.z, 1};
    float4 value = old + (current - old) * inv;
    params.accumulation[pixel] = value;
    params.pixels[pixel] = value;

    // Average the guides over the same samples so they align with the accumulated radiance.
    if (params.writeGuides) {
        float4 const oldAlbedo = params.sample ? params.albedo[pixel] : float4{0, 0, 0, 1};
        float4 const currentAlbedo{guideAlbedo.x, guideAlbedo.y, guideAlbedo.z, 1};
        params.albedo[pixel] = oldAlbedo + (currentAlbedo - oldAlbedo) * inv;

        float3 const oldNormal = params.sample ? params.normal[pixel] : float3{0, 0, 0};
        params.normal[pixel] = oldNormal + (guideNormal - oldNormal) * inv;
    }
}

extern "C" __global__ void __miss__background() {}

// Store triangle and sphere intersections in the shared payload format.
//
// Surface IDs identify objects, not primitives: Lucy instances use
// instanceId + 1 (1..3), and each sphere uses kSphereSurfaceIdBase + its
// primitive index. The values stay small so they remain exact when stored in
// the float w channel of surfacePositions.
static constexpr uint32_t kSphereSurfaceIdBase = 4;

extern "C" __global__ void __closesthit__triangle() {
    auto const* data = reinterpret_cast<HitRecordData const*>(optixGetSbtDataPointer());
    uint32_t const primitiveIndex = optixGetPrimitiveIndex();
    float2 const barycentric = optixGetTriangleBarycentrics();
    float3 const* normals = data->normals + 3 * primitiveIndex;
    float3 const objectNormal =
        (1.0f - barycentric.x - barycentric.y) * normals[0] + barycentric.x * normals[1] + barycentric.y * normals[2];
    float3 const worldNormal = optixTransformNormalFromObjectToWorldSpace(objectNormal);
    storeHit(worldNormal, data->material, optixGetInstanceId() + 1);
}

extern "C" __global__ void __closesthit__sphere() {
    auto const* data = reinterpret_cast<HitRecordData const*>(optixGetSbtDataPointer());
    uint32_t const primitiveIndex = optixGetPrimitiveIndex();
    float4 sphereData[1];
    optixGetSphereData(sphereData);
    float3 const center{sphereData[0].x, sphereData[0].y, sphereData[0].z};
    // Sphere data is in object space, so move the hit point into object space
    // and transform the normal back; this stays correct if the instance gains
    // a transform. (Object-space ray getters are not allowed in closest-hit.)
    float3 const worldHitPoint = optixGetWorldRayOrigin() + optixGetWorldRayDirection() * optixGetRayTmax();
    float3 const objectHitPoint = optixTransformPointFromWorldToObjectSpace(worldHitPoint);
    float3 const objectNormal = (objectHitPoint - center) * (1.0f / sphereData[0].w);
    float3 const worldNormal = optixTransformNormalFromObjectToWorldSpace(objectNormal);
    storeHit(worldNormal, data->sphereMaterials[primitiveIndex], kSphereSurfaceIdBase + primitiveIndex);
}
