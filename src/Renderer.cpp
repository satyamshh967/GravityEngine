#include "Renderer.hpp"
#include <raymath.h>
#include <algorithm>
#include <cstdio>
#include <string>

Renderer::Renderer(int screenWidth, int screenHeight) {
    resetCamera(screenWidth, screenHeight);
}

void Renderer::resetCamera(int screenWidth, int screenHeight) {
    camera.target = Vector2{0.0f, 0.0f};
    camera.offset = Vector2{screenWidth * 0.5f, screenHeight * 0.5f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;
}

Vec2 Renderer::screenToWorld(Vector2 screenPos) const {
    Vector2 w = GetScreenToWorld2D(screenPos, camera);
    return Vec2{static_cast<double>(w.x), static_cast<double>(w.y)};
}

Vector2 Renderer::worldToScreen(Vec2 worldPos) const {
    return GetWorldToScreen2D(worldPos.toVector2(), camera);
}

void Renderer::updateCamera(float deltaTime) {
    // Mouse drag panning (Right Mouse Button)
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / camera.zoom);
        camera.target = Vector2Add(camera.target, delta);
    }

    // Keyboard panning (WASD or Arrow keys)
    float panSpeed = 600.0f * (1.0f / camera.zoom) * deltaTime;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    camera.target.y -= panSpeed;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  camera.target.y += panSpeed;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  camera.target.x -= panSpeed;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) camera.target.x += panSpeed;

    // Mouse wheel zoom towards cursor
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), camera);
        camera.offset = GetMousePosition();
        camera.target = mouseWorld;
        
        float scaleFactor = 1.0f + (0.15f * std::abs(wheel));
        if (wheel < 0.0f) {
            scaleFactor = 1.0f / scaleFactor;
        }
        camera.zoom = std::clamp(camera.zoom * scaleFactor, 0.02f, 25.0f);
    }
}

void Renderer::drawGrid() const {
    if (!showGrid) return;

    // Center crosshairs
    DrawLine(-20000, 0, 20000, 0, Fade(DARKGRAY, 0.35f));
    DrawLine(0, -20000, 0, 20000, Fade(DARKGRAY, 0.35f));

    // Distance reference rings
    const float rings[] = {100.0f, 250.0f, 500.0f, 1000.0f, 2000.0f};
    for (float r : rings) {
        DrawCircleLines(0, 0, r, Fade(GRAY, 0.18f));
    }
}

void Renderer::drawTrails(const std::vector<Body>& bodies) const {
    if (!showTrails) return;

    for (const auto& body : bodies) {
        if (body.trail.size() < 2) continue;

        const size_t trailSize = body.trail.size();
        for (size_t i = 1; i < trailSize; ++i) {
            float progress = static_cast<float>(i) / static_cast<float>(trailSize);
            float alpha = progress * 0.7f;
            Color segColor = Fade(body.color, alpha);

            Vector2 p1 = body.trail[i - 1].toVector2();
            Vector2 p2 = body.trail[i].toVector2();
            DrawLineEx(p1, p2, 1.5f, segColor);
        }
    }
}

void Renderer::drawBodies(const std::vector<Body>& bodies) const {
    for (const auto& body : bodies) {
        Vector2 pos = body.pos.toVector2();
        float r = static_cast<float>(body.radius);

        // Soft outer glow / atmospheric aura
        DrawCircleV(pos, r * 1.6f, Fade(body.color, 0.22f));
        DrawCircleV(pos, r * 1.25f, Fade(body.color, 0.45f));

        // Solid core
        DrawCircleV(pos, r, body.color);

        // Inner specular highlight
        DrawCircleV(Vector2{pos.x - r * 0.28f, pos.y - r * 0.28f}, r * 0.35f, Fade(WHITE, 0.5f));

        // Subtle dark rim
        DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), r, Fade(BLACK, 0.4f));

        // Body name display for prominent bodies or when zoomed in
        if (camera.zoom > 0.4f && (body.mass > 5.0 || body.name != "Particle")) {
            DrawText(body.name.c_str(), static_cast<int>(pos.x + r + 4),
                     static_cast<int>(pos.y - 6), 11, Fade(RAYWHITE, 0.85f));
        }
    }
}

