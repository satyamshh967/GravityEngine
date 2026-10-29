#include "GravityEngine.hpp"
#include "Renderer.hpp"
#include "LaunchSettings.hpp"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cmath>
#include <random>
#include <algorithm>

int runHeadlessPhysicsTest() {
    std::cout << "========================================================\n";
    std::cout << "  RUNNING 2D N-BODY GRAVITY ENGINE HEADLESS UNIT TEST   \n";
    std::cout << "========================================================\n\n";

    GravityEngine engine;
    engine.loadTwoBodyOrbitTest();
    engine.integrator = IntegratorType::VelocityVerlet;
    engine.fixedDt = 0.002;

    double e0 = engine.computeTotalEnergy();
    std::cout << "[TEST] Integrator: Velocity Verlet\n";
    std::cout << "[TEST] Initial Mechanical Energy E0 = " << std::setprecision(10) << e0 << "\n";

    const int totalSteps = 1000;
    for (int step = 1; step <= totalSteps; ++step) {
        engine.step(engine.fixedDt);
    }

    double eFinal = engine.computeTotalEnergy();
    double drift = engine.getEnergyDriftPercent();

    std::cout << "[TEST] Steps executed: " << totalSteps << " (Simulated time: " << totalSteps * engine.fixedDt << "s)\n";
    std::cout << "[TEST] Final Mechanical Energy E1000 = " << std::setprecision(10) << eFinal << "\n";
    std::cout << "[TEST] Absolute Energy Drift: " << std::abs(eFinal - e0) << "\n";
    std::cout << "[TEST] Relative Energy Drift: " << std::setprecision(6) << drift << "%\n";

    // Compare with Symplectic Euler on the identical setup
    GravityEngine eulerEngine;
    eulerEngine.loadTwoBodyOrbitTest();
    eulerEngine.integrator = IntegratorType::SymplecticEuler;
    eulerEngine.fixedDt = 0.002;
    double eulerE0 = eulerEngine.computeTotalEnergy();
    for (int step = 1; step <= totalSteps; ++step) {
        eulerEngine.step(eulerEngine.fixedDt);
    }
    double eulerFinal = eulerEngine.computeTotalEnergy();
    double eulerDrift = eulerEngine.getEnergyDriftPercent();

    std::cout << "\n[COMPARISON] Symplectic Euler E0: " << std::setprecision(10) << eulerE0
              << ", E1000: " << eulerFinal
              << ", Energy Drift: " << std::setprecision(6) << eulerDrift << "%\n";

    std::cout << "\n[ASSERTION] Checking condition: drift < 0.01% ... ";
    if (drift < 0.01) {
        std::cout << "PASSED! (Velocity Verlet drift = " << drift << "% < 0.01%)\n";
        std::cout << "========================================================\n";
        return 0;
    } else {
        std::cout << "FAILED! (Drift was " << drift << "% >= 0.01%)\n";
        std::cout << "========================================================\n";
        return 1;
    }
}

