#include "Renderer.hpp"
#include <raymath.h>
#include <algorithm>
#include <cstdio>
#include <string>

Renderer::Renderer(int screenWidth, int screenHeight) {
    resetCamera(screenWidth, screenHeight);
    initFonts();
}

Renderer::~Renderer() {
    unloadFonts();
}

void Renderer::initFonts() {
    // Attempt to load standard high-DPI Windows TTF fonts
    if (FileExists("C:/Windows/Fonts/segoeui.ttf")) {
        uiFont = LoadFontEx("C:/Windows/Fonts/segoeui.ttf", 38, nullptr, 250);
        if (FileExists("C:/Windows/Fonts/segoeuib.ttf")) {
            boldFont = LoadFontEx("C:/Windows/Fonts/segoeuib.ttf", 42, nullptr, 250);
        } else {
            boldFont = uiFont;
        }
        SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
        SetTextureFilter(boldFont.texture, TEXTURE_FILTER_BILINEAR);
        fontLoaded = true;
    } else if (FileExists("C:/Windows/Fonts/arial.ttf")) {
        uiFont = LoadFontEx("C:/Windows/Fonts/arial.ttf", 38, nullptr, 250);
        boldFont = uiFont;
        SetTextureFilter(uiFont.texture, TEXTURE_FILTER_BILINEAR);
        fontLoaded = true;
    }
}

void Renderer::unloadFonts() {
    if (fontLoaded) {
        UnloadFont(uiFont);
        if (boldFont.texture.id != uiFont.texture.id) {
            UnloadFont(boldFont);
        }
        fontLoaded = false;
    }
}

void Renderer::drawText(const char* text, float x, float y, float size, Color col, bool bold) const {
    if (fontLoaded) {
        DrawTextEx(bold ? boldFont : uiFont, text, Vector2{x, y}, size, 0.5f, col);
    } else {
        DrawText(text, static_cast<int>(x), static_cast<int>(y), static_cast<int>(size), col);
    }
}

float Renderer::measureText(const char* text, float size, bool bold) const {
    if (fontLoaded) {
        return MeasureTextEx(bold ? boldFont : uiFont, text, size, 0.5f).x;
    } else {
        return static_cast<float>(MeasureText(text, static_cast<int>(size)));
    }
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

void Renderer::drawRoom(const GravityEngine& engine) const {
    if (!engine.roomBounds.enabled) return;

    const auto& rb = engine.roomBounds;
    float rx = static_cast<float>(rb.xMin);
    float ry = static_cast<float>(rb.yMin);
    float rw = static_cast<float>(rb.xMax - rb.xMin);
    float rh = static_cast<float>(rb.yMax - rb.yMin);
    float floorY = static_cast<float>(rb.yMax);
    float ceilY = static_cast<float>(rb.yMin);

    // 1. Lab Room Interior Chamber Background with soft tint
    DrawRectangleRec(Rectangle{rx, ry, rw, rh}, Color{12, 16, 26, 225});

    // 2. Room Grid Lines (Vertical & Horizontal every 50px)
    for (float x = rx + 50.0f; x < rb.xMax; x += 50.0f) {
        DrawLineV(Vector2{x, ry}, Vector2{x, floorY}, Color{30, 42, 65, 80});
    }
    for (float y = ry + 50.0f; y < floorY; y += 50.0f) {
        DrawLineV(Vector2{rx, y}, Vector2{static_cast<float>(rb.xMax), y}, Color{30, 42, 65, 80});
    }

    // 3. Critical Shatter Height Line (Red Dashed Line)
    // h_crit = v_shatter^2 / (2 * g)
    double gy = (std::abs(engine.uniformGravity.y) > 1e-3) ? std::abs(engine.uniformGravity.y) : 720.0;
    float critY = static_cast<float>(rb.yMax - (rb.shatterVelocity * rb.shatterVelocity) / (2.0 * gy));
    if (critY > ry && critY < floorY) {
        for (float x = rx + 8.0f; x < rb.xMax - 8.0f; x += 22.0f) {
            DrawLineEx(Vector2{x, critY}, Vector2{x + 13.0f, critY}, 2.0f, Color{255, 75, 75, 190});
        }
        drawText("--- CRITICAL SHATTER HEIGHT (Fall from above this line shatters on impact) ---",
                 rx + 25.0f, critY - 18.0f, 13.0f, Color{255, 95, 95, 230}, true);
    }

    // 4. Safe Bounce Zone Line (Green Dashed Line at 180px above floor)
    float safeY = floorY - 180.0f;
    if (safeY > ry && safeY < floorY) {
        for (float x = rx + 8.0f; x < rb.xMax - 8.0f; x += 22.0f) {
            DrawLineEx(Vector2{x, safeY}, Vector2{x + 13.0f, safeY}, 2.0f, Color{60, 225, 110, 160});
        }
        drawText("--- SAFE BOUNCE ZONE (Fall from below this line bounces harmlessly) ---",
                 rx + 25.0f, safeY - 18.0f, 13.0f, Color{80, 245, 130, 210}, true);
    }

    // 5. Heavy Reinforced Steel Floor Slab at yMax
    DrawRectangleRec(Rectangle{rx - 30.0f, floorY, rw + 60.0f, 36.0f}, Color{35, 42, 58, 255});
    DrawLineEx(Vector2{rx - 30.0f, floorY}, Vector2{rx + rw + 30.0f, floorY}, 3.5f, Color{0, 230, 255, 240});

    // Floor hazard warning stripes (yellow / dark gray)
    for (float hx = rx - 20.0f; hx < rb.xMax + 20.0f; hx += 32.0f) {
        DrawLineEx(Vector2{hx, floorY + 4.0f}, Vector2{hx + 16.0f, floorY + 32.0f}, 3.5f, Color{255, 205, 30, 190});
    }
    drawText("REINFORCED STEEL IMPACT FLOOR [Restitution: 82% | Earth g: 9.81 m/s²]",
             rx + 15.0f, floorY + 10.0f, 13.0f, Color{235, 245, 255, 240}, true);

    // 6. Ceiling Beam at yMin
    DrawRectangleRec(Rectangle{rx - 30.0f, ceilY - 28.0f, rw + 60.0f, 28.0f}, Color{28, 34, 48, 255});
    DrawLineEx(Vector2{rx - 30.0f, ceilY}, Vector2{rx + rw + 30.0f, ceilY}, 2.5f, Color{110, 160, 215, 230});

    // Overhead spotlights
    for (float lx = rx + 140.0f; lx < rb.xMax - 80.0f; lx += 220.0f) {
        DrawCircleV(Vector2{lx, ceilY}, 6.0f, Color{255, 240, 200, 255});
        DrawTriangle(Vector2{lx, ceilY},
                     Vector2{lx - 70.0f, ceilY + 150.0f},
                     Vector2{lx + 70.0f, ceilY + 150.0f},
                     Color{255, 230, 180, 14});
    }

    // 7. Left & Right Reinforced Chamber Walls with Height Ruler
    DrawRectangleRec(Rectangle{rx - 30.0f, ceilY, 30.0f, rh}, Color{26, 32, 46, 255});
    DrawLineEx(Vector2{rx, ceilY}, Vector2{rx, floorY}, 3.0f, Color{110, 150, 200, 230});

    DrawRectangleRec(Rectangle{static_cast<float>(rb.xMax), ceilY, 30.0f, rh}, Color{26, 32, 46, 255});
    DrawLineEx(Vector2{static_cast<float>(rb.xMax), ceilY}, Vector2{static_cast<float>(rb.xMax), floorY}, 3.0f, Color{110, 150, 200, 230});

    // Height ruler ticks on left wall: every 50px = 5 meters
    int meter = 0;
    for (float my = floorY; my >= ceilY; my -= 50.0f) {
        DrawLineEx(Vector2{rx - 14.0f, my}, Vector2{rx, my}, 2.0f, Color{160, 190, 225, 220});
        drawText(TextFormat("%dm", meter), rx - 42.0f, my - 6.0f, 11.0f, Color{190, 210, 235, 220});
        meter += 5;
    }

    // 8. Top Lab Room Title Banner
    drawText("EARTH GRAVITY EXPERIMENT LABORATORY", rx + 18.0f, ry + 16.0f, 18.0f, GOLD, true);
    drawText("Galileo Equivalence: a = g (All masses fall at exact same acceleration)",
             rx + 18.0f, ry + 42.0f, 13.0f, SKYBLUE, true);
    drawText("Constant Downward Field: g = 9.81 m/s² | High Fall Velocity > 680 px/s -> Particle Shatter",
             rx + 18.0f, ry + 62.0f, 12.0f, Color{210, 225, 245, 220});
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
            drawText(body.name.c_str(), pos.x + r + 6.0f, pos.y - 8.0f, 13.0f, Fade(RAYWHITE, 0.95f), true);
        }
    }
}

