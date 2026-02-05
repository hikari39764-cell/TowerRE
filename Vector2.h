#pragma once
#include "HIKARI/HIKARI_Utility.h"
#include <cmath>

inline Vector2 operator+(const Vector2& a, const Vector2& b) {
    return { a.x + b.x, a.y + b.y };
}

inline Vector2 operator-(const Vector2& a, const Vector2& b) {
    return { a.x - b.x, a.y - b.y };
}

inline Vector2 operator*(const Vector2& v, float s) {
    return { v.x * s, v.y * s };
}

inline Vector2 operator*(float s, const Vector2& v) {
    return v * s;
}

inline Vector2& operator+=(Vector2& a, const Vector2& b) {
    a.x += b.x;
    a.y += b.y;
    return a;
}

inline float LengthSq(const Vector2& v) {
    return v.x * v.x + v.y * v.y;
}

inline float Length(const Vector2& v) {
    return std::sqrt(LengthSq(v));
}

inline Vector2 Normalize(const Vector2& v) {
    float len = Length(v);
    if (len <= 1e-5f) {
        return { 0.0f, 0.0f };
    }
    return { v.x / len, v.y / len };
}

inline float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

inline Vector2 Lerp(const Vector2& a, const Vector2& b, float t) {
    return { Lerp(a.x, b.x, t), Lerp(a.y, b.y, t) };
}
