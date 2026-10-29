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
            DrawCircleLines(static_cast<int>(pos.x), static_cast<int>(pos.y), r + 3.5f, Fade(GOLD, 0.65f));
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
    DrawLineEx(Vector2{pos.x - boxR, pos.y - boxR}, Vector2{pos.x - boxR + bracketLen, pos.y - boxR}, 2.0f, reticleCol);
    DrawLineEx(Vector2{pos.x - boxR, pos.y - boxR}, Vector2{pos.x - boxR, pos.y - boxR + bracketLen}, 2.0f, reticleCol);

    DrawLineEx(Vector2{pos.x + boxR, pos.y - boxR}, Vector2{pos.x + boxR - bracketLen, pos.y - boxR}, 2.0f, reticleCol);
    DrawLineEx(Vector2{pos.x + boxR, pos.y - boxR}, Vector2{pos.x + boxR, pos.y - boxR + bracketLen}, 2.0f, reticleCol);

    DrawLineEx(Vector2{pos.x - boxR, pos.y + boxR}, Vector2{pos.x - boxR + bracketLen, pos.y + boxR}, 2.0f, reticleCol);
    DrawLineEx(Vector2{pos.x - boxR, pos.y + boxR}, Vector2{pos.x - boxR, pos.y + boxR - bracketLen}, 2.0f, reticleCol);

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
    Vector2 sp = spawnPos.toVector2();
    float r = static_cast<float>(spawnRadius);
    DrawCircleV(sp, r * 1.4f, Fade(spawnColor, 0.3f));
    DrawCircleV(sp, r, Fade(spawnColor, 0.7f));
    DrawCircleLines(static_cast<int>(sp.x), static_cast<int>(sp.y), r, WHITE);

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

bool Renderer::DrawButton(Rectangle rect, const char* text, Color bg, Color textCol, bool isActive, int fontSize) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);

    Color drawBg = bg;
    Color borderCol = isActive ? GOLD : (hovered ? SKYBLUE : Fade(GRAY, 0.5f));

    if (isActive) {
        drawBg = Color{
            static_cast<unsigned char>(std::min(255, drawBg.r + 35)),
            static_cast<unsigned char>(std::min(255, drawBg.g + 45)),
            static_cast<unsigned char>(std::min(255, drawBg.b + 55)),
            drawBg.a
        };
    } else if (hovered) {
        drawBg = Color{
            static_cast<unsigned char>(std::min(255, drawBg.r + 28)),
            static_cast<unsigned char>(std::min(255, drawBg.g + 28)),
            static_cast<unsigned char>(std::min(255, drawBg.b + 28)),
            drawBg.a
        };
    }

    DrawRectangleRounded(rect, 0.22f, 4, drawBg);
    DrawRectangleRoundedLines(rect, 0.22f, 4, borderCol);

    int textW = MeasureText(text, fontSize);
    int textX = static_cast<int>(rect.x + (rect.width - textW) * 0.5f);
    int textY = static_cast<int>(rect.y + (rect.height - fontSize) * 0.5f);

    DrawText(text, textX, textY, fontSize, hovered ? WHITE : textCol);

    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void Renderer::DrawCard(Rectangle rect, const char* title, Color titleCol) {
    DrawRectangleRounded(rect, 0.05f, 6, Fade(Color{14, 20, 32, 255}, 0.88f));
    DrawRectangleRoundedLines(rect, 0.05f, 6, Fade(Color{55, 75, 110, 255}, 0.55f));

    if (title != nullptr) {
        DrawText(title, static_cast<int>(rect.x + 14), static_cast<int>(rect.y + 10), 13, titleCol);
        DrawLine(static_cast<int>(rect.x + 12), static_cast<int>(rect.y + 28),
                 static_cast<int>(rect.x + rect.width - 12), static_cast<int>(rect.y + 28),
                 Fade(DARKGRAY, 0.45f));
    }
}

