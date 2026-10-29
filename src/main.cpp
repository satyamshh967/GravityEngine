#include "GravityEngine.hpp"
#include "Renderer.hpp"
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

    double fixedDt = 0.002;
    float timeScale = 1.0f;
    bool isPaused = false;
    double accumulator = 0.0;

    // Slingshot spawning state
    bool isDragging = false;
    Vec2 dragStartWorld{0.0, 0.0};
    Vec2 dragCurrentWorld{0.0, 0.0};
    double spawnMass = 10.0;
    int nextSpawnId = 1000;

    // Random generator for spawned bodies
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> colorDist(100, 255);

    while (!WindowShouldClose()) {
        int currentW = GetScreenWidth();
        int currentH = GetScreenHeight();
        float dt = GetFrameTime();
        if (dt > 0.1f) dt = 0.1f; // Clamp to prevent spiral of death

        // --- Input Handling ---

        // Pause toggle
        if (IsKeyPressed(KEY_SPACE)) {
            isPaused = !isPaused;
        }

        // Preset loading
        if (IsKeyPressed(KEY_ONE)) {
            engine.loadSolarSystem();
            renderer.resetCamera(currentW, currentH);
        }
        if (IsKeyPressed(KEY_TWO)) {
            engine.loadFigureEight();
            renderer.resetCamera(currentW, currentH);
        }
        if (IsKeyPressed(KEY_THREE)) {
            engine.loadBinaryDisk();
            renderer.resetCamera(currentW, currentH);
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
        }

        // Clear all bodies
        if (IsKeyPressed(KEY_X)) {
            engine.clearBodies();
        }

        // Time-scaling controls: Left/Right brackets or Left/Right keys
        if (IsKeyPressed(KEY_RIGHT_BRACKET) || IsKeyPressed(KEY_PERIOD)) {
            timeScale = std::min(timeScale + 0.5f, 10.0f);
        }
        if (IsKeyPressed(KEY_LEFT_BRACKET) || IsKeyPressed(KEY_COMMA)) {
            timeScale = std::max(timeScale - 0.5f, 0.25f);
        }

        // Spawn mass adjustment
        if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) {
            spawnMass = std::min(spawnMass * 1.5, 10000.0);
        }
        if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) {
            spawnMass = std::max(spawnMass / 1.5, 0.1);
        }

        // Camera update (pan, zoom)
        renderer.updateCamera(dt);

        // Follow heaviest body if enabled
        if (renderer.followHeaviest && !engine.bodies.empty()) {
            size_t heaviestIdx = 0;
            double maxMass = -1.0;
            for (size_t i = 0; i < engine.bodies.size(); ++i) {
                if (engine.bodies[i].mass > maxMass) {
                    maxMass = engine.bodies[i].mass;
                    heaviestIdx = i;
                }
            }
            Vector2 targetScreen = engine.bodies[heaviestIdx].pos.toVector2();
            renderer.camera.target = targetScreen;
        }

        // Interactive Slingshot Spawning with Left Mouse Button
        Vector2 mouseScreen = GetMousePosition();
        Vec2 mouseWorld = renderer.screenToWorld(mouseScreen);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            isDragging = true;
            dragStartWorld = mouseWorld;
            dragCurrentWorld = mouseWorld;
        }

        if (isDragging) {
            dragCurrentWorld = mouseWorld;

            // Allow mouse wheel to adjust spawn mass while dragging
            float wheel = GetMouseWheelMove();
            if (wheel > 0.0f) spawnMass = std::min(spawnMass * 1.25, 10000.0);
            if (wheel < 0.0f) spawnMass = std::max(spawnMass / 1.25, 0.1);

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                // Pullback slingshot velocity: (dragStart - dragCurrent) * velFactor
                const double velFactor = 0.5;
                Vec2 spawnVel = (dragStartWorld - dragCurrentWorld) * velFactor;
                double spawnRadius = std::clamp(3.0 + std::cbrt(spawnMass) * 1.6, 3.0, 32.0);

                Color newColor = Color{
                    static_cast<unsigned char>(colorDist(rng)),
                    static_cast<unsigned char>(colorDist(rng)),
                    static_cast<unsigned char>(colorDist(rng)),
                    255
                };

                int spawnedId = nextSpawnId++;
                Body newBody(spawnedId, "Launched #" + std::to_string(spawnedId),
                             spawnMass, spawnRadius, dragStartWorld, spawnVel, newColor, false);
                newBody.maxTrailPoints = 300;
                engine.addBody(newBody);

                isDragging = false;
            }
        }

        // --- Physics Fixed-Timestep Accumulator Loop ---
        if (!isPaused) {
            accumulator += dt * timeScale;
            // Cap accumulator to avoid spiral of death if frame takes too long
            if (accumulator > 0.2) accumulator = 0.2;

            while (accumulator >= fixedDt) {
                engine.step(fixedDt);
                accumulator -= fixedDt;
            }
        }

        // --- Rendering ---
        BeginDrawing();
        ClearBackground(Color{10, 12, 20, 255}); // Deep space aesthetic

        BeginMode2D(renderer.camera);

        renderer.drawGrid();
        renderer.drawTrails(engine.bodies);
        renderer.drawBodies(engine.bodies);
        renderer.drawVelocityVectors(engine.bodies);

        // Draw slingshot aiming and predicted trajectory
        if (isDragging) {
            const double velFactor = 0.5;
            Vec2 launchVel = (dragStartWorld - dragCurrentWorld) * velFactor;
            double spawnRadius = std::clamp(3.0 + std::cbrt(spawnMass) * 1.6, 3.0, 32.0);

            // Pull-back line
            DrawLineEx(dragStartWorld.toVector2(), dragCurrentWorld.toVector2(), 2.0f, Fade(YELLOW, 0.7f));
            DrawCircleV(dragCurrentWorld.toVector2(), 4.0f, RED);

            // Compute and draw trajectory prediction
            auto trajectory = engine.predictTrajectory(dragStartWorld, launchVel, spawnMass, 220, 0.005);
            renderer.drawTrajectoryPreview(trajectory, dragStartWorld, spawnRadius, Color{255, 230, 80, 255});
        }

        EndMode2D();

        // Screen-space UI
        renderer.drawHUD(engine, timeScale, isPaused, spawnMass, currentW, currentH);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
