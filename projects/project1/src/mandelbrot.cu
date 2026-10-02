#include <mandelbrot.h>
#include <cuda_runtime.h>


template <typename Real>
__global__ void renderMandelbrotSetKernel(
    float4* __restrict__ output,
    Complex<Real> const* __restrict__ referenceOrbit,
    Complex<Real> const* __restrict__ seriesCoeff,
    RenderParameters<Real> const& params) {

}

template <typename Real>
void renderMandelbrotSet(
    float4* output,
    Complex<Real> const* referenceOrbit,
    Complex<Real> const* seriesCoeff,
    RenderParameters<Real> const& params) {
    dim3 blockSize(16, 16);
    dim3 gridSize((params.width + blockSize.x - 1) / blockSize.x,
                  (params.height + blockSize.y - 1) / blockSize.y);
    renderMandelbrotSetKernel<<<gridSize, blockSize>>>(output, referenceOrbit, seriesCoeff, params);
    cudaDeviceSynchronize();
}