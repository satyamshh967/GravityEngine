#pragma once

#include "Vec2.hpp"
#include <string>
#include <deque>
#include <algorithm>
#include <cmath>
#include <raylib.h>

struct Body {
    int id{0};
    std::string name{"Body"};
    double mass{1.0};
    double radius{5.0};
    Vec2 pos{0.0, 0.0};
    Vec2 vel{0.0, 0.0};
    Vec2 acc{0.0, 0.0};
    Color color{RAYWHITE};
    bool isStatic{false};
    
    // Orbital trail history
    std::deque<Vec2> trail;
    size_t maxTrailPoints{350};
    double minTrailDistSq{4.0}; // minimum squared distance between trail points to avoid clumping

    Body() = default;

    Body(int id_, std::string name_, double mass_, double radius_,
         Vec2 pos_, Vec2 vel_, Color color_, bool isStatic_ = false)
        : id(id_),
          name(std::move(name_)),
          mass(mass_),
          radius(radius_),
          pos(pos_),
          vel(vel_),
          acc(0.0, 0.0),
          color(color_),
          isStatic(isStatic_) {}

    void addTrailPoint(const Vec2& p) {
        if (!trail.empty()) {
            if (trail.back().distanceSq(p) < minTrailDistSq) {
                return;
            }
        }
        trail.push_back(p);
        if (trail.size() > maxTrailPoints) {
            trail.pop_front();
        }
    }

    void clearTrail() {
        trail.clear();
    }

    // Physical diagnostics and property mutators
    [[nodiscard]] double getDensity() const {
        constexpr double kPi = 3.14159265358979323846;
        if (radius > 1e-6) {
            return mass / (kPi * radius * radius);
        }
        return 0.0;
    }

    void setDensity(double d) {
        constexpr double kPi = 3.14159265358979323846;
        if (d > 1e-6) {
            radius = std::clamp(std::sqrt(mass / (kPi * d)), 1.5, 120.0);
        }
    }

    void setMass(double m) {
        double d = getDensity();
        mass = std::max(m, 0.0001);
        if (d > 1e-6) {
            setDensity(d); // Maintain density by adjusting radius
        }
    }

    void setRadius(double r) {
        radius = std::clamp(r, 1.5, 120.0);
    }

    [[nodiscard]] double getSurfaceGravity(double G, double epsilon) const {
        double rSq = radius * radius;
        double denom = rSq + epsilon * epsilon;
        if (denom > 1e-12) {
            return (G * mass) / denom;
        }
        return 0.0;
    }

    [[nodiscard]] double getEscapeVelocity(double G) const {
        if (radius > 1e-6) {
            return std::sqrt(2.0 * G * mass / radius);
        }
        return 0.0;
    }
};