void Renderer::drawDestructionEffects(const GravityEngine& engine) const {
    // 1. Draw Shockwave Rings
    for (const auto& ring : engine.shockwaves) {
        Vector2 c = ring.center.toVector2();
        DrawCircleLines(static_cast<int>(c.x), static_cast<int>(c.y), ring.radius, Fade(ring.color, ring.alpha * 0.85f));
        DrawCircleLines(static_cast<int>(c.x), static_cast<int>(c.y), ring.radius + 1.8f, Fade(WHITE, ring.alpha * 0.55f));
    }

    // 2. Draw Destruction Particles & Spark Streaks
    for (const auto& p : engine.destructionParticles) {
        Vector2 pos = p.pos.toVector2();
        float alpha = p.lifetime / p.maxLifetime;

        // Glowing soft corona
        DrawCircleV(pos, p.size * 2.2f, Fade(p.color, alpha * 0.35f));
        // Incandescent core
        DrawCircleV(pos, p.size, Fade(p.color, alpha));

        // High-velocity trailing spark streak
        if (p.vel.lengthSq() > 25.0) {
            Vector2 tail = (p.pos - p.vel * 0.035).toVector2();
            DrawLineEx(pos, tail, std::max(1.2f, p.size * 0.7f), Fade(p.color, alpha * 0.8f));
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

bool Renderer::DrawButton(Rectangle rect, const char* text, Color bg, Color textCol, bool isActive, float fontSize) const {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, rect);

    Color drawBg = bg;
    Color borderCol = isActive ? GOLD : (hovered ? SKYBLUE : Fade(GRAY, 0.55f));

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

    float textW = measureText(text, fontSize, false);
    float textX = rect.x + (rect.width - textW) * 0.5f;
    float textY = rect.y + (rect.height - fontSize) * 0.5f - 1.0f;

    drawText(text, textX, textY, fontSize, hovered ? WHITE : textCol, isActive);

    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void Renderer::DrawCard(Rectangle rect, const char* title, Color titleCol) const {
    DrawRectangleRounded(rect, 0.05f, 6, Fade(Color{14, 20, 32, 255}, 0.88f));
    DrawRectangleRoundedLines(rect, 0.05f, 6, Fade(Color{55, 75, 110, 255}, 0.55f));

    if (title != nullptr) {
        drawText(title, rect.x + 16.0f, rect.y + 10.0f, 14.0f, titleCol, true);
        DrawLine(static_cast<int>(rect.x + 14), static_cast<int>(rect.y + 32),
                 static_cast<int>(rect.x + rect.width - 14), static_cast<int>(rect.y + 32),
                 Fade(DARKGRAY, 0.45f));
    }
}

void Renderer::drawHUD(const GravityEngine& engine, float timeScale, bool isPaused,
                      const LaunchSettings& launchSettings, int /*screenWidth*/, int screenHeight) const {
    // 1. Top-Left Diagnostics Panel (Crisp TTF Text)
    int panelW = 360;
    int panelH = 280;
    DrawRectangleRounded(Rectangle{15, 15, static_cast<float>(panelW), static_cast<float>(panelH)}, 0.08f, 6, Fade(BLACK, 0.86f));
    DrawRectangleRoundedLines(Rectangle{15, 15, static_cast<float>(panelW), static_cast<float>(panelH)}, 0.08f, 6, Fade(SKYBLUE, 0.65f));

    if (engine.roomBounds.enabled) {
        drawText("EARTH GRAVITY LABORATORY", 28.0f, 24.0f, 16.0f, GOLD, true);
    } else {
        drawText("2D N-BODY GRAVITY ENGINE", 28.0f, 24.0f, 17.0f, SKYBLUE, true);
    }
    DrawLine(28, 48, 28 + panelW - 26, 48, Fade(DARKGRAY, 0.5f));

    float y = 58.0f;
    float lineSpacing = 20.0f;

    drawText(TextFormat("FPS: %d", GetFPS()), 28.0f, y, 14.0f, GREEN);
    drawText(TextFormat("Bodies: %zu", engine.getBodyCount()), 165.0f, y, 14.0f, RAYWHITE);
    y += lineSpacing;

    drawText("Integrator [I]:", 28.0f, y, 14.0f, LIGHTGRAY);
    drawText(engine.getIntegratorName(), 165.0f, y, 14.0f,
             (engine.integrator == IntegratorType::VelocityVerlet) ? GOLD : ORANGE, true);
    y += lineSpacing;

    drawText("Collision [C]:", 28.0f, y, 14.0f, LIGHTGRAY);
    Color colModeColor = (engine.collisionMode == CollisionMode::DestructiveShatter) ? RED :
                         (engine.collisionMode == CollisionMode::InelasticMerge) ? VIOLET :
                         (engine.collisionMode == CollisionMode::Elastic) ? LIME : GRAY;
    drawText(engine.getCollisionModeName(), 165.0f, y, 14.0f, colModeColor, true);
    y += lineSpacing;

    drawText("Status [SPACE]:", 28.0f, y, 14.0f, LIGHTGRAY);
    drawText(isPaused ? "PAUSED" : "RUNNING", 165.0f, y, 14.0f, isPaused ? RED : LIME, true);
    y += lineSpacing;

    drawText("Time Scale [</>]:", 28.0f, y, 14.0f, LIGHTGRAY);
    drawText(TextFormat("%.1fx (substeps: %d)", timeScale, static_cast<int>(timeScale * 5)), 165.0f, y, 14.0f, RAYWHITE);
    y += lineSpacing;

    double kinetic = engine.computeKineticEnergy();
    double potential = engine.computePotentialEnergy();
    double total = kinetic + potential;
    double drift = engine.getEnergyDriftPercent();

    drawText("Total Energy E:", 28.0f, y, 14.0f, LIGHTGRAY);
    drawText(TextFormat("%+11.2e", total), 165.0f, y, 14.0f, RAYWHITE);
    y += lineSpacing;

    drawText("Energy Drift:", 28.0f, y, 14.0f, LIGHTGRAY);
    Color driftColor = (drift < 0.01) ? LIME : ((drift < 0.1) ? YELLOW : ORANGE);
    drawText(TextFormat("%.5f%%", drift), 165.0f, y, 14.0f, driftColor, true);
    y += lineSpacing;

    drawText("Payload [TAB]:", 28.0f, y, 14.0f, LIGHTGRAY);
    drawText(TextFormat("%s (M=%.1f)", launchSettings.typeName.c_str(), launchSettings.mass), 165.0f, y, 14.0f, YELLOW);
    y += lineSpacing;

    drawText("Camera Zoom:", 28.0f, y, 14.0f, LIGHTGRAY);
    drawText(TextFormat("%.2fx %s", camera.zoom, (followedBodyId != -1 ? "[LOCKED]" : "")), 165.0f, y, 14.0f, RAYWHITE);

    // 2. Bottom-Left Preset & Controls Cheat-sheet
    int cheatW = 540;
    int cheatH = 105;
    float cheatY = static_cast<float>(screenHeight - cheatH - 15);
    DrawRectangleRounded(Rectangle{15, cheatY, static_cast<float>(cheatW), static_cast<float>(cheatH)}, 0.08f, 6, Fade(BLACK, 0.86f));
    DrawRectangleRoundedLines(Rectangle{15, cheatY, static_cast<float>(cheatW), static_cast<float>(cheatH)}, 0.08f, 6, Fade(GRAY, 0.4f));

    drawText("PRESETS: [1] Solar  [2] Figure-8  [3] Binary Disk  [4] Earth Lab Room", 26.0f, cheatY + 14.0f, 13.0f, GOLD, true);
    drawText("CONTROLS:", 26.0f, cheatY + 36.0f, 13.0f, SKYBLUE, true);
    drawText("Click Body: Inspect in Sidebar | Drag: Slingshot | [H] Toggle Sidebar", 26.0f, cheatY + 56.0f, 13.0f, RAYWHITE);
    drawText("[TAB] Spawner Tab | [C] Collision Mode | [T] Trails | [V] Vectors | [X] Clear", 26.0f, cheatY + 76.0f, 13.0f, RAYWHITE);
}

void Renderer::drawSidebarTogglePill(int screenWidth, int /*screenHeight*/) {
    Rectangle pillRect{static_cast<float>(screenWidth - 175), 18.0f, 160.0f, 38.0f};
    if (DrawButton(pillRect, "<< SHOW SIDEBAR [H]", Color{18, 28, 48, 245}, GOLD, false, 13.0f)) {
        sidebarVisible = true;
    }
}

void Renderer::drawSidebar(Body* selectedBody, GravityEngine& engine, LaunchSettings& launchSettings,
                           int screenWidth, int screenHeight, bool& shouldDeselect, bool& shouldDelete) {
    if (!sidebarVisible) {
        drawSidebarTogglePill(screenWidth, screenHeight);
        return;
    }

    // Dynamic width adapting to wide displays
    sidebarWidth = std::clamp(screenWidth * 0.32f, 480.0f, 560.0f);

    float barX = static_cast<float>(screenWidth) - sidebarWidth;
    float barW = sidebarWidth;
    float barH = static_cast<float>(screenHeight);
    float xStart = barX + 20.0f;
    float contentW = barW - 40.0f;

    // Full-height dark translucent background with clean golden vertical border
    DrawRectangle(static_cast<int>(barX), 0, static_cast<int>(barW), static_cast<int>(barH), Color{9, 13, 22, 248});
    DrawLineEx(Vector2{barX, 0.0f}, Vector2{barX, barH}, 2.5f, Fade(GOLD, 0.75f));

    // Top Header & Tab Controls
    float y = 16.0f;

    float tabW = (contentW - 125.0f - 16.0f) * 0.5f;
    Rectangle hideBtnRect{xStart, y, 115.0f, 36.0f};
    if (DrawButton(hideBtnRect, "HIDE >> [H]", Color{26, 36, 52, 230}, LIGHTGRAY, false, 13.0f)) {
        sidebarVisible = false;
        return;
    }

    Rectangle tabInspRect{xStart + 125.0f, y, tabW, 36.0f};
    bool isInspActive = (activeTab == SidebarTab::Inspector);
    if (DrawButton(tabInspRect, "OBJECT INSPECTOR", isInspActive ? Color{42, 60, 92, 240} : Color{18, 24, 36, 200},
                   isInspActive ? GOLD : GRAY, isInspActive, 13.0f)) {
        activeTab = SidebarTab::Inspector;
    }

    Rectangle tabSpawnRect{xStart + 125.0f + tabW + 8.0f, y, tabW, 36.0f};
    bool isSpawnActive = (activeTab == SidebarTab::Spawner);
    if (DrawButton(tabSpawnRect, "SPAWNER SETTINGS", isSpawnActive ? Color{42, 60, 92, 240} : Color{18, 24, 36, 200},
                   isSpawnActive ? SKYBLUE : GRAY, isSpawnActive, 13.0f)) {
        activeTab = SidebarTab::Spawner;
    }

    y += 48.0f;
    DrawLineEx(Vector2{xStart, y}, Vector2{xStart + contentW, y}, 1.0f, Fade(DARKGRAY, 0.5f));
    y += 16.0f;

    // Dynamic vertical gap based on screen height
    float gap = std::clamp((static_cast<float>(screenHeight) - 860.0f) * 0.15f + 16.0f, 14.0f, 28.0f);

    // --- TAB 1: OBJECT INSPECTOR ---
    if (activeTab == SidebarTab::Inspector) {
        if (!selectedBody) {
            // Scene Presets Selection Card
            float sceneCardH = 110.0f;
            DrawCard(Rectangle{xStart, y, contentW, sceneCardH}, "SCENE PRESETS", GOLD);
            float scy = y + 42.0f;
            float scBtnW = (contentW - 36.0f - 24.0f) / 4.0f;

            if (DrawButton(Rectangle{xStart + 18.0f, scy, scBtnW, 44.0f}, "[1] Solar",
                           Color{26, 38, 56, 230}, !engine.roomBounds.enabled ? GOLD : RAYWHITE, false, 12.0f)) {
                engine.loadSolarSystem();
                resetCamera(screenWidth, screenHeight);
            }
            if (DrawButton(Rectangle{xStart + 18.0f + (scBtnW + 8.0f), scy, scBtnW, 44.0f}, "[2] Fig-8",
                           Color{26, 38, 56, 230}, RAYWHITE, false, 12.0f)) {
                engine.loadFigureEight();
                resetCamera(screenWidth, screenHeight);
            }
            if (DrawButton(Rectangle{xStart + 18.0f + 2 * (scBtnW + 8.0f), scy, scBtnW, 44.0f}, "[3] Binary",
                           Color{26, 38, 56, 230}, RAYWHITE, false, 12.0f)) {
                engine.loadBinaryDisk();
                resetCamera(screenWidth, screenHeight);
            }
            if (DrawButton(Rectangle{xStart + 18.0f + 3 * (scBtnW + 8.0f), scy, scBtnW, 44.0f}, "[4] Earth Lab",
                           Color{26, 48, 70, 230}, engine.roomBounds.enabled ? LIME : SKYBLUE, engine.roomBounds.enabled, 11.5f)) {
                engine.loadEarthRoom();
                resetCamera(screenWidth, screenHeight);
                camera.target = Vector2{0.0f, -20.0f};
                camera.zoom = 1.05f;
            }

            y += sceneCardH + gap;

            if (engine.roomBounds.enabled) {
                // Earth Gravity Lab Info & Actions Card
                float labCardH = 290.0f;
                DrawCard(Rectangle{xStart, y, contentW, labCardH}, "EARTH GRAVITY LABORATORY", LIME);
                float ly = y + 42.0f;

                drawText("Downward Acceleration field:", xStart + 20.0f, ly, 14.0f, LIGHTGRAY);
                drawText("a = g = 9.81 m/s2 (Mass-Independent)", xStart + 20.0f, ly + 20.0f, 15.0f, GOLD, true);
                ly += 46.0f;

                drawText("Critical Shatter Height:", xStart + 20.0f, ly, 14.0f, LIGHTGRAY);
                drawText("Fall > 320px -> Explodes on floor!", xStart + 20.0f, ly + 20.0f, 14.0f, RED, true);
                ly += 46.0f;

                drawText("Floor Bounce Restitution:", xStart + 20.0f, ly, 14.0f, LIGHTGRAY);
                drawText("82% Elastic Rebound (Safe below 320px)", xStart + 20.0f, ly + 20.0f, 14.0f, SKYBLUE);
                ly += 48.0f;

                float labActW = (contentW - 36.0f - 12.0f) * 0.5f;
                if (DrawButton(Rectangle{xStart + 18.0f, ly, labActW, 40.0f}, "Reset Galileo Pair", Color{30, 52, 30, 240}, LIME, false, 12.0f)) {
                    engine.loadEarthRoom();
                }
                if (DrawButton(Rectangle{xStart + 18.0f + labActW + 12.0f, ly, labActW, 40.0f}, "Clear Room", Color{60, 25, 25, 240}, RED, false, 12.0f)) {
                    engine.clearBodies();
                }

                y += labCardH + gap;
            } else {
                DrawCard(Rectangle{xStart, y, contentW, 260.0f}, "HOW TO USE THE INSPECTOR", SKYBLUE);
                float ey = y + 48.0f;
                drawText("1. Click any planet, star, or debris in space to select it.", xStart + 22.0f, ey, 15.0f, RAYWHITE);
                ey += 32.0f;
                drawText("2. View surface gravity pull, escape velocity & real-time kinematics.", xStart + 22.0f, ey, 15.0f, RAYWHITE);
                ey += 32.0f;
                drawText("3. Adjust mass, radius, and density with tactile steppers.", xStart + 22.0f, ey, 15.0f, RAYWHITE);
                ey += 32.0f;
                drawText("4. Lock camera tracking, freeze velocity, or circularize orbit.", xStart + 22.0f, ey, 15.0f, RAYWHITE);
                ey += 32.0f;
                drawText("5. Trigger catastrophic destruction physics with particle explosion.", xStart + 22.0f, ey, 15.0f, RAYWHITE);

                y += 280.0f;
            }

            Rectangle switchTabBtn{xStart + 50.0f, y, contentW - 100.0f, 44.0f};
            if (DrawButton(switchTabBtn, "OPEN SPAWNER SETTINGS [TAB]", Color{26, 48, 80, 230}, SKYBLUE, false, 14.0f)) {
                activeTab = SidebarTab::Spawner;
            }
            return;
        }

        // Section 1: Body Identity Card
        float card1H = 80.0f;
        DrawCard(Rectangle{xStart, y, contentW, card1H});

        Vector2 thumbPos{xStart + 40.0f, y + card1H * 0.5f};
        DrawCircleV(thumbPos, 22.0f, Fade(selectedBody->color, 0.35f));
        DrawCircleV(thumbPos, 16.0f, selectedBody->color);
        DrawCircleLines(static_cast<int>(thumbPos.x), static_cast<int>(thumbPos.y), 16, WHITE);

        drawText(selectedBody->name.c_str(), xStart + 76.0f, y + 16.0f, 22.0f, RAYWHITE, true);
        drawText(selectedBody->isStatic ? "[STATIC ANCHOR / FIXED]" : "[DYNAMIC IN KEPLERIAN ORBIT]",
                 xStart + 76.0f, y + 45.0f, 13.0f, selectedBody->isStatic ? GOLD : SKYBLUE, true);

        Rectangle deselectBtn{xStart + contentW - 125.0f, y + 22.0f, 115.0f, 36.0f};
        if (DrawButton(deselectBtn, "DESELECT [X]", Color{55, 25, 25, 220}, RED, false, 13.0f)) {
            shouldDeselect = true;
        }
        y += card1H + gap;

        // Section 2: Physical Mass & Dimensions Card
        float card2H = 185.0f;
        DrawCard(Rectangle{xStart, y, contentW, card2H}, "PHYSICAL MASS & DIMENSIONS", GOLD);
        float py = y + 42.0f;

        drawText("Mass:", xStart + 18.0f, py + 2.0f, 15.0f, LIGHTGRAY);
        drawText(TextFormat("%.3e kg", selectedBody->mass), xStart + 120.0f, py, 17.0f, YELLOW, true);
        py += 28.0f;

        float btnW4 = (contentW - 36.0f - 18.0f) / 4.0f;
        if (DrawButton(Rectangle{xStart + 18.0f, py, btnW4, 28.0f}, "-50%", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            selectedBody->setMass(selectedBody->mass * 0.5);
        }
        if (DrawButton(Rectangle{xStart + 18.0f + (btnW4 + 6.0f), py, btnW4, 28.0f}, "-10%", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            selectedBody->setMass(selectedBody->mass * 0.9);
        }
        if (DrawButton(Rectangle{xStart + 18.0f + 2 * (btnW4 + 6.0f), py, btnW4, 28.0f}, "+10%", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            selectedBody->setMass(selectedBody->mass * 1.1);
        }
        if (DrawButton(Rectangle{xStart + 18.0f + 3 * (btnW4 + 6.0f), py, btnW4, 28.0f}, "+50%", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            selectedBody->setMass(selectedBody->mass * 1.5);
        }
        py += 34.0f;

        if (DrawButton(Rectangle{xStart + 18.0f, py, btnW4, 28.0f}, "/10", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            selectedBody->setMass(selectedBody->mass * 0.1);
        }
        if (DrawButton(Rectangle{xStart + 18.0f + (btnW4 + 6.0f), py, btnW4, 28.0f}, "/2", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            selectedBody->setMass(selectedBody->mass * 0.5);
        }
        if (DrawButton(Rectangle{xStart + 18.0f + 2 * (btnW4 + 6.0f), py, btnW4, 28.0f}, "x2", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            selectedBody->setMass(selectedBody->mass * 2.0);
        }
        if (DrawButton(Rectangle{xStart + 18.0f + 3 * (btnW4 + 6.0f), py, btnW4, 28.0f}, "x10", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            selectedBody->setMass(selectedBody->mass * 10.0);
        }
        py += 36.0f;

        // Radius & Density
        drawText("Radius:", xStart + 18.0f, py + 2.0f, 15.0f, LIGHTGRAY);
        drawText(TextFormat("%.1f px", selectedBody->radius), xStart + 90.0f, py + 2.0f, 16.0f, RAYWHITE, true);

        if (DrawButton(Rectangle{xStart + 175.0f, py, 68.0f, 26.0f}, "-1 px", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            selectedBody->setRadius(selectedBody->radius - 1.0);
        }
        if (DrawButton(Rectangle{xStart + 248.0f, py, 68.0f, 26.0f}, "+1 px", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            selectedBody->setRadius(selectedBody->radius + 1.0);
        }

        drawText("Density:", xStart + 330.0f, py + 2.0f, 15.0f, LIGHTGRAY);
        drawText(TextFormat("%.3f", selectedBody->getDensity()), xStart + 400.0f, py + 2.0f, 16.0f, GREEN, true);

        y += card2H + gap;

        // Section 3: Gravitational Field & Kinematics Card
        float card3H = 220.0f;
        DrawCard(Rectangle{xStart, y, contentW, card3H}, "GRAVITATIONAL FIELD & KINEMATICS", SKYBLUE);
        float gy = y + 42.0f;
        float rowSpacing = 24.0f;

        double surfG = selectedBody->getSurfaceGravity(engine.G, engine.epsilon);
        double escV = selectedBody->getEscapeVelocity(engine.G);
        double speed = selectedBody->vel.length();
        double accPull = selectedBody->acc.length();
        double mu = engine.G * selectedBody->mass;

        drawText("Surface Gravity (g):", xStart + 18.0f, gy, 15.0f, LIGHTGRAY);
        drawText(TextFormat("%.4e m/s²", surfG), xStart + 245.0f, gy, 15.0f, GOLD, true);
        gy += rowSpacing;

        drawText("Escape Velocity (V_esc):", xStart + 18.0f, gy, 15.0f, LIGHTGRAY);
        drawText(TextFormat("%.2f m/s", escV), xStart + 245.0f, gy, 15.0f, GOLD, true);
        gy += rowSpacing;

        drawText("Gravitational Param (μ):", xStart + 18.0f, gy, 15.0f, LIGHTGRAY);
        drawText(TextFormat("%.3e", mu), xStart + 245.0f, gy, 15.0f, RAYWHITE);
        gy += rowSpacing;

        drawText("Current Velocity (|v|):", xStart + 18.0f, gy, 15.0f, LIGHTGRAY);
        drawText(TextFormat("%.2f m/s", speed), xStart + 245.0f, gy, 15.0f, LIME, true);
        gy += rowSpacing;

        drawText("Net Gravity Force (|a|):", xStart + 18.0f, gy, 15.0f, LIGHTGRAY);
        drawText(TextFormat("%.4e m/s²", accPull), xStart + 245.0f, gy, 15.0f, ORANGE, true);
        gy += rowSpacing;

        drawText("World Position (X, Y):", xStart + 18.0f, gy, 15.0f, LIGHTGRAY);
        drawText(TextFormat("(%.1f,  %.1f)", selectedBody->pos.x, selectedBody->pos.y), xStart + 245.0f, gy, 15.0f, RAYWHITE);
        gy += rowSpacing;

        drawText("Velocity Vector (Vx, Vy):", xStart + 18.0f, gy, 15.0f, LIGHTGRAY);
        drawText(TextFormat("(%.2f,  %.2f)", selectedBody->vel.x, selectedBody->vel.y), xStart + 245.0f, gy, 15.0f, RAYWHITE);

        y += card3H + gap;

        // Section 4: Orbit & Motion Commands Card
        float card4H = 145.0f;
        DrawCard(Rectangle{xStart, y, contentW, card4H}, "ORBIT & MOTION COMMANDS", GOLD);
        float oy = y + 42.0f;
        float btnW2 = (contentW - 36.0f - 12.0f) * 0.5f;

        bool isCamLocked = (followedBodyId == selectedBody->id);
        if (DrawButton(Rectangle{xStart + 18.0f, oy, btnW2, 38.0f},
                       isCamLocked ? "UNLOCK CAMERA" : "LOCK CAMERA TRACK",
                       Color{28, 46, 75, 230}, isCamLocked ? GOLD : RAYWHITE, isCamLocked, 13.0f)) {
            followedBodyId = isCamLocked ? -1 : selectedBody->id;
        }

        if (DrawButton(Rectangle{xStart + 18.0f + btnW2 + 12.0f, oy, btnW2, 38.0f},
                       selectedBody->isStatic ? "MAKE DYNAMIC" : "MAKE STATIC ANCHOR",
                       Color{28, 46, 75, 230}, selectedBody->isStatic ? GOLD : RAYWHITE, selectedBody->isStatic, 13.0f)) {
            selectedBody->isStatic = !selectedBody->isStatic;
            if (selectedBody->isStatic) selectedBody->vel = Vec2{0.0, 0.0};
        }
        oy += 46.0f;

        if (DrawButton(Rectangle{xStart + 18.0f, oy, btnW2, 38.0f}, "ZERO VELOCITY (HALT)", Color{46, 36, 62, 230}, RAYWHITE, false, 13.0f)) {
            selectedBody->vel = Vec2{0.0, 0.0};
        }

        if (DrawButton(Rectangle{xStart + 18.0f + btnW2 + 12.0f, oy, btnW2, 38.0f}, "AUTO CIRCULARIZE", Color{20, 58, 44, 230}, LIME, false, 13.0f)) {
            engine.circularizeOrbit(selectedBody->id);
        }

        y += card4H + gap;

        // Section 5: Destruction & Lifecycle Card (Utilizing bottom of sidebar)
        float card5H = 135.0f;
        DrawCard(Rectangle{xStart, y, contentW, card5H}, "DESTRUCTION & APPEARANCE", RED);
        float ly = y + 40.0f;

        const Color swatches[] = {
            Color{255, 215, 30, 255},  // Gold
            Color{70, 160, 255, 255},  // Cyan
            Color{240, 80, 60, 255},   // Mars Red
            Color{60, 230, 120, 255},  // Emerald
            Color{235, 175, 110, 255}, // Amber
            Color{220, 60, 240, 255},  // Magenta
            Color{120, 245, 255, 255}, // Aqua
            Color{240, 240, 245, 255}  // White
        };

        float swW = (contentW - 36.0f - 7 * 8.0f) / 8.0f;
        for (size_t i = 0; i < 8; ++i) {
            Rectangle sRect{xStart + 18.0f + i * (swW + 8.0f), ly, swW, 28.0f};
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
        ly += 38.0f;

        if (DrawButton(Rectangle{xStart + 18.0f, ly, btnW2, 38.0f}, "CLEAR ORBIT TRAIL", Color{36, 44, 58, 230}, RAYWHITE, false, 13.0f)) {
            selectedBody->clearTrail();
        }

        // Cataclysmic Destruction button
        if (DrawButton(Rectangle{xStart + 18.0f + btnW2 + 12.0f, ly, btnW2, 38.0f}, "DESTROY OBJECT (EXPLODE)", Color{95, 20, 20, 240}, RED, false, 13.0f)) {
            engine.triggerDestructionExplosion(selectedBody->id);
            shouldDelete = true;
        }
    }
    // --- TAB 2: SPAWNER SETTINGS (Spacious & Full Height) ---
    else if (activeTab == SidebarTab::Spawner) {
        // Section 0: Scene Presets Card
        float sceneCardH = 95.0f;
        DrawCard(Rectangle{xStart, y, contentW, sceneCardH}, "SCENE PRESETS", GOLD);
        float scy = y + 38.0f;
        float scBtnW = (contentW - 36.0f - 24.0f) / 4.0f;

        if (DrawButton(Rectangle{xStart + 18.0f, scy, scBtnW, 42.0f}, "[1] Solar",
                       Color{26, 38, 56, 230}, !engine.roomBounds.enabled ? GOLD : RAYWHITE, false, 12.0f)) {
            engine.loadSolarSystem();
            resetCamera(screenWidth, screenHeight);
        }
        if (DrawButton(Rectangle{xStart + 18.0f + (scBtnW + 8.0f), scy, scBtnW, 42.0f}, "[2] Fig-8",
                       Color{26, 38, 56, 230}, RAYWHITE, false, 12.0f)) {
            engine.loadFigureEight();
            resetCamera(screenWidth, screenHeight);
        }
        if (DrawButton(Rectangle{xStart + 18.0f + 2 * (scBtnW + 8.0f), scy, scBtnW, 42.0f}, "[3] Binary",
                       Color{26, 38, 56, 230}, RAYWHITE, false, 12.0f)) {
            engine.loadBinaryDisk();
            resetCamera(screenWidth, screenHeight);
        }
        if (DrawButton(Rectangle{xStart + 18.0f + 3 * (scBtnW + 8.0f), scy, scBtnW, 42.0f}, "[4] Earth Lab",
                       Color{26, 48, 70, 230}, engine.roomBounds.enabled ? LIME : SKYBLUE, engine.roomBounds.enabled, 11.5f)) {
            engine.loadEarthRoom();
            resetCamera(screenWidth, screenHeight);
            camera.target = Vector2{0.0f, -20.0f};
            camera.zoom = 1.05f;
        }

        y += sceneCardH + gap;

        // Section 1: Payload Presets Card
        float card1H = 160.0f;
        DrawCard(Rectangle{xStart, y, contentW, card1H},
                 engine.roomBounds.enabled ? "LAB EXPERIMENT PAYLOADS" : "CELESTIAL PAYLOAD PRESETS", GOLD);
        float py = y + 42.0f;

        float pBtnW = (contentW - 36.0f - 16.0f) / 3.0f;
        if (engine.roomBounds.enabled) {
            if (DrawButton(Rectangle{xStart + 18.0f, py, pBtnW, 42.0f}, "Heavy Anvil", Color{26, 38, 56, 230}, RAYWHITE,
                           launchSettings.presetType == LaunchPresetType::HeavyAnvil, 13.0f)) {
                launchSettings.applyPreset(LaunchPresetType::HeavyAnvil);
            }
            if (DrawButton(Rectangle{xStart + 18.0f + (pBtnW + 8.0f), py, pBtnW, 42.0f}, "Light Apple", Color{26, 38, 56, 230}, RAYWHITE,
                           launchSettings.presetType == LaunchPresetType::LightApple, 13.0f)) {
                launchSettings.applyPreset(LaunchPresetType::LightApple);
            }
            if (DrawButton(Rectangle{xStart + 18.0f + 2 * (pBtnW + 8.0f), py, pBtnW, 42.0f}, "Rubber Ball", Color{26, 38, 56, 230}, RAYWHITE,
                           launchSettings.presetType == LaunchPresetType::RubberBall, 13.0f)) {
                launchSettings.applyPreset(LaunchPresetType::RubberBall);
            }
            py += 50.0f;

            if (DrawButton(Rectangle{xStart + 18.0f, py, pBtnW, 42.0f}, "Asteroid", Color{26, 38, 56, 230}, RAYWHITE,
                           launchSettings.presetType == LaunchPresetType::Asteroid, 13.0f)) {
                launchSettings.applyPreset(LaunchPresetType::Asteroid);
            }
            if (DrawButton(Rectangle{xStart + 18.0f + (pBtnW + 8.0f), py, pBtnW, 42.0f}, "Planet", Color{26, 38, 56, 230}, RAYWHITE,
                           launchSettings.presetType == LaunchPresetType::Terrestrial, 13.0f)) {
                launchSettings.applyPreset(LaunchPresetType::Terrestrial);
            }
            if (DrawButton(Rectangle{xStart + 18.0f + 2 * (pBtnW + 8.0f), py, pBtnW, 42.0f}, "Star", Color{26, 38, 56, 230}, RAYWHITE,
                           launchSettings.presetType == LaunchPresetType::Star, 13.0f)) {
                launchSettings.applyPreset(LaunchPresetType::Star);
            }
        } else {
            if (DrawButton(Rectangle{xStart + 18.0f, py, pBtnW, 42.0f}, "Asteroid", Color{26, 38, 56, 230}, RAYWHITE,
                           launchSettings.presetType == LaunchPresetType::Asteroid, 14.0f)) {
                launchSettings.applyPreset(LaunchPresetType::Asteroid);
            }
            if (DrawButton(Rectangle{xStart + 18.0f + (pBtnW + 8.0f), py, pBtnW, 42.0f}, "Moon", Color{26, 38, 56, 230}, RAYWHITE,
                           launchSettings.presetType == LaunchPresetType::Moon, 14.0f)) {
                launchSettings.applyPreset(LaunchPresetType::Moon);
            }
            if (DrawButton(Rectangle{xStart + 18.0f + 2 * (pBtnW + 8.0f), py, pBtnW, 42.0f}, "Planet", Color{26, 38, 56, 230}, RAYWHITE,
                           launchSettings.presetType == LaunchPresetType::Terrestrial, 14.0f)) {
                launchSettings.applyPreset(LaunchPresetType::Terrestrial);
            }
            py += 50.0f;

            if (DrawButton(Rectangle{xStart + 18.0f, py, pBtnW, 42.0f}, "Gas Giant", Color{26, 38, 56, 230}, RAYWHITE,
                           launchSettings.presetType == LaunchPresetType::GasGiant, 14.0f)) {
                launchSettings.applyPreset(LaunchPresetType::GasGiant);
            }
            if (DrawButton(Rectangle{xStart + 18.0f + (pBtnW + 8.0f), py, pBtnW, 42.0f}, "Star", Color{26, 38, 56, 230}, RAYWHITE,
                           launchSettings.presetType == LaunchPresetType::Star, 14.0f)) {
                launchSettings.applyPreset(LaunchPresetType::Star);
            }
            if (DrawButton(Rectangle{xStart + 18.0f + 2 * (pBtnW + 8.0f), py, pBtnW, 42.0f}, "Singularity", Color{26, 38, 56, 230}, RAYWHITE,
                           launchSettings.presetType == LaunchPresetType::BlackHole, 14.0f)) {
                launchSettings.applyPreset(LaunchPresetType::BlackHole);
            }
        }

        y += card1H + gap;

        // Section 2: Spawn Injection Mode Card
        float card2H = 110.0f;
        DrawCard(Rectangle{xStart, y, contentW, card2H}, "SPAWN INJECTION MODE", GOLD);
        float my = y + 42.0f;

        if (DrawButton(Rectangle{xStart + 18.0f, my, pBtnW, 44.0f}, "Slingshot (Drag)", Color{26, 44, 70, 230}, RAYWHITE,
                       launchSettings.spawnMode == LaunchSpawnMode::Slingshot, 13.0f)) {
            launchSettings.spawnMode = LaunchSpawnMode::Slingshot;
        }
        const char* midModeName = engine.roomBounds.enabled ? "Drop / Free Fall" : "Auto Orbit (Click)";
        if (DrawButton(Rectangle{xStart + 18.0f + (pBtnW + 8.0f), my, pBtnW, 44.0f}, midModeName, Color{26, 44, 70, 230}, RAYWHITE,
                       launchSettings.spawnMode == LaunchSpawnMode::CircularOrbit, 12.0f)) {
            launchSettings.spawnMode = LaunchSpawnMode::CircularOrbit;
        }
        if (DrawButton(Rectangle{xStart + 18.0f + 2 * (pBtnW + 8.0f), my, pBtnW, 44.0f}, "Stationary (Click)", Color{26, 44, 70, 230}, RAYWHITE,
                       launchSettings.spawnMode == LaunchSpawnMode::Stationary, 13.0f)) {
            launchSettings.spawnMode = LaunchSpawnMode::Stationary;
        }

        y += card2H + gap;

        // Section 3: Custom Properties Card
        float card3H = 220.0f;
        DrawCard(Rectangle{xStart, y, contentW, card3H}, "CUSTOM PHYSICAL PROPERTIES", SKYBLUE);
        float cy = y + 42.0f;

        drawText("Launch Mass:", xStart + 18.0f, cy + 2.0f, 15.0f, LIGHTGRAY);
        drawText(TextFormat("%.3e kg", launchSettings.mass), xStart + 145.0f, cy, 17.0f, YELLOW, true);
        cy += 28.0f;

        float btnW4 = (contentW - 36.0f - 18.0f) / 4.0f;
        if (DrawButton(Rectangle{xStart + 18.0f, cy, btnW4, 30.0f}, "-50%", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            launchSettings.mass = std::max(launchSettings.mass * 0.5, 0.01);
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{xStart + 18.0f + (btnW4 + 6.0f), cy, btnW4, 30.0f}, "-10%", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            launchSettings.mass = std::max(launchSettings.mass * 0.9, 0.01);
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{xStart + 18.0f + 2 * (btnW4 + 6.0f), cy, btnW4, 30.0f}, "+10%", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            launchSettings.mass *= 1.1;
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{xStart + 18.0f + 3 * (btnW4 + 6.0f), cy, btnW4, 30.0f}, "+50%", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            launchSettings.mass *= 1.5;
            launchSettings.recalculateRadiusFromDensity();
        }
        cy += 40.0f;

        drawText("Launch Radius:", xStart + 18.0f, cy + 2.0f, 15.0f, LIGHTGRAY);
        drawText(TextFormat("%.1f px", launchSettings.radius), xStart + 145.0f, cy + 2.0f, 16.0f, RAYWHITE, true);

        if (DrawButton(Rectangle{xStart + 260.0f, cy, 85.0f, 28.0f}, "-1 px", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            launchSettings.radius = std::max(launchSettings.radius - 1.0, 1.5);
            launchSettings.recalculateDensityFromRadius();
        }
        if (DrawButton(Rectangle{xStart + 355.0f, cy, 85.0f, 28.0f}, "+1 px", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            launchSettings.radius += 1.0;
            launchSettings.recalculateDensityFromRadius();
        }
        cy += 38.0f;

        drawText("Launch Density:", xStart + 18.0f, cy + 2.0f, 15.0f, LIGHTGRAY);
        drawText(TextFormat("%.4f kg/px²", launchSettings.density), xStart + 145.0f, cy + 2.0f, 16.0f, GREEN, true);

        if (DrawButton(Rectangle{xStart + 260.0f, cy, 85.0f, 28.0f}, "-20%", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            launchSettings.density = std::max(launchSettings.density * 0.8, 0.001);
            launchSettings.recalculateRadiusFromDensity();
        }
        if (DrawButton(Rectangle{xStart + 355.0f, cy, 85.0f, 28.0f}, "+20%", Color{26, 38, 56, 220}, RAYWHITE, false, 12.0f)) {
            launchSettings.density *= 1.2;
            launchSettings.recalculateRadiusFromDensity();
        }

        y += card3H + gap;

        // Section 4: Color, Anchor & Trajectory Card (Fills lower space)
        float card4H = 195.0f;
        DrawCard(Rectangle{xStart, y, contentW, card4H}, "COLOR & INJECTION CONTROLS", VIOLET);
        float vy = y + 42.0f;

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

        float swW = (contentW - 36.0f - 7 * 8.0f) / 8.0f;
        for (size_t i = 0; i < 8; ++i) {
            Rectangle sRect{xStart + 18.0f + i * (swW + 8.0f), vy, swW, 28.0f};
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
        vy += 42.0f;

        if (DrawButton(Rectangle{xStart + 18.0f, vy, contentW - 36.0f, 36.0f},
                       launchSettings.isStatic ? "ANCHORED (STATIC) SPAWN: [ENABLED]" : "DYNAMIC SPAWN: [NORMAL]",
                       Color{28, 44, 70, 230}, launchSettings.isStatic ? GOLD : RAYWHITE, launchSettings.isStatic, 13.0f)) {
            launchSettings.isStatic = !launchSettings.isStatic;
        }
        vy += 44.0f;

        drawText("Trajectory Prediction Steps:", xStart + 18.0f, vy + 4.0f, 14.0f, LIGHTGRAY);
        float stepBtnW = 74.0f;
        if (DrawButton(Rectangle{xStart + 215.0f, vy, stepBtnW, 28.0f}, "150", Color{26, 38, 56, 230}, RAYWHITE,
                       launchSettings.trajectorySteps == 150, 13.0f)) {
            launchSettings.trajectorySteps = 150;
        }
        if (DrawButton(Rectangle{xStart + 295.0f, vy, stepBtnW, 28.0f}, "250", Color{26, 38, 56, 230}, RAYWHITE,
                       launchSettings.trajectorySteps == 250, 13.0f)) {
            launchSettings.trajectorySteps = 250;
        }
        if (DrawButton(Rectangle{xStart + 375.0f, vy, stepBtnW, 28.0f}, "400", Color{26, 38, 56, 230}, RAYWHITE,
                       launchSettings.trajectorySteps == 400, 13.0f)) {
            launchSettings.trajectorySteps = 400;
        }
    }
}

bool Renderer::isMouseOverUI(Vector2 mousePos, int screenWidth, int screenHeight) const {
    if (CheckCollisionPointRec(mousePos, Rectangle{15, 15, 350, 280})) return true;
    if (CheckCollisionPointRec(mousePos, Rectangle{15, static_cast<float>(screenHeight - 120), 480, 105})) return true;

    if (sidebarVisible) {
        if (mousePos.x >= screenWidth - sidebarWidth) return true;
    } else {
        if (CheckCollisionPointRec(mousePos, Rectangle{static_cast<float>(screenWidth - 175), 18.0f, 160.0f, 38.0f})) return true;
    }

    return false;
}