void Renderer::drawHUD(const GravityEngine& engine, float timeScale, bool isPaused,
                      const LaunchSettings& launchSettings, int /*screenWidth*/, int screenHeight) const {
    // 1. Top-Left Diagnostics Panel
    int panelW = 340;
    int panelH = 265;
    DrawRectangleRounded(Rectangle{15, 15, static_cast<float>(panelW), static_cast<float>(panelH)}, 0.08f, 6, Fade(BLACK, 0.84f));
    DrawRectangleRoundedLines(Rectangle{15, 15, static_cast<float>(panelW), static_cast<float>(panelH)}, 0.08f, 6, Fade(SKYBLUE, 0.6f));

    DrawText("2D N-BODY GRAVITY ENGINE", 28, 25, 16, SKYBLUE);
    DrawLine(28, 46, 28 + panelW - 26, 46, Fade(DARKGRAY, 0.5f));

    int y = 55;
    int lineSpacing = 19;

    DrawText(TextFormat("FPS: %d", GetFPS()), 28, y, 13, GREEN);
    DrawText(TextFormat("Bodies: %zu", engine.getBodyCount()), 160, y, 13, RAYWHITE);
    y += lineSpacing;

    DrawText("Integrator [I]:", 28, y, 13, LIGHTGRAY);
    DrawText(engine.getIntegratorName(), 160, y, 13,
             (engine.integrator == IntegratorType::VelocityVerlet) ? GOLD : ORANGE);
    y += lineSpacing;

    DrawText("Collision [C]:", 28, y, 13, LIGHTGRAY);
    DrawText(engine.getCollisionModeName(), 160, y, 13,
             (engine.collisionMode == CollisionMode::InelasticMerge) ? VIOLET : (engine.collisionMode == CollisionMode::Elastic ? LIME : GRAY));
    y += lineSpacing;

    DrawText("Status [SPACE]:", 28, y, 13, LIGHTGRAY);
    DrawText(isPaused ? "PAUSED" : "RUNNING", 160, y, 13, isPaused ? RED : LIME);
    y += lineSpacing;

    DrawText("Time Scale [</>]:", 28, y, 13, LIGHTGRAY);
    DrawText(TextFormat("%.1fx (substeps: %d)", timeScale, static_cast<int>(timeScale * 5)), 160, y, 13, RAYWHITE);
    y += lineSpacing;

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

    DrawText("Payload [TAB]:", 28, y, 13, LIGHTGRAY);
    DrawText(TextFormat("%s (M=%.1f)", launchSettings.typeName.c_str(), launchSettings.mass), 160, y, 13, YELLOW);
    y += lineSpacing;

    DrawText("Camera Zoom:", 28, y, 13, LIGHTGRAY);
    DrawText(TextFormat("%.2fx %s", camera.zoom, (followedBodyId != -1 ? "[LOCKED]" : "")), 160, y, 13, RAYWHITE);

    // 2. Bottom-Left Preset & Controls Cheat-sheet
    int cheatW = 460;
    int cheatH = 100;
    float cheatY = static_cast<float>(screenHeight - cheatH - 15);
    DrawRectangleRounded(Rectangle{15, cheatY, static_cast<float>(cheatW), static_cast<float>(cheatH)}, 0.08f, 6, Fade(BLACK, 0.84f));
    DrawRectangleRoundedLines(Rectangle{15, cheatY, static_cast<float>(cheatW), static_cast<float>(cheatH)}, 0.08f, 6, Fade(GRAY, 0.4f));

    DrawText("PRESETS: [1] Solar System  [2] Figure-8 Orbit  [3] Binary Disk", 26, static_cast<int>(cheatY + 12), 12, GOLD);
    DrawText("CONTROLS:", 26, static_cast<int>(cheatY + 34), 12, SKYBLUE);
    DrawText("Click Body: Inspect in Sidebar | Drag: Slingshot | [H] Toggle Sidebar", 26, static_cast<int>(cheatY + 52), 11, RAYWHITE);
    DrawText("[TAB] Spawner Tab | [T] Trails | [V] Vectors | [G] Grid | [R] Reset | [X] Clear", 26, static_cast<int>(cheatY + 70), 11, RAYWHITE);
}

void Renderer::drawSidebarTogglePill(int screenWidth, int /*screenHeight*/) {
    Rectangle pillRect{static_cast<float>(screenWidth - 160), 16.0f, 145.0f, 36.0f};
    if (DrawButton(pillRect, "<< SHOW SIDEBAR [H]", Color{18, 28, 46, 240}, GOLD, false, 12)) {
        sidebarVisible = true;
    }
}

