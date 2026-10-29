#pragma once

#include "Body.hpp"
#include <vector>
#include <string>
#include <cmath>

enum class IntegratorType {
    VelocityVerlet,
    SymplecticEuler
};

enum class CollisionMode {
    None,
    Elastic,
    InelasticMerge
};

class GravityEngine {
public:
    double G{1.0};
    double epsilon{5.0}; // Softening parameter
    IntegratorType integrator{IntegratorType::VelocityVerlet};
    CollisionMode collisionMode{CollisionMode::InelasticMerge};
    double fixedDt{0.002};
    double restitution{1.0}; // Elastic collision restitution coefficient

    std::vector<Body> bodies;
    double initialEnergy{0.0};
    bool initialEnergyValid{false};

    GravityEngine();

    // Simulation stepping
    void step(double dt);
    void computeAccelerations();

    // Specific integrators
    void stepVelocityVerlet(double dt);
    void stepSymplecticEuler(double dt);

    // Collision detection and response
    void handleCollisions();

    // Energy computations
    [[nodiscard]] double computeKineticEnergy() const;
    [[nodiscard]] double computePotentialEnergy() const;
    [[nodiscard]] double computeTotalEnergy() const;
    [[nodiscard]] double getEnergyDriftPercent() const;
    void resetInitialEnergy();

    // Trajectory prediction for slingshot spawner
    [[nodiscard]] std::vector<Vec2> predictTrajectory(
        Vec2 startPos, Vec2 startVel, double mass, int steps = 250, double dt = 0.005) const;

    // Body management
    void addBody(const Body& b);
    void clearBodies();
    [[nodiscard]] size_t getBodyCount() const { return bodies.size(); }

    // Preset scene loaders
    void loadSolarSystem();
    void loadFigureEight();
    void loadBinaryDisk();
    void loadTwoBodyOrbitTest();

    // Helper to get string representation of current state
    [[nodiscard]] const char* getIntegratorName() const;
    [[nodiscard]] const char* getCollisionModeName() const;
};
