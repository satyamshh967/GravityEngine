#pragma once

#include "Vec2.hpp"
#include <string>
#include <deque>
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
};