void Renderer::drawSidebar(Body* selectedBody, GravityEngine& engine, LaunchSettings& launchSettings,
                           int screenWidth, int screenHeight, bool& shouldDeselect, bool& shouldDelete) {
    if (!sidebarVisible) {
        drawSidebarTogglePill(screenWidth, screenHeight);
        return;
    }

    float barX = static_cast<float>(screenWidth) - sidebarWidth;
    float barW = sidebarWidth;
    float barH = static_cast<float>(screenHeight);
    float xStart = barX + 18.0f;
    float contentW = barW - 36.0f;

    // Full-height dark translucent sidebar background with glowing border line
    DrawRectangle(static_cast<int>(barX), 0, static_cast<int>(barW), static_cast<int>(barH), Color{10, 14, 23, 248});
    DrawLineEx(Vector2{barX, 0.0f}, Vector2{barX, barH}, 2.5f, Fade(GOLD, 0.7f));

    // Top Header & Tab Controls
    float y = 14.0f;

    Rectangle hideBtnRect{xStart, y, 105, 32};
    if (DrawButton(hideBtnRect, "HIDE >> [H]", Color{28, 38, 54, 230}, LIGHTGRAY, false, 12)) {
        sidebarVisible = false;
        return;
    }

    Rectangle tabInspRect{xStart + 114, y, 160, 32};
    bool isInspActive = (activeTab == SidebarTab::Inspector);
    if (DrawButton(tabInspRect, "OBJECT INSPECTOR", isInspActive ? Color{42, 60, 90, 240} : Color{20, 26, 38, 200},
                   isInspActive ? GOLD : GRAY, isInspActive, 12)) {
        activeTab = SidebarTab::Inspector;
    }

    Rectangle tabSpawnRect{xStart + 282, y, 162, 32};
    bool isSpawnActive = (activeTab == SidebarTab::Spawner);
    if (DrawButton(tabSpawnRect, "SPAWNER SETTINGS", isSpawnActive ? Color{42, 60, 90, 240} : Color{20, 26, 38, 200},
                   isSpawnActive ? SKYBLUE : GRAY, isSpawnActive, 12)) {
        activeTab = SidebarTab::Spawner;
    }

    y += 42.0f;
    DrawLineEx(Vector2{xStart, y}, Vector2{xStart + contentW, y}, 1.0f, Fade(DARKGRAY, 0.5f));
    y += 12.0f;

    // --- TAB 1: OBJECT INSPECTOR ---
    if (activeTab == SidebarTab::Inspector) {
        if (!selectedBody) {
            // Spacious Empty State
            y += 60.0f;
            DrawText("NO CELESTIAL OBJECT SELECTED", static_cast<int>(xStart + 45), static_cast<int>(y), 17, GOLD);
            y += 35.0f;

            DrawCard(Rectangle{xStart, y, contentW, 240.0f}, "HOW TO USE THE INSPECTOR", SKYBLUE);
            float ey = y + 42.0f;
            DrawText("1. Click any planet, star, or debris particle in space.", static_cast<int>(xStart + 20), static_cast<int>(ey), 14, RAYWHITE);
            ey += 28.0f;
            DrawText("2. View surface gravity, escape velocity & orbits.", static_cast<int>(xStart + 20), static_cast<int>(ey), 14, RAYWHITE);
            ey += 28.0f;
            DrawText("3. Adjust mass, radius, and density with live steppers.", static_cast<int>(xStart + 20), static_cast<int>(ey), 14, RAYWHITE);
            ey += 28.0f;
            DrawText("4. Lock camera tracking, freeze velocity, or circularize.", static_cast<int>(xStart + 20), static_cast<int>(ey), 14, RAYWHITE);
            ey += 28.0f;
            DrawText("5. Recolor bodies, clear trails, or destroy objects.", static_cast<int>(xStart + 20), static_cast<int>(ey), 14, RAYWHITE);

            y += 265.0f;
            Rectangle switchTabBtn{xStart + 60, y, contentW - 120, 42};
            if (DrawButton(switchTabBtn, "OPEN SPAWNER SETTINGS [TAB]", Color{26, 48, 80, 230}, SKYBLUE, false, 14)) {
                activeTab = SidebarTab::Spawner;
            }
            return;
        }

        // Section 1: Body Identity Card (Spacious)
        float card1H = 74.0f;
        DrawCard(Rectangle{xStart, y, contentW, card1H});

        Vector2 thumbPos{xStart + 36, y + card1H * 0.5f};
        DrawCircleV(thumbPos, 20.0f, Fade(selectedBody->color, 0.35f));
        DrawCircleV(thumbPos, 14.0f, selectedBody->color);
        DrawCircleLines(static_cast<int>(thumbPos.x), static_cast<int>(thumbPos.y), 14, WHITE);

        DrawText(selectedBody->name.c_str(), static_cast<int>(xStart + 70), static_cast<int>(y + 14), 20, RAYWHITE);
        DrawText(selectedBody->isStatic ? "[STATIC ANCHOR / FIXED]" : "[DYNAMIC IN KEPLERIAN ORBIT]",
                 static_cast<int>(xStart + 70), static_cast<int>(y + 42), 12, selectedBody->isStatic ? GOLD : SKYBLUE);

        Rectangle deselectBtn{xStart + contentW - 120, y + 20, 110, 34};
        if (DrawButton(deselectBtn, "DESELECT [X]", Color{55, 25, 25, 220}, RED, false, 12)) {
            shouldDeselect = true;
        }
        y += card1H + 12.0f;

        // Section 2: Physical Mass & Dimensions Card
        float card2H = 175.0f;
        DrawCard(Rectangle{xStart, y, contentW, card2H}, "PHYSICAL MASS & DIMENSIONS", GOLD);
        float py = y + 36.0f;

        // Mass Row
        DrawText("Mass:", static_cast<int>(xStart + 16), static_cast<int>(py + 2), 14, LIGHTGRAY);
        DrawText(TextFormat("%.3e kg", selectedBody->mass), static_cast<int>(xStart + 105), static_cast<int>(py), 16, YELLOW);
        py += 24.0f;

        float btnW4 = (contentW - 32.0f - 18.0f) / 4.0f;
        // Mass Steppers Row 1
        if (DrawButton(Rectangle{xStart + 16, py, btnW4, 26}, "-50%", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            selectedBody->setMass(selectedBody->mass * 0.5);
        }
        if (DrawButton(Rectangle{xStart + 16 + (btnW4 + 6), py, btnW4, 26}, "-10%", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            selectedBody->setMass(selectedBody->mass * 0.9);
        }
        if (DrawButton(Rectangle{xStart + 16 + 2 * (btnW4 + 6), py, btnW4, 26}, "+10%", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            selectedBody->setMass(selectedBody->mass * 1.1);
        }
        if (DrawButton(Rectangle{xStart + 16 + 3 * (btnW4 + 6), py, btnW4, 26}, "+50%", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            selectedBody->setMass(selectedBody->mass * 1.5);
        }
        py += 30.0f;

        // Mass Multipliers Row 2
        if (DrawButton(Rectangle{xStart + 16, py, btnW4, 26}, "/10", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            selectedBody->setMass(selectedBody->mass * 0.1);
        }
        if (DrawButton(Rectangle{xStart + 16 + (btnW4 + 6), py, btnW4, 26}, "/2", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            selectedBody->setMass(selectedBody->mass * 0.5);
        }
        if (DrawButton(Rectangle{xStart + 16 + 2 * (btnW4 + 6), py, btnW4, 26}, "x2", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            selectedBody->setMass(selectedBody->mass * 2.0);
        }
        if (DrawButton(Rectangle{xStart + 16 + 3 * (btnW4 + 6), py, btnW4, 26}, "x10", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            selectedBody->setMass(selectedBody->mass * 10.0);
        }
        py += 34.0f;

        // Radius & Density Rows
        DrawText("Radius:", static_cast<int>(xStart + 16), static_cast<int>(py + 2), 14, LIGHTGRAY);
        DrawText(TextFormat("%.1f px", selectedBody->radius), static_cast<int>(xStart + 80), static_cast<int>(py + 2), 15, RAYWHITE);

        if (DrawButton(Rectangle{xStart + 155, py, 60, 24}, "-1 px", Color{28, 40, 58, 220}, RAYWHITE, false, 11)) {
            selectedBody->setRadius(selectedBody->radius - 1.0);
        }
        if (DrawButton(Rectangle{xStart + 220, py, 60, 24}, "+1 px", Color{28, 40, 58, 220}, RAYWHITE, false, 11)) {
            selectedBody->setRadius(selectedBody->radius + 1.0);
        }

        DrawText("Density:", static_cast<int>(xStart + 295), static_cast<int>(py + 2), 14, LIGHTGRAY);
        DrawText(TextFormat("%.3f", selectedBody->getDensity()), static_cast<int>(xStart + 360), static_cast<int>(py + 2), 15, GREEN);
        py += 28.0f;

        y += card2H + 12.0f;

        // Section 3: Gravitational Field & Kinematics Card (Spacious, fills center)
        float card3H = 205.0f;
        DrawCard(Rectangle{xStart, y, contentW, card3H}, "GRAVITATIONAL FIELD & KINEMATICS", SKYBLUE);
        float gy = y + 36.0f;
        float rowSpacing = 22.0f;

        double surfG = selectedBody->getSurfaceGravity(engine.G, engine.epsilon);
        double escV = selectedBody->getEscapeVelocity(engine.G);
        double speed = selectedBody->vel.length();
        double accPull = selectedBody->acc.length();
        double mu = engine.G * selectedBody->mass;

        DrawText("Surface Gravity (g):", static_cast<int>(xStart + 16), static_cast<int>(gy), 14, LIGHTGRAY);
        DrawText(TextFormat("%.4e m/s²", surfG), static_cast<int>(xStart + 225), static_cast<int>(gy), 15, GOLD);
        gy += rowSpacing;

        DrawText("Escape Velocity (V_esc):", static_cast<int>(xStart + 16), static_cast<int>(gy), 14, LIGHTGRAY);
        DrawText(TextFormat("%.2f m/s", escV), static_cast<int>(xStart + 225), static_cast<int>(gy), 15, GOLD);
        gy += rowSpacing;

        DrawText("Gravitational Param (μ):", static_cast<int>(xStart + 16), static_cast<int>(gy), 14, LIGHTGRAY);
        DrawText(TextFormat("%.3e", mu), static_cast<int>(xStart + 225), static_cast<int>(gy), 14, RAYWHITE);
        gy += rowSpacing;

        DrawText("Current Velocity (|v|):", static_cast<int>(xStart + 16), static_cast<int>(gy), 14, LIGHTGRAY);
        DrawText(TextFormat("%.2f m/s", speed), static_cast<int>(xStart + 225), static_cast<int>(gy), 15, LIME);
        gy += rowSpacing;

        DrawText("Net Gravity Force (|a|):", static_cast<int>(xStart + 16), static_cast<int>(gy), 14, LIGHTGRAY);
        DrawText(TextFormat("%.4e m/s²", accPull), static_cast<int>(xStart + 225), static_cast<int>(gy), 15, ORANGE);
        gy += rowSpacing;

        DrawText("World Position (X, Y):", static_cast<int>(xStart + 16), static_cast<int>(gy), 14, LIGHTGRAY);
        DrawText(TextFormat("(%.1f,  %.1f)", selectedBody->pos.x, selectedBody->pos.y), static_cast<int>(xStart + 225), static_cast<int>(gy), 14, RAYWHITE);
        gy += rowSpacing;

        DrawText("Velocity Vector (Vx, Vy):", static_cast<int>(xStart + 16), static_cast<int>(gy), 14, LIGHTGRAY);
        DrawText(TextFormat("(%.2f,  %.2f)", selectedBody->vel.x, selectedBody->vel.y), static_cast<int>(xStart + 225), static_cast<int>(gy), 14, RAYWHITE);

        y += card3H + 12.0f;

        // Section 4: Motion & Orbital Commands Card (Tactile, large buttons)
        float card4H = 145.0f;
        DrawCard(Rectangle{xStart, y, contentW, card4H}, "ORBIT & MOTION COMMANDS", GOLD);
        float oy = y + 36.0f;
        float btnW2 = (contentW - 32.0f - 10.0f) * 0.5f;

        bool isCamLocked = (followedBodyId == selectedBody->id);
        if (DrawButton(Rectangle{xStart + 16, oy, btnW2, 34}, isCamLocked ? "UNLOCK CAMERA" : "LOCK CAMERA TRACK",
                       Color{28, 45, 72, 230}, isCamLocked ? GOLD : RAYWHITE, isCamLocked, 13)) {
            followedBodyId = isCamLocked ? -1 : selectedBody->id;
        }

        if (DrawButton(Rectangle{xStart + 16 + btnW2 + 10, oy, btnW2, 34},
                       selectedBody->isStatic ? "MAKE DYNAMIC" : "MAKE STATIC ANCHOR",
                       Color{28, 45, 72, 230}, selectedBody->isStatic ? GOLD : RAYWHITE, selectedBody->isStatic, 13)) {
            selectedBody->isStatic = !selectedBody->isStatic;
            if (selectedBody->isStatic) selectedBody->vel = Vec2{0.0, 0.0};
        }
        oy += 40.0f;

        if (DrawButton(Rectangle{xStart + 16, oy, btnW2, 34}, "ZERO VELOCITY (HALT)", Color{45, 36, 60, 230}, RAYWHITE, false, 13)) {
            selectedBody->vel = Vec2{0.0, 0.0};
        }

        if (DrawButton(Rectangle{xStart + 16 + btnW2 + 10, oy, btnW2, 34}, "AUTO CIRCULARIZE", Color{20, 56, 42, 230}, LIME, false, 13)) {
            engine.circularizeOrbit(selectedBody->id);
        }
        oy += 40.0f;

        y += card4H + 12.0f;

        // Section 5: Appearance & Lifecycle Card (Utilizing bottom area)
        float card5H = 125.0f;
        DrawCard(Rectangle{xStart, y, contentW, card5H}, "COLOR PALETTE & LIFECYCLE", VIOLET);
        float ly = y + 36.0f;

        const Color swatches[] = {
            Color{255, 215, 30, 255},  // Gold / Sun
            Color{70, 160, 255, 255},  // Earth Cyan
            Color{240, 80, 60, 255},   // Mars Red
            Color{60, 230, 120, 255},  // Emerald
            Color{235, 175, 110, 255}, // Jupiter Amber
            Color{220, 60, 240, 255},  // Magenta
            Color{120, 245, 255, 255}, // Comet Aqua
            Color{240, 240, 245, 255}  // Pearl White
        };

        float swW = (contentW - 32.0f - 7 * 8.0f) / 8.0f;
        for (size_t i = 0; i < 8; ++i) {
            Rectangle sRect{xStart + 16 + i * (swW + 8.0f), ly, swW, 26};
            DrawRectangleRounded(sRect, 0.25f, 3, swatches[i]);
            if (selectedBody->color.r == swatches[i].r && selectedBody->color.g == swatches[i].g && selectedBody->color.b == swatches[i].b) {
                DrawRectangleRoundedLines(sRect, 0.25f, 3, WHITE);
            } else {
                DrawRectangleRoundedLines(sRect, 0.25f, 3, Fade(GRAY, 0.45f));
            }
            if (CheckCollisionPointRec(GetMousePosition(), sRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                selectedBody->color = swatches[i];
            }
        }
        ly += 36.0f;

        if (DrawButton(Rectangle{xStart + 16, ly, btnW2, 34}, "CLEAR ORBIT TRAIL", Color{38, 44, 56, 230}, RAYWHITE, false, 13)) {
            selectedBody->clearTrail();
        }

        if (DrawButton(Rectangle{xStart + 16 + btnW2 + 10, ly, btnW2, 34}, "DESTROY OBJECT", Color{85, 25, 25, 230}, RED, false, 13)) {
            shouldDelete = true;
        }
    }
    // --- TAB 2: SPAWNER SETTINGS (Spacious layout) ---
    else if (activeTab == SidebarTab::Spawner) {
        // Section 1: Payload Presets Card
        float card1H = 150.0f;
        DrawCard(Rectangle{xStart, y, contentW, card1H}, "CELESTIAL PAYLOAD PRESETS", GOLD);
        float py = y + 36.0f;

        float pBtnW = (contentW - 32.0f - 16.0f) / 3.0f;
        if (DrawButton(Rectangle{xStart + 16, py, pBtnW, 36}, "Asteroid", Color{26, 38, 56, 230}, RAYWHITE,
                       launchSettings.presetType == LaunchPresetType::Asteroid, 14)) {
            launchSettings.applyPreset(LaunchPresetType::Asteroid);
        }
        if (DrawButton(Rectangle{xStart + 16 + (pBtnW + 8), py, pBtnW, 36}, "Moon", Color{26, 38, 56, 230}, RAYWHITE,
                       launchSettings.presetType == LaunchPresetType::Moon, 14)) {
            launchSettings.applyPreset(LaunchPresetType::Moon);
        }
        if (DrawButton(Rectangle{xStart + 16 + 2 * (pBtnW + 8), py, pBtnW, 36}, "Planet", Color{26, 38, 56, 230}, RAYWHITE,
                       launchSettings.presetType == LaunchPresetType::Terrestrial, 14)) {
            launchSettings.applyPreset(LaunchPresetType::Terrestrial);
        }
        py += 44.0f;

        if (DrawButton(Rectangle{xStart + 16, py, pBtnW, 36}, "Gas Giant", Color{26, 38, 56, 230}, RAYWHITE,
                       launchSettings.presetType == LaunchPresetType::GasGiant, 14)) {
            launchSettings.applyPreset(LaunchPresetType::GasGiant);
        }
        if (DrawButton(Rectangle{xStart + 16 + (pBtnW + 8), py, pBtnW, 36}, "Star", Color{26, 38, 56, 230}, RAYWHITE,
                       launchSettings.presetType == LaunchPresetType::Star, 14)) {
            launchSettings.applyPreset(LaunchPresetType::Star);
        }
        if (DrawButton(Rectangle{xStart + 16 + 2 * (pBtnW + 8), py, pBtnW, 36}, "Singularity", Color{26, 38, 56, 230}, RAYWHITE,
                       launchSettings.presetType == LaunchPresetType::BlackHole, 14)) {
            launchSettings.applyPreset(LaunchPresetType::BlackHole);
        }

        y += card1H + 14.0f;

        // Section 2: Spawn Injection Mode Card
        float card2H = 95.0f;
        DrawCard(Rectangle{xStart, y, contentW, card2H}, "SPAWN INJECTION MODE", GOLD);
        float my = y + 36.0f;

        if (DrawButton(Rectangle{xStart + 16, my, pBtnW, 38}, "Slingshot (Drag)", Color{26, 44, 68, 230}, RAYWHITE,
                       launchSettings.spawnMode == LaunchSpawnMode::Slingshot, 12)) {
            launchSettings.spawnMode = LaunchSpawnMode::Slingshot;
        }
        if (DrawButton(Rectangle{xStart + 16 + (pBtnW + 8), my, pBtnW, 38}, "Auto Orbit (Click)", Color{26, 44, 68, 230}, RAYWHITE,
                       launchSettings.spawnMode == LaunchSpawnMode::CircularOrbit, 12)) {
            launchSettings.spawnMode = LaunchSpawnMode::CircularOrbit;
        }
        if (DrawButton(Rectangle{xStart + 16 + 2 * (pBtnW + 8), my, pBtnW, 38}, "Stationary (Click)", Color{26, 44, 68, 230}, RAYWHITE,
                       launchSettings.spawnMode == LaunchSpawnMode::Stationary, 12)) {
            launchSettings.spawnMode = LaunchSpawnMode::Stationary;
        }

        y += card2H + 14.0f;

        // Section 3: Custom Properties Card
        float card3H = 205.0f;
        DrawCard(Rectangle{xStart, y, contentW, card3H}, "CUSTOM PHYSICAL PROPERTIES", SKYBLUE);
        float cy = y + 36.0f;

        DrawText("Launch Mass:", static_cast<int>(xStart + 16), static_cast<int>(cy + 2), 14, LIGHTGRAY);
        DrawText(TextFormat("%.3e kg", launchSettings.mass), static_cast<int>(xStart + 140), static_cast<int>(cy), 16, YELLOW);
        cy += 26.0f;

        float btnW4 = (contentW - 32.0f - 18.0f) / 4.0f;
        if (DrawButton(Rectangle{xStart + 16, cy, btnW4, 28}, "-50%", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            launchSettings.mass = std::max(launchSettings.mass * 0.5, 0.01);
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{xStart + 16 + (btnW4 + 6), cy, btnW4, 28}, "-10%", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            launchSettings.mass = std::max(launchSettings.mass * 0.9, 0.01);
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{xStart + 16 + 2 * (btnW4 + 6), cy, btnW4, 28}, "+10%", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            launchSettings.mass *= 1.1;
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{xStart + 16 + 3 * (btnW4 + 6), cy, btnW4, 28}, "+50%", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            launchSettings.mass *= 1.5;
            launchSettings.recalculateRadiusFromDensity();
        }
        cy += 36.0f;

        DrawText("Launch Radius:", static_cast<int>(xStart + 16), static_cast<int>(cy + 2), 14, LIGHTGRAY);
        DrawText(TextFormat("%.1f px", launchSettings.radius), static_cast<int>(xStart + 140), static_cast<int>(cy + 2), 15, RAYWHITE);

        if (DrawButton(Rectangle{xStart + 240, cy, 80, 26}, "-1 px", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            launchSettings.radius = std::max(launchSettings.radius - 1.0, 1.5);
            launchSettings.recalculateDensityFromRadius();
        }
        if (DrawButton(Rectangle{xStart + 330, cy, 80, 26}, "+1 px", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            launchSettings.radius += 1.0;
            launchSettings.recalculateDensityFromRadius();
        }
        cy += 34.0f;

        DrawText("Launch Density:", static_cast<int>(xStart + 16), static_cast<int>(cy + 2), 14, LIGHTGRAY);
        DrawText(TextFormat("%.4f kg/px²", launchSettings.density), static_cast<int>(xStart + 140), static_cast<int>(cy + 2), 15, GREEN);

        if (DrawButton(Rectangle{xStart + 240, cy, 80, 26}, "-20%", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            launchSettings.density = std::max(launchSettings.density * 0.8, 0.001);
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{xStart + 330, cy, 80, 26}, "+20%", Color{28, 40, 58, 220}, RAYWHITE, false, 12)) {
            launchSettings.density *= 1.2;
            launchSettings.recalculateRadiusFromDensity();
        }

        y += card3H + 14.0f;

        // Section 4: Color, Anchor & Trajectory Card
        float card4H = 175.0f;
        DrawCard(Rectangle{xStart, y, contentW, card4H}, "COLOR & INJECTION CONTROLS", VIOLET);
        float vy = y + 36.0f;

        const Color swatches[] = {
            Color{70, 160, 255, 255},  // Cyan
            Color{255, 215, 30, 255},  // Gold
            Color{240, 80, 60, 255},   // Ruby
            Color{60, 230, 120, 255},  // Emerald
            Color{235, 175, 110, 255}, // Amber
            Color{220, 60, 240, 255},  // Magenta
            Color{120, 245, 255, 255}, // Aqua
            Color{240, 240, 245, 255}  // Pearl
        };

        float swW = (contentW - 32.0f - 7 * 8.0f) / 8.0f;
        for (size_t i = 0; i < 8; ++i) {
            Rectangle sRect{xStart + 16 + i * (swW + 8.0f), vy, swW, 26};
            DrawRectangleRounded(sRect, 0.25f, 3, swatches[i]);
            if (launchSettings.color.r == swatches[i].r && launchSettings.color.g == swatches[i].g && launchSettings.color.b == swatches[i].b) {
                DrawRectangleRoundedLines(sRect, 0.25f, 3, WHITE);
            } else {
                DrawRectangleRoundedLines(sRect, 0.25f, 3, Fade(GRAY, 0.45f));
            }
            if (CheckCollisionPointRec(GetMousePosition(), sRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                launchSettings.color = swatches[i];
            }
        }
        vy += 36.0f;

        if (DrawButton(Rectangle{xStart + 16, vy, contentW - 32.0f, 32},
                       launchSettings.isStatic ? "ANCHORED (STATIC) SPAWN: [ENABLED]" : "DYNAMIC SPAWN: [NORMAL]",
                       Color{28, 44, 68, 230}, launchSettings.isStatic ? GOLD : RAYWHITE, launchSettings.isStatic, 13)) {
            launchSettings.isStatic = !launchSettings.isStatic;
        }
        vy += 38.0f;

        DrawText("Trajectory Preview Steps:", static_cast<int>(xStart + 16), static_cast<int>(vy + 4), 13, LIGHTGRAY);
        if (DrawButton(Rectangle{xStart + 195, vy, 68, 26}, "150", Color{28, 38, 54, 230}, RAYWHITE,
                       launchSettings.trajectorySteps == 150, 12)) {
            launchSettings.trajectorySteps = 150;
        }
        if (DrawButton(Rectangle{xStart + 270, vy, 68, 26}, "250", Color{28, 38, 54, 230}, RAYWHITE,
                       launchSettings.trajectorySteps == 250, 12)) {
            launchSettings.trajectorySteps = 250;
        }
        if (DrawButton(Rectangle{xStart + 345, vy, 68, 26}, "400", Color{28, 38, 54, 230}, RAYWHITE,
                       launchSettings.trajectorySteps == 400, 12)) {
            launchSettings.trajectorySteps = 400;
        }
    }
}

bool Renderer::isMouseOverUI(Vector2 mousePos, int screenWidth, int screenHeight) const {
    if (CheckCollisionPointRec(mousePos, Rectangle{15, 15, 340, 265})) return true;
    if (CheckCollisionPointRec(mousePos, Rectangle{15, static_cast<float>(screenHeight - 115), 460, 100})) return true;

    if (sidebarVisible) {
        if (mousePos.x >= screenWidth - sidebarWidth) return true;
    } else {
        if (CheckCollisionPointRec(mousePos, Rectangle{static_cast<float>(screenWidth - 160), 16.0f, 145.0f, 36.0f})) return true;
    }

    return false;
}