void Renderer::drawVelocityVectors(const std::vector<Body>& bodies) const {
    if (!showVelocityVectors) return;

    const float velScale = 0.5f;
    for (const auto& body : bodies) {
        if (body.vel.lengthSq() < 0.01) continue;

        Vector2 start = body.pos.toVector2();
        Vector2 end = Vector2{
            static_cast<float>(body.pos.x + body.vel.x * velScale),
            static_cast<float>(body.pos.y + body.vel.y * velScale)
        };

        DrawLineEx(start, end, 1.5f, Fade(GREEN, 0.75f));

        // Arrow head
        Vector2 dir = Vector2Normalize(Vector2Subtract(end, start));
        Vector2 perp = Vector2{-dir.y, dir.x};
        float arrowSize = 4.0f;
        Vector2 pLeft = Vector2Subtract(Vector2Subtract(end, Vector2Scale(dir, arrowSize)), Vector2Scale(perp, arrowSize * 0.5f));
        Vector2 pRight = Vector2Add(Vector2Subtract(end, Vector2Scale(dir, arrowSize)), Vector2Scale(perp, arrowSize * 0.5f));

        DrawTriangle(end, pLeft, pRight, Fade(GREEN, 0.85f));
    }
}

void Renderer::drawTrajectoryPreview(const std::vector<Vec2>& trajectoryPoints,
                                    Vec2 spawnPos, double spawnRadius, Color spawnColor) const {
    // Draw phantom ghost of body to be spawned
    Vector2 sp = spawnPos.toVector2();
    float r = static_cast<float>(spawnRadius);
    DrawCircleV(sp, r * 1.4f, Fade(spawnColor, 0.3f));
    DrawCircleV(sp, r, Fade(spawnColor, 0.7f));
    DrawCircleLines(static_cast<int>(sp.x), static_cast<int>(sp.y), r, WHITE);

    // Draw predicted trajectory curve
    if (trajectoryPoints.size() >= 2) {
        const size_t count = trajectoryPoints.size();
        for (size_t i = 1; i < count; ++i) {
            float alpha = 1.0f - (static_cast<float>(i) / static_cast<float>(count)) * 0.7f;
            Color curveColor = Fade(GOLD, alpha);
            DrawLineEx(trajectoryPoints[i - 1].toVector2(),
                       trajectoryPoints[i].toVector2(),
                       2.0f, curveColor);
        }
    }
}

