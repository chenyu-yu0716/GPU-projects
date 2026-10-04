#pragma once

#include <type_traits>

#include <complex.hpp>
#include <palette.h>

struct RenderParameters {
    int width = 0;
    int height = 0;
    int maxIter = 0;
    double pixelScale = 0.0;
    bool antialias = false;

    Palette palette{};

    Complex directCenter{};
    Complex cameraDelta{};

    // TODO: Add necessary data fields here
};

static_assert(std::is_trivially_copyable_v<Palette>);
static_assert(std::is_trivially_copyable_v<RenderParameters>);
