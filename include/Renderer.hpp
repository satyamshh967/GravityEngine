#pragma once

#include "GravityEngine.hpp"
#include "LaunchSettings.hpp"
#include <raylib.h>
#include <vector>

enum class SidebarTab {
    Inspector,
    Spawner
};

class Renderer {
public:
    Camera2D camera{};
    bool showTrails{true};
    bool showVelocityVectors{true};
    bool showGrid{true};
    bool followHeaviest{false};
    int followedBodyId{-1};

    // Typography
    Font uiFont{};
    Font boldFont{};
    bool fontLoaded{false};

    // Sidebar state & dimensions (Enlarged and spacious)
    float sidebarWidth{520.0f};
    bool sidebarVisible{true};
    SidebarTab activeTab{SidebarTab::Inspector};

    float animTime{0.0f};

    Renderer(int screenWidth, int screenHeight);
    ~Renderer();

    void initFonts();
    void unloadFonts();

    // Typography helpers
    void drawText(const char* text, float x, float y, float size, Color col, bool bold = false) const;
    float measureText(const char* text, float size, bool bold = false) const;

    // Camera controls
    void updateCamera(float deltaTime);
    void resetCamera(int screenWidth, int screenHeight);

    // Drawing functions in World Space
    void drawGrid() const;
    void drawRoom(const GravityEngine& engine) const;
    void drawTrails(const std::vector<Body>& bodies) const;
    void drawBodies(const std::vector<Body>& bodies, int selectedId = -1) const;
    void drawVelocityVectors(const std::vector<Body>& bodies) const;
    void drawDestructionEffects(const GravityEngine& engine) const;
    void drawSelectionReticle(const Body& body) const;
    void drawTrajectoryPreview(const std::vector<Vec2>& trajectoryPoints,
                               Vec2 spawnPos, double spawnRadius, Color spawnColor) const;

    // HUD & Interactive Sidebar in Screen Space
    void drawHUD(const GravityEngine& engine, float timeScale, bool isPaused,
                 const LaunchSettings& launchSettings, int screenWidth, int screenHeight) const;

    void drawSidebar(Body* selectedBody, GravityEngine& engine, LaunchSettings& launchSettings,
                     int screenWidth, int screenHeight, bool& shouldDeselect, bool& shouldDelete);

    void drawSidebarTogglePill(int screenWidth, int screenHeight);

    [[nodiscard]] bool isMouseOverUI(Vector2 mousePos, int screenWidth, int screenHeight) const;

    // Coordinate conversion
    [[nodiscard]] Vec2 screenToWorld(Vector2 screenPos) const;
    [[nodiscard]] Vector2 worldToScreen(Vec2 worldPos) const;

    // UI helper widgets
    bool DrawButton(Rectangle rect, const char* text, Color bg, Color textCol, bool isActive = false, float fontSize = 14.0f) const;
    void DrawCard(Rectangle rect, const char* title = nullptr, Color titleCol = GOLD) const;
};
