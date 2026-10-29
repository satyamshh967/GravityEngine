#pragma once

#include "GravityEngine.hpp"
#include <raylib.h>
#include <vector>

class Renderer {
public:
    Camera2D camera{};
    bool showTrails{true};
    bool showVelocityVectors{true};
    bool showGrid{true};
    bool followHeaviest{false};

    Renderer(int screenWidth, int screenHeight);

    // Camera controls
    void updateCamera(float deltaTime);
    void resetCamera(int screenWidth, int screenHeight);

    // Drawing functions
    void drawGrid() const;
    void drawTrails(const std::vector<Body>& bodies) const;
    void drawBodies(const std::vector<Body>& bodies) const;
    void drawVelocityVectors(const std::vector<Body>& bodies) const;
    void drawTrajectoryPreview(const std::vector<Vec2>& trajectoryPoints,
                               Vec2 spawnPos, double spawnRadius, Color spawnColor) const;
    void drawHUD(const GravityEngine& engine, float timeScale, bool isPaused,
                 double spawnMass, int screenWidth, int screenHeight) const;

    // Coordinate conversion
    [[nodiscard]] Vec2 screenToWorld(Vector2 screenPos) const;
    [[nodiscard]] Vector2 worldToScreen(Vec2 worldPos) const;
};
