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
    followedBodyId = -1;
}

Vec2 Renderer::screenToWorld(Vector2 screenPos) const {
    Vector2 w = GetScreenToWorld2D(screenPos, camera);
    return Vec2{static_cast<double>(w.x), static_cast<double>(w.y)};
}

Vector2 Renderer::worldToScreen(Vec2 worldPos) const {
    return GetWorldToScreen2D(worldPos.toVector2(), camera);
}

void Renderer::updateCamera(float deltaTime) {
    animTime += deltaTime;

    // Mouse drag panning (Right Mouse Button)
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / camera.zoom);
        camera.target = Vector2Add(camera.target, delta);
        followedBodyId = -1; // Manual pan cancels target lock
    }

    // Keyboard panning (WASD or Arrow keys)
    float panSpeed = 600.0f * (1.0f / camera.zoom) * deltaTime;
    bool moved = false;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    { camera.target.y -= panSpeed; moved = true; }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  { camera.target.y += panSpeed; moved = true; }
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  { camera.target.x -= panSpeed; moved = true; }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) { camera.target.x += panSpeed; moved = true; }
    if (moved) followedBodyId = -1;

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

void Renderer::drawBodies(const std::vector<Body>& bodies, int selectedId) const {
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

        // Rim
        DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), r, Fade(BLACK, 0.4f));

        // Static anchor indicator
        if (body.isStatic) {
            DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), r + 3.0f, Fade(GOLD, 0.6f));
        }

        // Body name display for prominent bodies or when selected
        if (body.id == selectedId || (camera.zoom > 0.4f && (body.mass > 5.0 || body.name != "Particle"))) {
            DrawText(body.name.c_str(), static_cast<int>(pos.x + r + 4),
                     static_cast<int>(pos.y - 6), 11, Fade(RAYWHITE, 0.9f));
        }
    }
}

