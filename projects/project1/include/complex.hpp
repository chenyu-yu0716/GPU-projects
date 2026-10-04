#pragma once

#include <high_precision_float.h>

template <typename T> struct TComplex {
public:
    T re;
    T im;

public:
    using Real = T;
};

using Complex = TComplex<double>;
using HighPrecisionComplex = TComplex<HighPrecisionFloat>;

template <typename T> inline constexpr TComplex<T> operator+(TComplex<T> lhs, TComplex<T> rhs) {
    return {lhs.re + rhs.re, lhs.im + rhs.im};
}

template <typename T> inline constexpr TComplex<T> operator-(TComplex<T> lhs, TComplex<T> rhs) {
    return {lhs.re - rhs.re, lhs.im - rhs.im};
}

template <typename T> inline constexpr TComplex<T> operator*(TComplex<T> lhs, TComplex<T> rhs) {
    return {lhs.re * rhs.re - lhs.im * rhs.im, lhs.re * rhs.im + lhs.im * rhs.re};
}

template <typename T> inline constexpr TComplex<T> operator*(TComplex<T> value, T scalar) {
    return {value.re * scalar, value.im * scalar};
}

template <typename T> inline constexpr TComplex<T> operator*(T scalar, TComplex<T> value) {
    return value * scalar;
}

template <typename T> inline constexpr T normSquared(TComplex<T> value) {
    return value.re * value.re + value.im * value.im;
}

inline HighPrecisionFloat magnitude(HighPrecisionComplex const& value) {
    return boost::multiprecision::sqrt(value.re * value.re + value.im * value.im);
}

inline Complex toComplex(HighPrecisionComplex const& value) {
    return {value.re.convert_to<Complex::Real>(), value.im.convert_to<Complex::Real>()};
}