void Renderer::drawHUD(const GravityEngine& engine, float timeScale, bool isPaused,
                      double spawnMass, int screenWidth, int screenHeight) const {
    // 1. Top-Left Diagnostics Panel
    int panelW = 340;
    int panelH = 265;
    DrawRectangleRounded(Rectangle{15, 15, static_cast<float>(panelW), static_cast<float>(panelH)}, 0.08f, 6, Fade(BLACK, 0.78f));
    DrawRectangleRoundedLines(Rectangle{15, 15, static_cast<float>(panelW), static_cast<float>(panelH)}, 0.08f, 6, Fade(SKYBLUE, 0.6f));

    DrawText("2D N-BODY GRAVITY ENGINE", 28, 25, 16, SKYBLUE);
    DrawLine(28, 46, 28 + panelW - 26, 46, Fade(DARKGRAY, 0.5f));

    int y = 55;
    int lineSpacing = 19;

    // FPS & Body Count
    DrawText(TextFormat("FPS: %d", GetFPS()), 28, y, 13, GREEN);
    DrawText(TextFormat("Bodies: %zu", engine.getBodyCount()), 160, y, 13, RAYWHITE);
    y += lineSpacing;

    // Integrator
    DrawText("Integrator [I]:", 28, y, 13, LIGHTGRAY);
    DrawText(engine.getIntegratorName(), 160, y, 13,
             (engine.integrator == IntegratorType::VelocityVerlet) ? GOLD : ORANGE);
    y += lineSpacing;

    // Collision Mode
    DrawText("Collision [C]:", 28, y, 13, LIGHTGRAY);
    DrawText(engine.getCollisionModeName(), 160, y, 13,
             (engine.collisionMode == CollisionMode::InelasticMerge) ? VIOLET : (engine.collisionMode == CollisionMode::Elastic ? LIME : GRAY));
    y += lineSpacing;

    // Simulation State & Time Scale
    DrawText("Status [SPACE]:", 28, y, 13, LIGHTGRAY);
    DrawText(isPaused ? "PAUSED" : "RUNNING", 160, y, 13, isPaused ? RED : LIME);
    y += lineSpacing;

    DrawText("Time Scale [</>]:", 28, y, 13, LIGHTGRAY);
    DrawText(TextFormat("%.1fx (substeps: %d)", timeScale, static_cast<int>(timeScale * 5)), 160, y, 13, RAYWHITE);
    y += lineSpacing;

    // Energy Diagnostics
    double kinetic = engine.computeKineticEnergy();
    double potential = engine.computePotentialEnergy();
    double total = kinetic + potential;
    double drift = engine.getEnergyDriftPercent();

    DrawText("Total Energy E:", 28, y, 13, LIGHTGRAY);
    DrawText(TextFormat("%+11.2e", total), 160, y, 13, RAYWHITE);
    y += lineSpacing;

    DrawText("Energy Drift:", 28, y, 13, LIGHTGRAY);
    Color driftColor = (drift < 0.01) ? LIME : ((drift < 0.1) ? YELLOW : ORANGE);
    DrawText(TextFormat("%.5f%%", drift), 160, y, 13, driftColor);
    y += lineSpacing;

    // Spawn Mass
    DrawText("Spawn Mass [+/-]:", 28, y, 13, LIGHTGRAY);
    DrawText(TextFormat("%.1f", spawnMass), 160, y, 13, YELLOW);
    y += lineSpacing;

    // Zoom level
    DrawText("Camera Zoom:", 28, y, 13, LIGHTGRAY);
    DrawText(TextFormat("%.2fx", camera.zoom), 160, y, 13, RAYWHITE);

    // 2. Bottom-Left Preset & Controls Cheat-sheet
    int cheatW = 460;
    int cheatH = 100;
    float cheatY = static_cast<float>(screenHeight - cheatH - 15);
    DrawRectangleRounded(Rectangle{15, cheatY, static_cast<float>(cheatW), static_cast<float>(cheatH)}, 0.08f, 6, Fade(BLACK, 0.78f));
    DrawRectangleRoundedLines(Rectangle{15, cheatY, static_cast<float>(cheatW), static_cast<float>(cheatH)}, 0.08f, 6, Fade(GRAY, 0.4f));

    DrawText("PRESETS: [1] Solar System  [2] Figure-8 Orbit  [3] Binary Disk", 26, static_cast<int>(cheatY + 12), 12, GOLD);
    DrawText("CONTROLS:", 26, static_cast<int>(cheatY + 34), 12, SKYBLUE);
    DrawText("Left Click + Drag: Slingshot Spawn Body | Right Drag / WASD: Pan", 26, static_cast<int>(cheatY + 52), 11, RAYWHITE);
    DrawText("Wheel: Zoom | [T] Trails | [V] Vectors | [G] Grid | [R] Reset Cam | [X] Clear", 26, static_cast<int>(cheatY + 70), 11, RAYWHITE);

    // 3. Slingshot guide tip if mouse is held
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        DrawText("AIMING SLINGSHOT: Release to launch body into orbit", screenWidth / 2 - 180, 25, 14, YELLOW);
    }
}