void Renderer::drawSelectionReticle(const Body& body) const {
    Vector2 pos = body.pos.toVector2();
    float r = static_cast<float>(body.radius);
    float boxR = r + 8.0f + 2.0f * std::sin(animTime * 4.0f);
    float bracketLen = std::max(6.0f, boxR * 0.35f);

    Color reticleCol = Color{255, 220, 40, static_cast<unsigned char>(200 + 55 * std::sin(animTime * 6.0f))};

    // 4 Corner Brackets
    // Top-Left
    DrawLineEx(Vector2{pos.x - boxR, pos.y - boxR}, Vector2{pos.x - boxR + bracketLen, pos.y - boxR}, 2.0f, reticleCol);
    DrawLineEx(Vector2{pos.x - boxR, pos.y - boxR}, Vector2{pos.x - boxR, pos.y - boxR + bracketLen}, 2.0f, reticleCol);

    // Top-Right
    DrawLineEx(Vector2{pos.x + boxR, pos.y - boxR}, Vector2{pos.x + boxR - bracketLen, pos.y - boxR}, 2.0f, reticleCol);
    DrawLineEx(Vector2{pos.x + boxR, pos.y - boxR}, Vector2{pos.x + boxR, pos.y - boxR + bracketLen}, 2.0f, reticleCol);

    // Bottom-Left
    DrawLineEx(Vector2{pos.x - boxR, pos.y + boxR}, Vector2{pos.x - boxR + bracketLen, pos.y + boxR}, 2.0f, reticleCol);
    DrawLineEx(Vector2{pos.x - boxR, pos.y + boxR}, Vector2{pos.x - boxR, pos.y + boxR - bracketLen}, 2.0f, reticleCol);

    // Bottom-Right
    DrawLineEx(Vector2{pos.x + boxR, pos.y + boxR}, Vector2{pos.x + boxR - bracketLen, pos.y + boxR}, 2.0f, reticleCol);
    DrawLineEx(Vector2{pos.x + boxR, pos.y + boxR}, Vector2{pos.x + boxR, pos.y + boxR - bracketLen}, 2.0f, reticleCol);

    // Pulsing selection circle
    DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), boxR * 0.85f, Fade(reticleCol, 0.45f));

    // Draw net acceleration vector in Orange (gravitational pull)
    if (body.acc.lengthSq() > 0.001) {
        float accScale = 2.0f;
        Vector2 accEnd = Vector2{
            static_cast<float>(body.pos.x + body.acc.x * accScale),
            static_cast<float>(body.pos.y + body.acc.y * accScale)
        };
        DrawLineEx(pos, accEnd, 1.5f, Fade(ORANGE, 0.85f));
        DrawCircleV(accEnd, 2.5f, ORANGE);
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
    // Phantom ghost of body to be spawned
    Vector2 sp = spawnPos.toVector2();
    float r = static_cast<float>(spawnRadius);
    DrawCircleV(sp, r * 1.4f, Fade(spawnColor, 0.3f));
    DrawCircleV(sp, r, Fade(spawnColor, 0.7f));
    DrawCircleLines(static_cast<int>(sp.x), static_cast<int>(sp.y), r, WHITE);

    // Predicted trajectory curve
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

bool Renderer::DrawButton(Rectangle rect, const char* text, Color bg, Color textCol, bool isActive) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);

    Color drawBg = bg;
    Color borderCol = isActive ? GOLD : (hovered ? SKYBLUE : Fade(GRAY, 0.5f));

    if (isActive) {
        drawBg = Color{drawBg.r, static_cast<unsigned char>(std::min(255, drawBg.g + 40)), drawBg.b, drawBg.a};
    } else if (hovered) {
        drawBg = Color{
            static_cast<unsigned char>(std::min(255, drawBg.r + 25)),
            static_cast<unsigned char>(std::min(255, drawBg.g + 25)),
            static_cast<unsigned char>(std::min(255, drawBg.b + 25)),
            drawBg.a
        };
    }

    DrawRectangleRounded(rect, 0.25f, 4, drawBg);
    DrawRectangleRoundedLines(rect, 0.25f, 4, borderCol);

    int fontSize = 11;
    int textW = MeasureText(text, fontSize);
    int textX = static_cast<int>(rect.x + (rect.width - textW) * 0.5f);
    int textY = static_cast<int>(rect.y + (rect.height - fontSize) * 0.5f);

    DrawText(text, textX, textY, fontSize, hovered ? WHITE : textCol);

    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void Renderer::drawHUD(const GravityEngine& engine, float timeScale, bool isPaused,
                      const LaunchSettings& launchSettings, int screenWidth, int screenHeight,
                      bool showLauncherPanel) const {
    // 1. Top-Left Diagnostics Panel
    int panelW = 340;
    int panelH = 265;
    DrawRectangleRounded(Rectangle{15, 15, static_cast<float>(panelW), static_cast<float>(panelH)}, 0.08f, 6, Fade(BLACK, 0.82f));
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

    // Active Spawner Type & Mode
    DrawText("Spawner [TAB]:", 28, y, 13, LIGHTGRAY);
    DrawText(TextFormat("%s (M=%.1f)", launchSettings.typeName.c_str(), launchSettings.mass), 160, y, 13, YELLOW);
    y += lineSpacing;

    // Zoom level & Target
    DrawText("Camera Zoom:", 28, y, 13, LIGHTGRAY);
    DrawText(TextFormat("%.2fx %s", camera.zoom, (followedBodyId != -1 ? "[LOCKED]" : "")), 160, y, 13, RAYWHITE);

    // 2. Bottom-Left Preset & Controls Cheat-sheet
    int cheatW = 460;
    int cheatH = 100;
    float cheatY = static_cast<float>(screenHeight - cheatH - 15);
    DrawRectangleRounded(Rectangle{15, cheatY, static_cast<float>(cheatW), static_cast<float>(cheatH)}, 0.08f, 6, Fade(BLACK, 0.82f));
    DrawRectangleRoundedLines(Rectangle{15, cheatY, static_cast<float>(cheatW), static_cast<float>(cheatH)}, 0.08f, 6, Fade(GRAY, 0.4f));

    DrawText("PRESETS: [1] Solar System  [2] Figure-8 Orbit  [3] Binary Disk", 26, static_cast<int>(cheatY + 12), 12, GOLD);
    DrawText("CONTROLS:", 26, static_cast<int>(cheatY + 34), 12, SKYBLUE);
    DrawText("Left Click Body: Select / Inspect | Left Drag: Launch Spawner", 26, static_cast<int>(cheatY + 52), 11, RAYWHITE);
    DrawText("[TAB] Spawner Menu | [T] Trails | [V] Vectors | [G] Grid | [R] Reset | [X] Clear", 26, static_cast<int>(cheatY + 70), 11, RAYWHITE);

    // 3. Spawner Menu Toggle Button (Top-Right)
    Rectangle spawnerBtnRect{static_cast<float>(screenWidth - 215), 15.0f, 200.0f, 32.0f};
    const char* btnLabel = showLauncherPanel ? "CLOSE SPAWNER MENU [TAB]" : "OPEN SPAWNER MENU [TAB]";
    DrawButton(spawnerBtnRect, btnLabel, Color{25, 35, 55, 230}, showLauncherPanel ? GOLD : SKYBLUE, showLauncherPanel);
}

