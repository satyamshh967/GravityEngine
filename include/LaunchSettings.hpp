#pragma once

#include <raylib.h>
#include <string>
#include <algorithm>
#include <cmath>

enum class LaunchPresetType {
    Asteroid,
    Moon,
    Terrestrial,
    GasGiant,
    Star,
    BlackHole,
    Custom
};

enum class LaunchSpawnMode {
    Slingshot,       // Drag to pull back velocity vector
    CircularOrbit,   // Click to place in circular orbit around nearest body
    Stationary       // Spawn with zero velocity (anchored or floating)
};

struct LaunchSettings {
    LaunchPresetType presetType{LaunchPresetType::Terrestrial};
    LaunchSpawnMode spawnMode{LaunchSpawnMode::Slingshot};
    double mass{80.0};
    double radius{6.5};
    double density{0.60}; // mass / (pi * radius^2)
    Color color{Color{70, 160, 255, 255}};
    bool isStatic{false};
    int trajectorySteps{250};
    size_t trailLength{350};
    std::string typeName{"Planet"};

    LaunchSettings() {
        applyPreset(LaunchPresetType::Terrestrial);
    }

    void applyPreset(LaunchPresetType type) {
        presetType = type;
        constexpr double kPi = 3.14159265358979323846;

        switch (type) {
            case LaunchPresetType::Asteroid:
                mass = 0.5;
                radius = 2.5;
                density = mass / (kPi * radius * radius);
                color = Color{180, 180, 180, 255};
                isStatic = false;
                typeName = "Asteroid";
                trailLength = 200;
                break;
            case LaunchPresetType::Moon:
                mass = 6.0;
                radius = 3.8;
                density = mass / (kPi * radius * radius);
                color = Color{210, 215, 220, 255};
                isStatic = false;
                typeName = "Moon";
                trailLength = 250;
                break;
            case LaunchPresetType::Terrestrial:
                mass = 80.0;
                radius = 6.5;
                density = mass / (kPi * radius * radius);
                color = Color{70, 160, 255, 255};
                isStatic = false;
                typeName = "Planet";
                trailLength = 350;
                break;
            case LaunchPresetType::GasGiant:
                mass = 650.0;
                radius = 13.0;
                density = mass / (kPi * radius * radius);
                color = Color{235, 175, 110, 255};
                isStatic = false;
                typeName = "Gas Giant";
                trailLength = 400;
                break;
            case LaunchPresetType::Star:
                mass = 20000.0;
                radius = 22.0;
                density = mass / (kPi * radius * radius);
                color = Color{255, 215, 30, 255};
                isStatic = false;
                typeName = "Star";
                trailLength = 450;
                break;
            case LaunchPresetType::BlackHole:
                mass = 80000.0;
                radius = 8.0;
                density = mass / (kPi * radius * radius);
                color = Color{220, 50, 255, 255};
                isStatic = false;
                typeName = "Singularity";
                trailLength = 500;
                break;
            case LaunchPresetType::Custom:
                typeName = "Custom";
                break;
        }
    }

    void recalculateRadiusFromDensity() {
        constexpr double kPi = 3.14159265358979323846;
        if (density > 1e-6) {
            radius = std::clamp(std::sqrt(mass / (kPi * density)), 1.5, 90.0);
        }
    }

    void recalculateDensityFromRadius() {
        constexpr double kPi = 3.14159265358979323846;
        if (radius > 1e-4) {
            density = mass / (kPi * radius * radius);
        }
    }

    [[nodiscard]] const char* getSpawnModeName() const {
        switch (spawnMode) {
            case LaunchSpawnMode::Slingshot: return "Slingshot (Drag)";
            case LaunchSpawnMode::CircularOrbit: return "Auto Circular Orbit";
            case LaunchSpawnMode::Stationary: return "Stationary";
            default: return "Unknown";
        }
    }
};
