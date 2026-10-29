#pragma once

#include <cmath>
#include <raylib.h>

struct Vec2 {
    double x{0.0};
    double y{0.0};

    constexpr Vec2() = default;
    constexpr Vec2(double x_, double y_) : x(x_), y(y_) {}

    // Vector operations
    constexpr Vec2 operator+(const Vec2& rhs) const noexcept {
        return Vec2{x + rhs.x, y + rhs.y};
    }

    constexpr Vec2 operator-(const Vec2& rhs) const noexcept {
        return Vec2{x - rhs.x, y - rhs.y};
    }

    constexpr Vec2 operator*(double scalar) const noexcept {
        return Vec2{x * scalar, y * scalar};
    }

    friend constexpr Vec2 operator*(double scalar, const Vec2& vec) noexcept {
        return Vec2{vec.x * scalar, vec.y * scalar};
    }

    constexpr Vec2 operator/(double scalar) const noexcept {
        double inv = (scalar != 0.0) ? (1.0 / scalar) : 0.0;
        return Vec2{x * inv, y * inv};
    }

    constexpr Vec2 operator-() const noexcept {
        return Vec2{-x, -y};
    }

    // Compound assignment operators
    constexpr Vec2& operator+=(const Vec2& rhs) noexcept {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    constexpr Vec2& operator-=(const Vec2& rhs) noexcept {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    constexpr Vec2& operator*=(double scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    constexpr Vec2& operator/=(double scalar) noexcept {
        double inv = (scalar != 0.0) ? (1.0 / scalar) : 0.0;
        x *= inv;
        y *= inv;
        return *this;
    }

    constexpr bool operator==(const Vec2& rhs) const noexcept {
        return x == rhs.x && y == rhs.y;
    }

    constexpr bool operator!=(const Vec2& rhs) const noexcept {
        return !(*this == rhs);
    }

    // Math methods
    [[nodiscard]] constexpr double dot(const Vec2& rhs) const noexcept {
        return x * rhs.x + y * rhs.y;
    }

    [[nodiscard]] constexpr double lengthSq() const noexcept {
        return x * x + y * y;
    }

    [[nodiscard]] double length() const noexcept {
        return std::sqrt(lengthSq());
    }

    [[nodiscard]] Vec2 normalized() const noexcept {
        double len = length();
        if (len > 1e-14) {
            return *this / len;
        }
        return Vec2{0.0, 0.0};
    }

    [[nodiscard]] constexpr double distanceSq(const Vec2& rhs) const noexcept {
        double dx = rhs.x - x;
        double dy = rhs.y - y;
        return dx * dx + dy * dy;
    }

    [[nodiscard]] double distance(const Vec2& rhs) const noexcept {
        return std::sqrt(distanceSq(rhs));
    }

    // Raylib Vector2 conversion
    [[nodiscard]] Vector2 toVector2() const noexcept {
        return Vector2{static_cast<float>(x), static_cast<float>(y)};
    }

    [[nodiscard]] static Vec2 fromVector2(const Vector2& v) noexcept {
        return Vec2{static_cast<double>(v.x), static_cast<double>(v.y)};
    }
};