void Renderer::drawInspectorPanel(Body* body, GravityEngine& engine, int screenWidth, int /*screenHeight*/,
                                  bool& shouldDeselect, bool& shouldDelete) {
    if (!body) return;

    int panelW = 345;
    int panelH = 550;
    float panelX = static_cast<float>(screenWidth - panelW - 15);
    float panelY = 55.0f;

    Rectangle panelRect{panelX, panelY, static_cast<float>(panelW), static_cast<float>(panelH)};
    DrawRectangleRounded(panelRect, 0.04f, 6, Fade(Color{12, 16, 26, 255}, 0.94f));
    DrawRectangleRoundedLines(panelRect, 0.04f, 6, Fade(GOLD, 0.75f));

    // Header with Title & Close button
    DrawText("OBJECT INSPECTOR", static_cast<int>(panelX + 16), static_cast<int>(panelY + 14), 14, GOLD);

    Rectangle closeRect{panelX + panelW - 32, panelY + 10, 22, 22};
    if (DrawButton(closeRect, "X", Color{60, 20, 20, 200}, RED)) {
        shouldDeselect = true;
    }

    DrawLine(static_cast<int>(panelX + 15), static_cast<int>(panelY + 38),
             static_cast<int>(panelX + panelW - 15), static_cast<int>(panelY + 38), Fade(DARKGRAY, 0.5f));

    // Body Visual Thumbnail & Name
    Vector2 thumbPos{panelX + 32, panelY + 60};
    DrawCircleV(thumbPos, 14.0f, Fade(body->color, 0.3f));
    DrawCircleV(thumbPos, 10.0f, body->color);
    DrawCircleLines(static_cast<int>(thumbPos.x), static_cast<int>(thumbPos.y), 10, WHITE);

    DrawText(body->name.c_str(), static_cast<int>(panelX + 54), static_cast<int>(panelY + 50), 15, RAYWHITE);
    DrawText(body->isStatic ? "[ANCHORED / STATIC]" : "[DYNAMIC IN ORBIT]",
             static_cast<int>(panelX + 54), static_cast<int>(panelY + 68), 10, body->isStatic ? GOLD : SKYBLUE);

    float y = panelY + 95.0f;
    float rowH = 24.0f;

    // --- Mass Section ---
    DrawText("Mass:", static_cast<int>(panelX + 16), static_cast<int>(y), 12, LIGHTGRAY);
    DrawText(TextFormat("%.2e", body->mass), static_cast<int>(panelX + 90), static_cast<int>(y), 12, YELLOW);

    if (DrawButton(Rectangle{panelX + 170, y - 2, 36, 20}, "-10%", Color{30, 40, 60, 220}, RAYWHITE)) {
        body->setMass(body->mass * 0.9);
    }
    if (DrawButton(Rectangle{panelX + 210, y - 2, 36, 20}, "+10%", Color{30, 40, 60, 220}, RAYWHITE)) {
        body->setMass(body->mass * 1.1);
    }
    if (DrawButton(Rectangle{panelX + 252, y - 2, 38, 20}, "/2", Color{30, 40, 60, 220}, RAYWHITE)) {
        body->setMass(body->mass * 0.5);
    }
    if (DrawButton(Rectangle{panelX + 294, y - 2, 38, 20}, "x2", Color{30, 40, 60, 220}, RAYWHITE)) {
        body->setMass(body->mass * 2.0);
    }
    y += rowH + 6;

    // --- Radius Section ---
    DrawText("Radius:", static_cast<int>(panelX + 16), static_cast<int>(y), 12, LIGHTGRAY);
    DrawText(TextFormat("%.1f px", body->radius), static_cast<int>(panelX + 90), static_cast<int>(y), 12, RAYWHITE);

    if (DrawButton(Rectangle{panelX + 210, y - 2, 45, 20}, "-1 px", Color{30, 40, 60, 220}, RAYWHITE)) {
        body->setRadius(body->radius - 1.0);
    }
    if (DrawButton(Rectangle{panelX + 265, y - 2, 45, 20}, "+1 px", Color{30, 40, 60, 220}, RAYWHITE)) {
        body->setRadius(body->radius + 1.0);
    }
    y += rowH + 6;

    // --- Density Section ---
    DrawText("Density (ρ):", static_cast<int>(panelX + 16), static_cast<int>(y), 12, LIGHTGRAY);
    DrawText(TextFormat("%.4f", body->getDensity()), static_cast<int>(panelX + 90), static_cast<int>(y), 12, GREEN);

    if (DrawButton(Rectangle{panelX + 210, y - 2, 45, 20}, "-20%", Color{30, 40, 60, 220}, RAYWHITE)) {
        body->setDensity(body->getDensity() * 0.8);
    }
    if (DrawButton(Rectangle{panelX + 265, y - 2, 45, 20}, "+20%", Color{30, 40, 60, 220}, RAYWHITE)) {
        body->setDensity(body->getDensity() * 1.2);
    }
    y += rowH + 10;

    // --- Physics & Gravity Diagnostics Section ---
    DrawLine(static_cast<int>(panelX + 15), static_cast<int>(y),
             static_cast<int>(panelX + panelW - 15), static_cast<int>(y), Fade(GRAY, 0.3f));
    y += 8;

    DrawText("GRAVITATIONAL FIELD & STATS", static_cast<int>(panelX + 16), static_cast<int>(y), 11, SKYBLUE);
    y += 18;

    double surfG = body->getSurfaceGravity(engine.G, engine.epsilon);
    double escV = body->getEscapeVelocity(engine.G);
    double speed = body->vel.length();
    double accPull = body->acc.length();

    DrawText("Surface Gravity (g):", static_cast<int>(panelX + 16), static_cast<int>(y), 11, LIGHTGRAY);
    DrawText(TextFormat("%.3e", surfG), static_cast<int>(panelX + 170), static_cast<int>(y), 11, GOLD);
    y += 18;

    DrawText("Escape Velocity (V_esc):", static_cast<int>(panelX + 16), static_cast<int>(y), 11, LIGHTGRAY);
    DrawText(TextFormat("%.2f m/s", escV), static_cast<int>(panelX + 170), static_cast<int>(y), 11, GOLD);
    y += 18;

    DrawText("Current Speed:", static_cast<int>(panelX + 16), static_cast<int>(y), 11, LIGHTGRAY);
    DrawText(TextFormat("%.2f m/s", speed), static_cast<int>(panelX + 170), static_cast<int>(y), 11, RAYWHITE);
    y += 18;

    DrawText("Net Gravity Pull (a):", static_cast<int>(panelX + 16), static_cast<int>(y), 11, LIGHTGRAY);
    DrawText(TextFormat("%.3e m/s²", accPull), static_cast<int>(panelX + 170), static_cast<int>(y), 11, ORANGE);
    y += 18;

    DrawText("Position (X, Y):", static_cast<int>(panelX + 16), static_cast<int>(y), 11, LIGHTGRAY);
    DrawText(TextFormat("%.1f, %.1f", body->pos.x, body->pos.y), static_cast<int>(panelX + 170), static_cast<int>(y), 11, RAYWHITE);
    y += 24;

    // --- Action Buttons ---
    DrawLine(static_cast<int>(panelX + 15), static_cast<int>(y),
             static_cast<int>(panelX + panelW - 15), static_cast<int>(y), Fade(GRAY, 0.3f));
    y += 10;

    bool isLocked = (followedBodyId == body->id);
    if (DrawButton(Rectangle{panelX + 16, y, 148, 26}, isLocked ? "UNLOCK CAM" : "LOCK CAMERA",
                   Color{30, 45, 65, 230}, isLocked ? GOLD : RAYWHITE, isLocked)) {
        followedBodyId = isLocked ? -1 : body->id;
    }

    if (DrawButton(Rectangle{panelX + 174, y, 154, 26}, body->isStatic ? "MAKE DYNAMIC" : "MAKE STATIC",
                   Color{30, 45, 65, 230}, body->isStatic ? GOLD : RAYWHITE, body->isStatic)) {
        body->isStatic = !body->isStatic;
        if (body->isStatic) body->vel = Vec2{0.0, 0.0};
    }
    y += 32;

    if (DrawButton(Rectangle{panelX + 16, y, 148, 26}, "HALT (V = 0)", Color{40, 35, 60, 230}, RAYWHITE)) {
        body->vel = Vec2{0.0, 0.0};
    }

    if (DrawButton(Rectangle{panelX + 174, y, 154, 26}, "CIRCULARIZE ORBIT", Color{20, 50, 40, 230}, LIME)) {
        engine.circularizeOrbit(body->id);
    }
    y += 34;

    // --- Color Swatches ---
    DrawText("Set Color Palette:", static_cast<int>(panelX + 16), static_cast<int>(y), 11, LIGHTGRAY);
    y += 16;

    const Color swatches[] = {
        Color{70, 160, 255, 255},  // Cyan / Earth
        Color{255, 215, 30, 255},  // Gold / Sun
        Color{240, 80, 60, 255},   // Mars / Red
        Color{60, 230, 120, 255},  // Emerald
        Color{220, 60, 240, 255},  // Magenta
        Color{240, 240, 245, 255}  // Pearl White
    };

    float swatchW = 45.0f;
    for (size_t i = 0; i < 6; ++i) {
        Rectangle sRect{panelX + 16 + i * 52, y, swatchW, 20};
        DrawRectangleRounded(sRect, 0.25f, 3, swatches[i]);
        DrawRectangleRoundedLines(sRect, 0.25f, 3, Fade(WHITE, 0.6f));
        if (CheckCollisionPointRec(GetMousePosition(), sRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            body->color = swatches[i];
        }
    }
    y += 30;

    // Clear Trail & Delete Buttons
    if (DrawButton(Rectangle{panelX + 16, y, 148, 26}, "CLEAR TRAIL", Color{40, 45, 55, 230}, RAYWHITE)) {
        body->clearTrail();
    }

    if (DrawButton(Rectangle{panelX + 174, y, 154, 26}, "DESTROY OBJECT", Color{80, 25, 25, 230}, RED)) {
        shouldDelete = true;
    }
}

void Renderer::drawLauncherSettingsPanel(LaunchSettings& settings, int screenWidth, int /*screenHeight*/,
                                        bool& isOpen) {
    if (!isOpen) return;

    int panelW = 345;
    int panelH = 550;
    float panelX = static_cast<float>(screenWidth - panelW - 15);
    float panelY = 55.0f;

    Rectangle panelRect{panelX, panelY, static_cast<float>(panelW), static_cast<float>(panelH)};
    DrawRectangleRounded(panelRect, 0.04f, 6, Fade(Color{12, 16, 26, 255}, 0.94f));
    DrawRectangleRoundedLines(panelRect, 0.04f, 6, Fade(SKYBLUE, 0.75f));

    // Header with Title & Close button
    DrawText("LAUNCHER / SPAWNER SETTINGS", static_cast<int>(panelX + 16), static_cast<int>(panelY + 14), 14, SKYBLUE);

    Rectangle closeRect{panelX + panelW - 32, panelY + 10, 22, 22};
    if (DrawButton(closeRect, "X", Color{60, 20, 20, 200}, RED)) {
        isOpen = false;
    }

    DrawLine(static_cast<int>(panelX + 15), static_cast<int>(panelY + 38),
             static_cast<int>(panelX + panelW - 15), static_cast<int>(panelY + 38), Fade(DARKGRAY, 0.5f));

    float y = panelY + 48.0f;

    // --- Celestial Presets ---
    DrawText("SELECT LAUNCH PRESET:", static_cast<int>(panelX + 16), static_cast<int>(y), 11, GOLD);
    y += 18;

    if (DrawButton(Rectangle{panelX + 16, y, 98, 25}, "Asteroid", Color{30, 40, 55, 230}, RAYWHITE,
                   settings.presetType == LaunchPresetType::Asteroid)) {
        settings.applyPreset(LaunchPresetType::Asteroid);
    }
    if (DrawButton(Rectangle{panelX + 122, y, 98, 25}, "Moon", Color{30, 40, 55, 230}, RAYWHITE,
                   settings.presetType == LaunchPresetType::Moon)) {
        settings.applyPreset(LaunchPresetType::Moon);
    }
    if (DrawButton(Rectangle{panelX + 228, y, 98, 25}, "Planet", Color{30, 40, 55, 230}, RAYWHITE,
                   settings.presetType == LaunchPresetType::Terrestrial)) {
        settings.applyPreset(LaunchPresetType::Terrestrial);
    }
    y += 30;

    if (DrawButton(Rectangle{panelX + 16, y, 98, 25}, "Gas Giant", Color{30, 40, 55, 230}, RAYWHITE,
                   settings.presetType == LaunchPresetType::GasGiant)) {
        settings.applyPreset(LaunchPresetType::GasGiant);
    }
    if (DrawButton(Rectangle{panelX + 122, y, 98, 25}, "Star", Color{30, 40, 55, 230}, RAYWHITE,
                   settings.presetType == LaunchPresetType::Star)) {
        settings.applyPreset(LaunchPresetType::Star);
    }
    if (DrawButton(Rectangle{panelX + 228, y, 98, 25}, "Singularity", Color{30, 40, 55, 230}, RAYWHITE,
                   settings.presetType == LaunchPresetType::BlackHole)) {
        settings.applyPreset(LaunchPresetType::BlackHole);
    }
    y += 36;

    // --- Launch Spawn Mode ---
    DrawLine(static_cast<int>(panelX + 15), static_cast<int>(y),
             static_cast<int>(panelX + panelW - 15), static_cast<int>(y), Fade(GRAY, 0.3f));
    y += 8;

    DrawText("SPAWN MODE:", static_cast<int>(panelX + 16), static_cast<int>(y), 11, GOLD);
    y += 18;

    if (DrawButton(Rectangle{panelX + 16, y, 98, 25}, "Slingshot", Color{30, 45, 65, 230}, RAYWHITE,
                   settings.spawnMode == LaunchSpawnMode::Slingshot)) {
        settings.spawnMode = LaunchSpawnMode::Slingshot;
    }
    if (DrawButton(Rectangle{panelX + 122, y, 98, 25}, "Auto Orbit", Color{30, 45, 65, 230}, RAYWHITE,
                   settings.spawnMode == LaunchSpawnMode::CircularOrbit)) {
        settings.spawnMode = LaunchSpawnMode::CircularOrbit;
    }
    if (DrawButton(Rectangle{panelX + 228, y, 98, 25}, "Stationary", Color{30, 45, 65, 230}, RAYWHITE,
                   settings.spawnMode == LaunchSpawnMode::Stationary)) {
        settings.spawnMode = LaunchSpawnMode::Stationary;
    }
    y += 36;

    // --- Mass & Density Customization ---
    DrawLine(static_cast<int>(panelX + 15), static_cast<int>(y),
             static_cast<int>(panelX + panelW - 15), static_cast<int>(y), Fade(GRAY, 0.3f));
    y += 8;

    DrawText("CUSTOM PHYSICAL PROPERTIES:", static_cast<int>(panelX + 16), static_cast<int>(y), 11, GOLD);
    y += 18;

    DrawText("Launch Mass:", static_cast<int>(panelX + 16), static_cast<int>(y), 12, LIGHTGRAY);
    DrawText(TextFormat("%.2e", settings.mass), static_cast<int>(panelX + 110), static_cast<int>(y), 12, YELLOW);

    if (DrawButton(Rectangle{panelX + 180, y - 2, 34, 20}, "-50%", Color{30, 40, 60, 220}, RAYWHITE)) {
        settings.mass = std::max(settings.mass * 0.5, 0.01);
        settings.recalculateRadiusFromDensity();
    }
    if (DrawButton(Rectangle{panelX + 218, y - 2, 34, 20}, "-10%", Color{30, 40, 60, 220}, RAYWHITE)) {
        settings.mass = std::max(settings.mass * 0.9, 0.01);
        settings.recalculateRadiusFromDensity();
    }
    if (DrawButton(Rectangle{panelX + 256, y - 2, 34, 20}, "+10%", Color{30, 40, 60, 220}, RAYWHITE)) {
        settings.mass *= 1.1;
        settings.recalculateRadiusFromDensity();
    }
    if (DrawButton(Rectangle{panelX + 294, y - 2, 34, 20}, "+50%", Color{30, 40, 60, 220}, RAYWHITE)) {
        settings.mass *= 1.5;
        settings.recalculateRadiusFromDensity();
    }
    y += 28;

    DrawText("Launch Radius:", static_cast<int>(panelX + 16), static_cast<int>(y), 12, LIGHTGRAY);
    DrawText(TextFormat("%.1f px", settings.radius), static_cast<int>(panelX + 110), static_cast<int>(y), 12, RAYWHITE);

    if (DrawButton(Rectangle{panelX + 218, y - 2, 45, 20}, "-1 px", Color{30, 40, 60, 220}, RAYWHITE)) {
        settings.radius = std::max(settings.radius - 1.0, 1.5);
        settings.recalculateDensityFromRadius();
    }
    if (DrawButton(Rectangle{panelX + 270, y - 2, 45, 20}, "+1 px", Color{30, 40, 60, 220}, RAYWHITE)) {
        settings.radius += 1.0;
        settings.recalculateDensityFromRadius();
    }
    y += 28;

    DrawText("Density (ρ):", static_cast<int>(panelX + 16), static_cast<int>(y), 12, LIGHTGRAY);
    DrawText(TextFormat("%.4f", settings.density), static_cast<int>(panelX + 110), static_cast<int>(y), 12, GREEN);

    if (DrawButton(Rectangle{panelX + 218, y - 2, 45, 20}, "-20%", Color{30, 40, 60, 220}, RAYWHITE)) {
        settings.density = std::max(settings.density * 0.8, 0.001);
        settings.recalculateRadiusFromDensity();
    }
    if (DrawButton(Rectangle{panelX + 270, y - 2, 45, 20}, "+20%", Color{30, 40, 60, 220}, RAYWHITE)) {
        settings.density *= 1.2;
        settings.recalculateRadiusFromDensity();
    }
    y += 32;

    // --- Color Swatches ---
    DrawText("Launch Color Palette:", static_cast<int>(panelX + 16), static_cast<int>(y), 11, LIGHTGRAY);
    y += 16;

    const Color swatches[] = {
        Color{70, 160, 255, 255},  // Cyan
        Color{255, 215, 30, 255},  // Gold
        Color{240, 80, 60, 255},   // Ruby
        Color{60, 230, 120, 255},  // Emerald
        Color{220, 60, 240, 255},  // Magenta
        Color{240, 240, 245, 255}  // White
    };

    float swatchW = 45.0f;
    for (size_t i = 0; i < 6; ++i) {
        Rectangle sRect{panelX + 16 + i * 52, y, swatchW, 20};
        DrawRectangleRounded(sRect, 0.25f, 3, swatches[i]);
        if (settings.color.r == swatches[i].r && settings.color.g == swatches[i].g && settings.color.b == swatches[i].b) {
            DrawRectangleRoundedLines(sRect, 0.25f, 3, WHITE);
        } else {
            DrawRectangleRoundedLines(sRect, 0.25f, 3, Fade(GRAY, 0.4f));
        }
        if (CheckCollisionPointRec(GetMousePosition(), sRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            settings.color = swatches[i];
        }
    }
    y += 32;

    // --- Spawn As Static Toggle ---
    if (DrawButton(Rectangle{panelX + 16, y, 310, 26},
                   settings.isStatic ? "ANCHORED (STATIC) SPAWN: [ENABLED]" : "DYNAMIC SPAWN: [NORMAL]",
                   Color{30, 40, 60, 230}, settings.isStatic ? GOLD : RAYWHITE, settings.isStatic)) {
        settings.isStatic = !settings.isStatic;
    }
    y += 32;

    // Trajectory prediction length
    DrawText("Trajectory Preview Steps:", static_cast<int>(panelX + 16), static_cast<int>(y), 11, LIGHTGRAY);
    y += 16;

    if (DrawButton(Rectangle{panelX + 16, y, 98, 24}, "Short (150)", Color{30, 40, 55, 230}, RAYWHITE,
                   settings.trajectorySteps == 150)) {
        settings.trajectorySteps = 150;
    }
    if (DrawButton(Rectangle{panelX + 122, y, 98, 24}, "Med (250)", Color{30, 40, 55, 230}, RAYWHITE,
                   settings.trajectorySteps == 250)) {
        settings.trajectorySteps = 250;
    }
    if (DrawButton(Rectangle{panelX + 228, y, 98, 24}, "Long (400)", Color{30, 40, 55, 230}, RAYWHITE,
                   settings.trajectorySteps == 400)) {
        settings.trajectorySteps = 400;
    }
}

bool Renderer::isMouseOverUI(Vector2 mousePos, bool hasSelection, bool showLauncherSettings,
                             int screenWidth, int screenHeight) const {
    // 1. Diagnostics panel (Top-Left)
    if (CheckCollisionPointRec(mousePos, Rectangle{15, 15, 340, 265})) return true;

    // 2. Preset cheat-sheet (Bottom-Left)
    if (CheckCollisionPointRec(mousePos, Rectangle{15, static_cast<float>(screenHeight - 115), 460, 100})) return true;

    // 3. Spawner menu toggle button (Top-Right)
    if (CheckCollisionPointRec(mousePos, Rectangle{static_cast<float>(screenWidth - 215), 15.0f, 200.0f, 32.0f})) return true;

    // 4. Object Inspector panel (Right)
    if (hasSelection && CheckCollisionPointRec(mousePos, Rectangle{static_cast<float>(screenWidth - 360), 55.0f, 345.0f, 550.0f})) return true;

    // 5. Launcher Settings panel (Right)
    if (showLauncherSettings && CheckCollisionPointRec(mousePos, Rectangle{static_cast<float>(screenWidth - 360), 55.0f, 345.0f, 550.0f})) return true;

    return false;
}
