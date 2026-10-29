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

    DrawRectangleRounded(rect, 0.22f, 4, drawBg);
    DrawRectangleRoundedLines(rect, 0.22f, 4, borderCol);

    int fontSize = 11;
    int textW = MeasureText(text, fontSize);
    int textX = static_cast<int>(rect.x + (rect.width - textW) * 0.5f);
    int textY = static_cast<int>(rect.y + (rect.height - fontSize) * 0.5f);

    DrawText(text, textX, textY, fontSize, hovered ? WHITE : textCol);

    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
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
    DrawText("Payload [TAB]:", 28, y, 13, LIGHTGRAY);
    DrawText(TextFormat("%s (M=%.1f)", launchSettings.typeName.c_str(), launchSettings.mass), 160, y, 13, YELLOW);
    y += lineSpacing;

    // Zoom level & Target
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
    // Show button when collapsed on the right screen edge
    Rectangle pillRect{static_cast<float>(screenWidth - 145), 18.0f, 132.0f, 32.0f};
    if (DrawButton(pillRect, "<< SHOW SIDEBAR", Color{20, 30, 48, 235}, GOLD)) {
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

    // Full-height dark glass background with glowing left border
    DrawRectangle(static_cast<int>(barX), 0, static_cast<int>(barW), static_cast<int>(barH), Color{11, 15, 24, 248});
    DrawLineEx(Vector2{barX, 0.0f}, Vector2{barX, barH}, 2.0f, Fade(GOLD, 0.65f));

    // Top Header & Collapse Controls
    float y = 14.0f;

    // Hide button
    Rectangle hideBtnRect{barX + 16, y, 92, 26};
    if (DrawButton(hideBtnRect, "HIDE >> [H]", Color{30, 40, 55, 220}, LIGHTGRAY)) {
        sidebarVisible = false;
        return;
    }

    // Tabs: [INSPECTOR] and [SPAWNER SETTINGS]
    Rectangle tabInspRect{barX + 116, y, 142, 26};
    bool isInspActive = (activeTab == SidebarTab::Inspector);
    if (DrawButton(tabInspRect, "OBJECT INSPECTOR", isInspActive ? Color{40, 55, 80, 240} : Color{22, 28, 40, 200},
                   isInspActive ? GOLD : GRAY, isInspActive)) {
        activeTab = SidebarTab::Inspector;
    }

    Rectangle tabSpawnRect{barX + 264, y, 148, 26};
    bool isSpawnActive = (activeTab == SidebarTab::Spawner);
    if (DrawButton(tabSpawnRect, "SPAWNER SETTINGS", isSpawnActive ? Color{40, 55, 80, 240} : Color{22, 28, 40, 200},
                   isSpawnActive ? SKYBLUE : GRAY, isSpawnActive)) {
        activeTab = SidebarTab::Spawner;
    }

    y += 34.0f;
    DrawLineEx(Vector2{barX + 12, y}, Vector2{barX + barW - 12, y}, 1.0f, Fade(DARKGRAY, 0.5f));
    y += 12.0f;

    // --- TAB CONTENT 1: OBJECT INSPECTOR ---
    if (activeTab == SidebarTab::Inspector) {
        if (!selectedBody) {
            // Empty State
            y += 40.0f;
            DrawText("NO CELESTIAL OBJECT SELECTED", static_cast<int>(barX + 40), static_cast<int>(y), 15, GOLD);
            y += 28.0f;

            DrawText("Click on any celestial body in space to inspect it:", static_cast<int>(barX + 24), static_cast<int>(y), 12, LIGHTGRAY);
            y += 22.0f;
            DrawText("- Modify mass, radius, and density in real-time.", static_cast<int>(barX + 30), static_cast<int>(y), 11, RAYWHITE);
            y += 18.0f;
            DrawText("- Analyze surface gravity pull, escape velocity & orbits.", static_cast<int>(barX + 30), static_cast<int>(y), 11, RAYWHITE);
            y += 18.0f;
            DrawText("- Lock camera tracking or circularize orbit.", static_cast<int>(barX + 30), static_cast<int>(y), 11, RAYWHITE);
            y += 18.0f;
            DrawText("- Toggle static anchor or halt velocity instantly.", static_cast<int>(barX + 30), static_cast<int>(y), 11, RAYWHITE);

            y += 45.0f;
            Rectangle switchTabBtn{barX + 80, y, 260, 32};
            if (DrawButton(switchTabBtn, "OPEN SPAWNER SETTINGS [TAB]", Color{25, 45, 75, 230}, SKYBLUE)) {
                activeTab = SidebarTab::Spawner;
            }
            return;
        }

        // Header Card with Body Thumbnail, Name, and Deselect Button
        Vector2 thumbPos{barX + 38, y + 20};
        DrawCircleV(thumbPos, 18.0f, Fade(selectedBody->color, 0.35f));
        DrawCircleV(thumbPos, 13.0f, selectedBody->color);
        DrawCircleLines(static_cast<int>(thumbPos.x), static_cast<int>(thumbPos.y), 13, WHITE);

        DrawText(selectedBody->name.c_str(), static_cast<int>(barX + 66), static_cast<int>(y + 8), 17, RAYWHITE);
        DrawText(selectedBody->isStatic ? "[STATIC ANCHOR]" : "[DYNAMIC BODY IN ORBIT]",
                 static_cast<int>(barX + 66), static_cast<int>(y + 28), 11, selectedBody->isStatic ? GOLD : SKYBLUE);

        Rectangle deselectBtn{barX + barW - 110, y + 10, 95, 24};
        if (DrawButton(deselectBtn, "DESELECT [X]", Color{50, 30, 35, 200}, RED)) {
            shouldDeselect = true;
        }

        y += 54.0f;
        DrawLineEx(Vector2{barX + 16, y}, Vector2{barX + barW - 16, y}, 1.0f, Fade(GRAY, 0.3f));
        y += 10.0f;

        // --- Physical Properties Controls ---
        DrawText("PHYSICAL PROPERTIES", static_cast<int>(barX + 18), static_cast<int>(y), 12, GOLD);
        y += 20.0f;

        // Mass
        DrawText("Mass:", static_cast<int>(barX + 18), static_cast<int>(y), 13, LIGHTGRAY);
        DrawText(TextFormat("%.3e kg", selectedBody->mass), static_cast<int>(barX + 110), static_cast<int>(y), 13, YELLOW);
        y += 20.0f;

        if (DrawButton(Rectangle{barX + 18, y, 44, 22}, "-50%", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setMass(selectedBody->mass * 0.5);
        }
        if (DrawButton(Rectangle{barX + 66, y, 44, 22}, "-10%", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setMass(selectedBody->mass * 0.9);
        }
        if (DrawButton(Rectangle{barX + 114, y, 44, 22}, "+10%", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setMass(selectedBody->mass * 1.1);
        }
        if (DrawButton(Rectangle{barX + 162, y, 44, 22}, "+50%", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setMass(selectedBody->mass * 1.5);
        }
        if (DrawButton(Rectangle{barX + 214, y, 44, 22}, "/2", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setMass(selectedBody->mass * 0.5);
        }
        if (DrawButton(Rectangle{barX + 262, y, 44, 22}, "x2", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setMass(selectedBody->mass * 2.0);
        }
        if (DrawButton(Rectangle{barX + 314, y, 44, 22}, "/10", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setMass(selectedBody->mass * 0.1);
        }
        if (DrawButton(Rectangle{barX + 362, y, 44, 22}, "x10", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setMass(selectedBody->mass * 10.0);
        }
        y += 30.0f;

        // Radius
        DrawText("Radius:", static_cast<int>(barX + 18), static_cast<int>(y), 13, LIGHTGRAY);
        DrawText(TextFormat("%.1f px", selectedBody->radius), static_cast<int>(barX + 110), static_cast<int>(y), 13, RAYWHITE);

        if (DrawButton(Rectangle{barX + 214, y - 2, 44, 22}, "-5 px", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setRadius(selectedBody->radius - 5.0);
        }
        if (DrawButton(Rectangle{barX + 262, y - 2, 44, 22}, "-1 px", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setRadius(selectedBody->radius - 1.0);
        }
        if (DrawButton(Rectangle{barX + 314, y - 2, 44, 22}, "+1 px", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setRadius(selectedBody->radius + 1.0);
        }
        if (DrawButton(Rectangle{barX + 362, y - 2, 44, 22}, "+5 px", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setRadius(selectedBody->radius + 5.0);
        }
        y += 30.0f;

        // Density
        DrawText("Density (ρ):", static_cast<int>(barX + 18), static_cast<int>(y), 13, LIGHTGRAY);
        DrawText(TextFormat("%.4f kg/px²", selectedBody->getDensity()), static_cast<int>(barX + 110), static_cast<int>(y), 13, GREEN);

        if (DrawButton(Rectangle{barX + 214, y - 2, 44, 22}, "-20%", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setDensity(selectedBody->getDensity() * 0.8);
        }
        if (DrawButton(Rectangle{barX + 262, y - 2, 44, 22}, "-5%", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setDensity(selectedBody->getDensity() * 0.95);
        }
        if (DrawButton(Rectangle{barX + 314, y - 2, 44, 22}, "+5%", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setDensity(selectedBody->getDensity() * 1.05);
        }
        if (DrawButton(Rectangle{barX + 362, y - 2, 44, 22}, "+20%", Color{30, 42, 60, 220}, RAYWHITE)) {
            selectedBody->setDensity(selectedBody->getDensity() * 1.2);
        }
        y += 34.0f;

        // --- Gravitational Field & Mechanics Section ---
        DrawLineEx(Vector2{barX + 16, y}, Vector2{barX + barW - 16, y}, 1.0f, Fade(GRAY, 0.3f));
        y += 10.0f;

        DrawText("GRAVITATIONAL FIELD & MECHANICS", static_cast<int>(barX + 18), static_cast<int>(y), 12, SKYBLUE);
        y += 22.0f;

        double surfG = selectedBody->getSurfaceGravity(engine.G, engine.epsilon);
        double escV = selectedBody->getEscapeVelocity(engine.G);
        double speed = selectedBody->vel.length();
        double accPull = selectedBody->acc.length();
        double mu = engine.G * selectedBody->mass;

        DrawText("Surface Gravity (g):", static_cast<int>(barX + 18), static_cast<int>(y), 12, LIGHTGRAY);
        DrawText(TextFormat("%.3e m/s²", surfG), static_cast<int>(barX + 210), static_cast<int>(y), 12, GOLD);
        y += 20.0f;

        DrawText("Escape Velocity (V_esc):", static_cast<int>(barX + 18), static_cast<int>(y), 12, LIGHTGRAY);
        DrawText(TextFormat("%.2f m/s", escV), static_cast<int>(barX + 210), static_cast<int>(y), 12, GOLD);
        y += 20.0f;

        DrawText("Grav. Parameter (μ):", static_cast<int>(barX + 18), static_cast<int>(y), 12, LIGHTGRAY);
        DrawText(TextFormat("%.3e", mu), static_cast<int>(barX + 210), static_cast<int>(y), 12, RAYWHITE);
        y += 20.0f;

        DrawText("Current Speed (|v|):", static_cast<int>(barX + 18), static_cast<int>(y), 12, LIGHTGRAY);
        DrawText(TextFormat("%.2f m/s", speed), static_cast<int>(barX + 210), static_cast<int>(y), 12, LIME);
        y += 20.0f;

        DrawText("Net Gravity Pull (|a|):", static_cast<int>(barX + 18), static_cast<int>(y), 12, LIGHTGRAY);
        DrawText(TextFormat("%.3e m/s²", accPull), static_cast<int>(barX + 210), static_cast<int>(y), 12, ORANGE);
        y += 20.0f;

        DrawText("World Position (X, Y):", static_cast<int>(barX + 18), static_cast<int>(y), 12, LIGHTGRAY);
        DrawText(TextFormat("(%.1f, %.1f)", selectedBody->pos.x, selectedBody->pos.y), static_cast<int>(barX + 210), static_cast<int>(y), 12, RAYWHITE);
        y += 20.0f;

        DrawText("Velocity Vector (Vx, Vy):", static_cast<int>(barX + 18), static_cast<int>(y), 12, LIGHTGRAY);
        DrawText(TextFormat("(%.2f, %.2f)", selectedBody->vel.x, selectedBody->vel.y), static_cast<int>(barX + 210), static_cast<int>(y), 12, RAYWHITE);
        y += 28.0f;

        // --- Action Buttons ---
        DrawLineEx(Vector2{barX + 16, y}, Vector2{barX + barW - 16, y}, 1.0f, Fade(GRAY, 0.3f));
        y += 10.0f;

        bool isCamLocked = (followedBodyId == selectedBody->id);
        if (DrawButton(Rectangle{barX + 18, y, 188, 28}, isCamLocked ? "UNLOCK CAMERA" : "LOCK CAMERA TRACK",
                       Color{28, 44, 68, 230}, isCamLocked ? GOLD : RAYWHITE, isCamLocked)) {
            followedBodyId = isCamLocked ? -1 : selectedBody->id;
        }

        if (DrawButton(Rectangle{barX + 214, y, 198, 28}, selectedBody->isStatic ? "MAKE DYNAMIC" : "MAKE STATIC ANCHOR",
                       Color{28, 44, 68, 230}, selectedBody->isStatic ? GOLD : RAYWHITE, selectedBody->isStatic)) {
            selectedBody->isStatic = !selectedBody->isStatic;
            if (selectedBody->isStatic) selectedBody->vel = Vec2{0.0, 0.0};
        }
        y += 34.0f;

        if (DrawButton(Rectangle{barX + 18, y, 188, 28}, "ZERO VELOCITY (HALT)", Color{40, 35, 58, 230}, RAYWHITE)) {
            selectedBody->vel = Vec2{0.0, 0.0};
        }

        if (DrawButton(Rectangle{barX + 214, y, 198, 28}, "AUTO CIRCULARIZE", Color{20, 52, 40, 230}, LIME)) {
            engine.circularizeOrbit(selectedBody->id);
        }
        y += 36.0f;

        // --- Color Palette ---
        DrawText("Recolor Body:", static_cast<int>(barX + 18), static_cast<int>(y), 12, LIGHTGRAY);
        y += 18.0f;

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

        float swW = 44.0f;
        for (size_t i = 0; i < 8; ++i) {
            Rectangle sRect{barX + 18 + i * 49, y, swW, 22};
            DrawRectangleRounded(sRect, 0.25f, 3, swatches[i]);
            DrawRectangleRoundedLines(sRect, 0.25f, 3, Fade(WHITE, 0.6f));
            if (CheckCollisionPointRec(GetMousePosition(), sRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                selectedBody->color = swatches[i];
            }
        }
        y += 34.0f;

        // Clear Trail & Delete Buttons
        if (DrawButton(Rectangle{barX + 18, y, 188, 28}, "CLEAR ORBIT TRAIL", Color{38, 42, 52, 230}, RAYWHITE)) {
            selectedBody->clearTrail();
        }

        if (DrawButton(Rectangle{barX + 214, y, 198, 28}, "DESTROY OBJECT", Color{80, 24, 24, 230}, RED)) {
            shouldDelete = true;
        }
    }
    // --- TAB CONTENT 2: SPAWNER SETTINGS ---
    else if (activeTab == SidebarTab::Spawner) {
        // Celestial Presets
        DrawText("SELECT PAYLOAD PRESET:", static_cast<int>(barX + 18), static_cast<int>(y), 12, GOLD);
        y += 20.0f;

        if (DrawButton(Rectangle{barX + 18, y, 124, 26}, "Asteroid", Color{28, 38, 54, 230}, RAYWHITE,
                       launchSettings.presetType == LaunchPresetType::Asteroid)) {
            launchSettings.applyPreset(LaunchPresetType::Asteroid);
        }
        if (DrawButton(Rectangle{barX + 152, y, 124, 26}, "Moon", Color{28, 38, 54, 230}, RAYWHITE,
                       launchSettings.presetType == LaunchPresetType::Moon)) {
            launchSettings.applyPreset(LaunchPresetType::Moon);
        }
        if (DrawButton(Rectangle{barX + 286, y, 124, 26}, "Planet", Color{28, 38, 54, 230}, RAYWHITE,
                       launchSettings.presetType == LaunchPresetType::Terrestrial)) {
            launchSettings.applyPreset(LaunchPresetType::Terrestrial);
        }
        y += 32.0f;

        if (DrawButton(Rectangle{barX + 18, y, 124, 26}, "Gas Giant", Color{28, 38, 54, 230}, RAYWHITE,
                       launchSettings.presetType == LaunchPresetType::GasGiant)) {
            launchSettings.applyPreset(LaunchPresetType::GasGiant);
        }
        if (DrawButton(Rectangle{barX + 152, y, 124, 26}, "Star", Color{28, 38, 54, 230}, RAYWHITE,
                       launchSettings.presetType == LaunchPresetType::Star)) {
            launchSettings.applyPreset(LaunchPresetType::Star);
        }
        if (DrawButton(Rectangle{barX + 286, y, 124, 26}, "Singularity", Color{28, 38, 54, 230}, RAYWHITE,
                       launchSettings.presetType == LaunchPresetType::BlackHole)) {
            launchSettings.applyPreset(LaunchPresetType::BlackHole);
        }
        y += 38.0f;

        // Spawn Mode
        DrawLineEx(Vector2{barX + 16, y}, Vector2{barX + barW - 16, y}, 1.0f, Fade(GRAY, 0.3f));
        y += 10.0f;

        DrawText("SPAWN INJECTION MODE:", static_cast<int>(barX + 18), static_cast<int>(y), 12, GOLD);
        y += 20.0f;

        if (DrawButton(Rectangle{barX + 18, y, 124, 26}, "Slingshot (Drag)", Color{28, 44, 66, 230}, RAYWHITE,
                       launchSettings.spawnMode == LaunchSpawnMode::Slingshot)) {
            launchSettings.spawnMode = LaunchSpawnMode::Slingshot;
        }
        if (DrawButton(Rectangle{barX + 152, y, 124, 26}, "Auto Orbit (Click)", Color{28, 44, 66, 230}, RAYWHITE,
                       launchSettings.spawnMode == LaunchSpawnMode::CircularOrbit)) {
            launchSettings.spawnMode = LaunchSpawnMode::CircularOrbit;
        }
        if (DrawButton(Rectangle{barX + 286, y, 124, 26}, "Stationary (Click)", Color{28, 44, 66, 230}, RAYWHITE,
                       launchSettings.spawnMode == LaunchSpawnMode::Stationary)) {
            launchSettings.spawnMode = LaunchSpawnMode::Stationary;
        }
        y += 38.0f;

        // Physical properties
        DrawLineEx(Vector2{barX + 16, y}, Vector2{barX + barW - 16, y}, 1.0f, Fade(GRAY, 0.3f));
        y += 10.0f;

        DrawText("CUSTOM PHYSICAL PROPERTIES:", static_cast<int>(barX + 18), static_cast<int>(y), 12, GOLD);
        y += 20.0f;

        DrawText("Launch Mass:", static_cast<int>(barX + 18), static_cast<int>(y), 13, LIGHTGRAY);
        DrawText(TextFormat("%.3e kg", launchSettings.mass), static_cast<int>(barX + 120), static_cast<int>(y), 13, YELLOW);
        y += 20.0f;

        if (DrawButton(Rectangle{barX + 18, y, 56, 22}, "-50%", Color{30, 42, 60, 220}, RAYWHITE)) {
            launchSettings.mass = std::max(launchSettings.mass * 0.5, 0.01);
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{barX + 80, y, 56, 22}, "-10%", Color{30, 42, 60, 220}, RAYWHITE)) {
            launchSettings.mass = std::max(launchSettings.mass * 0.9, 0.01);
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{barX + 142, y, 56, 22}, "+10%", Color{30, 42, 60, 220}, RAYWHITE)) {
            launchSettings.mass *= 1.1;
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{barX + 204, y, 56, 22}, "+50%", Color{30, 42, 60, 220}, RAYWHITE)) {
            launchSettings.mass *= 1.5;
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{barX + 276, y, 56, 22}, "/10", Color{30, 42, 60, 220}, RAYWHITE)) {
            launchSettings.mass = std::max(launchSettings.mass * 0.1, 0.01);
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{barX + 338, y, 56, 22}, "x10", Color{30, 42, 60, 220}, RAYWHITE)) {
            launchSettings.mass *= 10.0;
            launchSettings.recalculateRadiusFromDensity();
        }
        y += 32.0f;

        // Launch Radius
        DrawText("Launch Radius:", static_cast<int>(barX + 18), static_cast<int>(y), 13, LIGHTGRAY);
        DrawText(TextFormat("%.1f px", launchSettings.radius), static_cast<int>(barX + 120), static_cast<int>(y), 13, RAYWHITE);

        if (DrawButton(Rectangle{barX + 246, y - 2, 74, 22}, "-1 px", Color{30, 42, 60, 220}, RAYWHITE)) {
            launchSettings.radius = std::max(launchSettings.radius - 1.0, 1.5);
            launchSettings.recalculateDensityFromRadius();
        }
        if (DrawButton(Rectangle{barX + 330, y - 2, 74, 22}, "+1 px", Color{30, 42, 60, 220}, RAYWHITE)) {
            launchSettings.radius += 1.0;
            launchSettings.recalculateDensityFromRadius();
        }
        y += 30.0f;

        // Launch Density
        DrawText("Density (ρ):", static_cast<int>(barX + 18), static_cast<int>(y), 13, LIGHTGRAY);
        DrawText(TextFormat("%.4f kg/px²", launchSettings.density), static_cast<int>(barX + 120), static_cast<int>(y), 13, GREEN);

        if (DrawButton(Rectangle{barX + 246, y - 2, 74, 22}, "-20%", Color{30, 42, 60, 220}, RAYWHITE)) {
            launchSettings.density = std::max(launchSettings.density * 0.8, 0.001);
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{barX + 330, y - 2, 74, 22}, "+20%", Color{30, 42, 60, 220}, RAYWHITE)) {
            launchSettings.density *= 1.2;
            launchSettings.recalculateRadiusFromDensity();
        }
        y += 36.0f;

        // Launch Color
        DrawText("Launch Color Palette:", static_cast<int>(barX + 18), static_cast<int>(y), 12, LIGHTGRAY);
        y += 18.0f;

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

        float swW = 44.0f;
        for (size_t i = 0; i < 8; ++i) {
            Rectangle sRect{barX + 18 + i * 49, y, swW, 22};
            DrawRectangleRounded(sRect, 0.25f, 3, swatches[i]);
            if (launchSettings.color.r == swatches[i].r && launchSettings.color.g == swatches[i].g && launchSettings.color.b == swatches[i].b) {
                DrawRectangleRoundedLines(sRect, 0.25f, 3, WHITE);
            } else {
                DrawRectangleRoundedLines(sRect, 0.25f, 3, Fade(GRAY, 0.4f));
            }
            if (CheckCollisionPointRec(GetMousePosition(), sRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                launchSettings.color = swatches[i];
            }
        }
        y += 36.0f;

        // Anchor toggle
        if (DrawButton(Rectangle{barX + 18, y, 386, 28},
                       launchSettings.isStatic ? "ANCHORED (STATIC) SPAWN: [ENABLED]" : "DYNAMIC SPAWN: [NORMAL]",
                       Color{28, 42, 64, 230}, launchSettings.isStatic ? GOLD : RAYWHITE, launchSettings.isStatic)) {
            launchSettings.isStatic = !launchSettings.isStatic;
        }
        y += 36.0f;

        // Trajectory steps
        DrawText("Trajectory Prediction Steps:", static_cast<int>(barX + 18), static_cast<int>(y), 12, LIGHTGRAY);
        y += 18.0f;

        if (DrawButton(Rectangle{barX + 18, y, 124, 26}, "Short (150)", Color{28, 38, 54, 230}, RAYWHITE,
                       launchSettings.trajectorySteps == 150)) {
            launchSettings.trajectorySteps = 150;
        }
        if (DrawButton(Rectangle{barX + 152, y, 124, 26}, "Medium (250)", Color{28, 38, 54, 230}, RAYWHITE,
                       launchSettings.trajectorySteps == 250)) {
            launchSettings.trajectorySteps = 250;
        }
        if (DrawButton(Rectangle{barX + 286, y, 124, 26}, "Long (400)", Color{28, 38, 54, 230}, RAYWHITE,
                       launchSettings.trajectorySteps == 400)) {
            launchSettings.trajectorySteps = 400;
        }
    }
}

bool Renderer::isMouseOverUI(Vector2 mousePos, int screenWidth, int screenHeight) const {
    // 1. Diagnostics panel (Top-Left)
    if (CheckCollisionPointRec(mousePos, Rectangle{15, 15, 340, 265})) return true;

    // 2. Preset cheat-sheet (Bottom-Left)
    if (CheckCollisionPointRec(mousePos, Rectangle{15, static_cast<float>(screenHeight - 115), 460, 100})) return true;

    // 3. Right Sidebar
    if (sidebarVisible) {
        if (mousePos.x >= screenWidth - sidebarWidth) return true;
    } else {
        // Toggle pill when collapsed
        if (CheckCollisionPointRec(mousePos, Rectangle{static_cast<float>(screenWidth - 145), 18.0f, 132.0f, 32.0f})) return true;
    }

    return false;
}
