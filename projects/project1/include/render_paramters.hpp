#pragma once

#include <complex.hpp>
#include <palette.h>

template <typename Real>
struct RenderParameters {
    int width = 0;
    int height = 0;
    int maxIter = 0;
    int referenceCount = 0;
    int seriesCoeffCount = 0;
    int usePerturbation = 0;
    int seriesBase = 0;
    int seriesOrder = 0;
    int seriesJump = 0;
    int antialias = 1;
    double pixelScale = 0.0;
    double seriesRadius = 1.0;
    Complex<Real> directCenter{};
    Complex<Real> cameraDelta{};
};