int main(int argc, char* argv[]) {
    // Check for --test CLI flag
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--test") == 0 || std::strcmp(argv[i], "-t") == 0) {
            return runHeadlessPhysicsTest();
        }
    }

    // Interactive graphical window
    const int screenWidth = 1440;
    const int screenHeight = 900;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "2D N-Body Gravity Engine (C++20)");
    SetTargetFPS(60);

    GravityEngine engine;
    Renderer renderer(screenWidth, screenHeight);
    LaunchSettings launchSettings;

    double fixedDt = 0.002;
    float timeScale = 1.0f;
    bool isPaused = false;
    double accumulator = 0.0;

    // Selection & Spawner state
    int selectedBodyId = -1;

    // Slingshot spawning state
    bool isDragging = false;
    Vec2 dragStartWorld{0.0, 0.0};
    Vec2 dragCurrentWorld{0.0, 0.0};
    int nextSpawnId = 1000;

    while (!WindowShouldClose()) {
        int currentW = GetScreenWidth();
        int currentH = GetScreenHeight();
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f; // Clamp against spiral of death

        // --- Keyboard Shortcuts & Input Handling ---

        // Pause toggle
        if (IsKeyPressed(KEY_SPACE)) {
            isPaused = !isPaused;
        }

        // Toggle Right Sidebar visibility [H]
        if (IsKeyPressed(KEY_H)) {
            renderer.sidebarVisible = !renderer.sidebarVisible;
        }

        // Toggle Sidebar Spawner Tab [TAB]
        if (IsKeyPressed(KEY_TAB)) {
            if (!renderer.sidebarVisible) {
                renderer.sidebarVisible = true;
                renderer.activeTab = SidebarTab::Spawner;
            } else {
                renderer.activeTab = (renderer.activeTab == SidebarTab::Spawner)
                                     ? SidebarTab::Inspector : SidebarTab::Spawner;
            }
        }

        // Deselect or close sidebar with ESC
        if (IsKeyPressed(KEY_ESCAPE)) {
            if (selectedBodyId != -1) {
                selectedBodyId = -1;
            } else if (renderer.sidebarVisible) {
                renderer.sidebarVisible = false;
            }
        }

        // Preset loading
        if (IsKeyPressed(KEY_ONE)) {
            engine.loadSolarSystem();
            renderer.resetCamera(currentW, currentH);
            selectedBodyId = -1;
        }
        if (IsKeyPressed(KEY_TWO)) {
            engine.loadFigureEight();
            renderer.resetCamera(currentW, currentH);
            selectedBodyId = -1;
        }
        if (IsKeyPressed(KEY_THREE)) {
            engine.loadBinaryDisk();
            renderer.resetCamera(currentW, currentH);
            selectedBodyId = -1;
        }

        // Integrator toggle
        if (IsKeyPressed(KEY_I)) {
            if (engine.integrator == IntegratorType::VelocityVerlet) {
                engine.integrator = IntegratorType::SymplecticEuler;
            } else {
                engine.integrator = IntegratorType::VelocityVerlet;
            }
        }

        // Collision mode toggle
        if (IsKeyPressed(KEY_C)) {
            if (engine.collisionMode == CollisionMode::InelasticMerge) {
                engine.collisionMode = CollisionMode::Elastic;
            } else if (engine.collisionMode == CollisionMode::Elastic) {
                engine.collisionMode = CollisionMode::None;
            } else {
                engine.collisionMode = CollisionMode::InelasticMerge;
            }
        }

        // Visual toggles
        if (IsKeyPressed(KEY_T)) renderer.showTrails = !renderer.showTrails;
        if (IsKeyPressed(KEY_V)) renderer.showVelocityVectors = !renderer.showVelocityVectors;
        if (IsKeyPressed(KEY_G)) renderer.showGrid = !renderer.showGrid;
        if (IsKeyPressed(KEY_F)) renderer.followHeaviest = !renderer.followHeaviest;

        // Reset camera
        if (IsKeyPressed(KEY_R)) {
            renderer.resetCamera(currentW, currentH);
            selectedBodyId = -1;
        }

        // Clear all bodies
        if (IsKeyPressed(KEY_X)) {
            engine.clearBodies();
            selectedBodyId = -1;
        }

        // Time-scaling controls: Left/Right brackets or < / >
        if (IsKeyPressed(KEY_RIGHT_BRACKET) || IsKeyPressed(KEY_PERIOD)) {
            timeScale = std::min(timeScale + 0.5f, 10.0f);
        }
        if (IsKeyPressed(KEY_LEFT_BRACKET) || IsKeyPressed(KEY_COMMA)) {
            timeScale = std::max(timeScale - 0.5f, 0.25f);
        }

        // Spawn mass adjustment via +/- keys
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
            launchSettings.mass = std::min(launchSettings.mass * 1.25, 200000.0);
            launchSettings.recalculateRadiusFromDensity();
        }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
            launchSettings.mass = std::max(launchSettings.mass / 1.25, 0.01);
            launchSettings.recalculateRadiusFromDensity();
        }

        // Camera update (pan, zoom)
        renderer.updateCamera(dt);

        // Follow selected locked body or heaviest body
        if (renderer.followedBodyId != -1) {
            Body* followed = engine.getBodyById(renderer.followedBodyId);
            if (followed) {
                renderer.camera.target = followed->pos.toVector2();
            } else {
                renderer.followedBodyId = -1;
            }
        } else if (renderer.followHeaviest && !engine.bodies.empty()) {
            size_t heaviestIdx = 0;
            double maxMass = -1.0;
            for (size_t i = 0; i < engine.bodies.size(); ++i) {
                if (engine.bodies[i].mass > maxMass) {
                    maxMass = engine.bodies[i].mass;
                    heaviestIdx = i;
                }
            }
            renderer.camera.target = engine.bodies[heaviestIdx].pos.toVector2();
        }

        // Mouse interactions in world coordinates
        Vector2 mouseScreen = GetMousePosition();
        Vec2 mouseWorld = renderer.screenToWorld(mouseScreen);
        bool mouseOverUI = renderer.isMouseOverUI(mouseScreen, currentW, currentH);

        // Object selection & Launch Spawning
        if (!mouseOverUI) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                // Check if user clicked on an existing body
                int clickedId = engine.findNearestBody(mouseWorld, 28.0 / renderer.camera.zoom);
                if (clickedId != -1) {
                    selectedBodyId = clickedId;
                    renderer.activeTab = SidebarTab::Inspector;
                    renderer.sidebarVisible = true; // Auto-open sidebar when clicking an object
                    isDragging = false;
                } else {
                    // Clicked on empty space
                    if (launchSettings.spawnMode == LaunchSpawnMode::CircularOrbit) {
                        Vec2 orbVel = engine.computeCircularOrbitVelocity(mouseWorld);
                        int spawnedId = nextSpawnId++;
                        Body newBody(spawnedId, launchSettings.typeName + " #" + std::to_string(spawnedId),
                                     launchSettings.mass, launchSettings.radius, mouseWorld, orbVel,
                                     launchSettings.color, launchSettings.isStatic);
                        newBody.maxTrailPoints = launchSettings.trailLength;
                        engine.addBody(newBody);
                        selectedBodyId = spawnedId;
                        renderer.activeTab = SidebarTab::Inspector;
                        renderer.sidebarVisible = true;
                    } else if (launchSettings.spawnMode == LaunchSpawnMode::Stationary) {
                        int spawnedId = nextSpawnId++;
                        Body newBody(spawnedId, launchSettings.typeName + " #" + std::to_string(spawnedId),
                                     launchSettings.mass, launchSettings.radius, mouseWorld, Vec2{0.0, 0.0},
                                     launchSettings.color, launchSettings.isStatic);
                        newBody.maxTrailPoints = launchSettings.trailLength;
                        engine.addBody(newBody);
                        selectedBodyId = spawnedId;
                        renderer.activeTab = SidebarTab::Inspector;
                        renderer.sidebarVisible = true;
                    } else {
                        // Slingshot drag launch mode
                        selectedBodyId = -1;
                        isDragging = true;
                        dragStartWorld = mouseWorld;
                        dragCurrentWorld = mouseWorld;
                    }
                }
            }
        }

        if (isDragging) {
            dragCurrentWorld = mouseWorld;

            // Adjust launch mass via mouse wheel while dragging
            float wheel = GetMouseWheelMove();
            if (wheel > 0.0f) {
                launchSettings.mass = std::min(launchSettings.mass * 1.2, 200000.0);
                launchSettings.recalculateRadiusFromDensity();
            }
            if (wheel < 0.0f) {
                launchSettings.mass = std::max(launchSettings.mass / 1.2, 0.01);
                launchSettings.recalculateRadiusFromDensity();
            }

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                const double velFactor = 0.5;
                Vec2 spawnVel = (dragStartWorld - dragCurrentWorld) * velFactor;

                int spawnedId = nextSpawnId++;
                Body newBody(spawnedId, launchSettings.typeName + " #" + std::to_string(spawnedId),
                             launchSettings.mass, launchSettings.radius, dragStartWorld, spawnVel,
                             launchSettings.color, launchSettings.isStatic);
                newBody.maxTrailPoints = launchSettings.trailLength;
                engine.addBody(newBody);

                isDragging = false;
                selectedBodyId = spawnedId;
                renderer.activeTab = SidebarTab::Inspector;
                renderer.sidebarVisible = true;
            }
        }

        // --- Physics Fixed-Timestep Accumulator Loop ---
        if (!isPaused) {
            accumulator += dt * timeScale;
            if (accumulator > 0.2) accumulator = 0.2; // Cap accumulator to avoid spiral of death

            while (accumulator >= fixedDt) {
                engine.step(fixedDt);
                accumulator -= fixedDt;
            }
        }

        // --- Rendering ---
        BeginDrawing();
        ClearBackground(Color{10, 12, 20, 255}); // Deep space aesthetic

        // 1. World-Space Rendering
        BeginMode2D(renderer.camera);

        renderer.drawGrid();
        renderer.drawTrails(engine.bodies);
        renderer.drawBodies(engine.bodies, selectedBodyId);

        // Highlight selected body reticle
        if (selectedBodyId != -1) {
            const Body* sel = engine.getBodyById(selectedBodyId);
            if (sel) {
                renderer.drawSelectionReticle(*sel);
            }
        }

        renderer.drawVelocityVectors(engine.bodies);

        // Draw slingshot aiming & trajectory preview
        if (isDragging) {
            const double velFactor = 0.5;
            Vec2 launchVel = (dragStartWorld - dragCurrentWorld) * velFactor;

            // Pull-back line
            DrawLineEx(dragStartWorld.toVector2(), dragCurrentWorld.toVector2(), 2.0f, Fade(YELLOW, 0.7f));
            DrawCircleV(dragCurrentWorld.toVector2(), 4.0f, RED);

            // Real-time predicted trajectory arc
            auto trajectory = engine.predictTrajectory(dragStartWorld, launchVel, launchSettings.mass,
                                                      launchSettings.trajectorySteps, 0.005);
            renderer.drawTrajectoryPreview(trajectory, dragStartWorld, launchSettings.radius, launchSettings.color);
        }

        EndMode2D();

        // 2. Screen-Space UI Panels
        renderer.drawHUD(engine, timeScale, isPaused, launchSettings, currentW, currentH);

        // Draw Enlarged Full-Height Right Sidebar (Inspector & Spawner Tabs) with Hide/Show
        Body* sel = (selectedBodyId != -1) ? engine.getBodyById(selectedBodyId) : nullptr;
        bool shouldDeselect = false;
        bool shouldDelete = false;

        renderer.drawSidebar(sel, engine, launchSettings, currentW, currentH, shouldDeselect, shouldDelete);

        if (shouldDelete) {
            engine.deleteBody(selectedBodyId);
            selectedBodyId = -1;
        } else if (shouldDeselect) {
            selectedBodyId = -1;
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
