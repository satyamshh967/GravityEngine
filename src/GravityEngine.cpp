#include "GravityEngine.hpp"
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>

GravityEngine::GravityEngine() {
    loadSolarSystem();
}

void GravityEngine::computeAccelerations() {
    const size_t n = bodies.size();
    for (size_t i = 0; i < n; ++i) {
        bodies[i].acc = (enableUniformGravity && !bodies[i].isStatic) ? uniformGravity : Vec2{0.0, 0.0};
    }

    if (!enableUniformGravity || !disableMutualGravityInRoom) {
        const double epsSq = epsilon * epsilon;

        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                Vec2 r_ij = bodies[j].pos - bodies[i].pos;
                double distSq = r_ij.lengthSq();
                double softenedDist = std::sqrt(distSq + epsSq);
                double denom = (distSq + epsSq) * softenedDist;

                if (denom < 1e-12) {
                    continue;
                }

                double fFactor = G / denom;

                if (!bodies[i].isStatic) {
                    bodies[i].acc += r_ij * (fFactor * bodies[j].mass);
                }
                if (!bodies[j].isStatic) {
                    bodies[j].acc -= r_ij * (fFactor * bodies[i].mass);
                }
            }
        }
    }
}

void GravityEngine::stepVelocityVerlet(double dt) {
    const size_t n = bodies.size();
    const double halfDtSq = 0.5 * dt * dt;
    const double halfDt = 0.5 * dt;

    // 1. Update positions: r(t + dt) = r(t) + v(t)*dt + 0.5*a(t)*dt^2
    for (size_t i = 0; i < n; ++i) {
        if (!bodies[i].isStatic) {
            bodies[i].pos += bodies[i].vel * dt + bodies[i].acc * halfDtSq;
        }
    }

    // 2. Store a(t)
    std::vector<Vec2> oldAcc(n);
    for (size_t i = 0; i < n; ++i) {
        oldAcc[i] = bodies[i].acc;
    }

    // 3. Compute new accelerations a(t + dt) based on updated positions
    computeAccelerations();

    // 4. Update velocities: v(t + dt) = v(t) + 0.5*(a(t) + a(t + dt))*dt
    for (size_t i = 0; i < n; ++i) {
        if (!bodies[i].isStatic) {
            bodies[i].vel += (oldAcc[i] + bodies[i].acc) * halfDt;
        }
    }
}

void GravityEngine::stepSymplecticEuler(double dt) {
    const size_t n = bodies.size();

    // 1. Update velocities: v(t + dt) = v(t) + a(t)*dt
    // 2. Update positions: r(t + dt) = r(t) + v(t + dt)*dt
    for (size_t i = 0; i < n; ++i) {
        if (!bodies[i].isStatic) {
            bodies[i].vel += bodies[i].acc * dt;
            bodies[i].pos += bodies[i].vel * dt;
        }
    }

    // 3. Compute a(t + dt)
    computeAccelerations();
}

void GravityEngine::step(double dt) {
    if (bodies.empty() && !roomBounds.enabled) return;

    if (integrator == IntegratorType::VelocityVerlet) {
        stepVelocityVerlet(dt);
    } else {
        stepSymplecticEuler(dt);
    }

    // Update trails
    for (auto& body : bodies) {
        body.addTrailPoint(body.pos);
    }

    // Handle collisions between bodies
    handleCollisions();

    // Handle room boundary collisions & height-dependent shatter
    if (roomBounds.enabled) {
        handleRoomCollisions(dt);
    }
}

void GravityEngine::spawnShockwave(Vec2 pos, float maxRadius, Color color, float speed) {
    if (shockwaves.size() > 30) {
        shockwaves.erase(shockwaves.begin());
    }
    ShockwaveRing ring;
    ring.center = pos;
    ring.radius = 4.0f;
    ring.maxRadius = maxRadius;
    ring.speed = speed;
    ring.color = color;
    ring.alpha = 1.0f;
    shockwaves.push_back(ring);
}

void GravityEngine::spawnImpactSparks(Vec2 pos, Vec2 normal, double energy, Color c1, Color c2, int count) {
    std::mt19937 rng(static_cast<unsigned>(pos.x * 100 + pos.y * 10 + count));
    std::uniform_real_distribution<float> angleSpread(-1.2f, 1.2f);
    std::uniform_real_distribution<float> speedDist(15.0f, static_cast<float>(std::min(220.0, 20.0 + std::sqrt(energy) * 8.0)));
    std::uniform_real_distribution<float> lifeDist(0.4f, 1.4f);
    std::uniform_real_distribution<float> sizeDist(1.5f, 4.0f);
    std::uniform_real_distribution<float> colMix(0.0f, 1.0f);

    float baseAngle = std::atan2(static_cast<float>(normal.y), static_cast<float>(normal.x));

    for (int k = 0; k < count; ++k) {
        if (destructionParticles.size() > 800) {
            destructionParticles.erase(destructionParticles.begin());
        }

        float a = baseAngle + angleSpread(rng);
        float spd = speedDist(rng);
        float life = lifeDist(rng);
        float t = colMix(rng);

        Color pCol{
            static_cast<unsigned char>(c1.r * (1.0f - t) + c2.r * t),
            static_cast<unsigned char>(c1.g * (1.0f - t) + c2.g * t),
            static_cast<unsigned char>(c1.b * (1.0f - t) + c2.b * t),
            255
        };

        DestructionParticle p;
        p.pos = pos;
        p.vel = Vec2{std::cos(a) * spd, std::sin(a) * spd};
        p.color = pCol;
        p.size = sizeDist(rng);
        p.lifetime = life;
        p.maxLifetime = life;
        destructionParticles.push_back(p);
    }
}

void GravityEngine::triggerDestructionExplosion(int bodyId, int particleCount) {
    Body* target = getBodyById(bodyId);
    if (!target) return;

    Vec2 pPos = target->pos;
    Vec2 pVel = target->vel;
    double pMass = target->mass;
    double pRadius = target->radius;
    Color pCol = target->color;

    // Camera shake
    cameraShakeIntensity = std::clamp(static_cast<float>(std::log10(pMass + 10.0) * 4.5f), 4.0f, 25.0f);

    // Shockwave rings
    spawnShockwave(pPos, static_cast<float>(pRadius * 7.0f), Color{255, 230, 120, 255}, 260.0f);
    spawnShockwave(pPos, static_cast<float>(pRadius * 4.0f), Color{255, 100, 50, 255}, 180.0f);

    // Spawn destruction embers and molten debris particles
    std::mt19937 rng(static_cast<unsigned>(pPos.x * 37 + pPos.y * 19 + pMass));
    std::uniform_real_distribution<float> angleDist(0.0f, 6.2831853f);
    std::uniform_real_distribution<float> speedDist(30.0f, 220.0f);
    std::uniform_real_distribution<float> lifeDist(0.8f, 2.8f);
    std::uniform_real_distribution<float> sizeDist(2.0f, 5.5f);

    for (int k = 0; k < particleCount; ++k) {
        if (destructionParticles.size() > 900) {
            destructionParticles.erase(destructionParticles.begin());
        }

        float a = angleDist(rng);
        float spd = speedDist(rng);
        float life = lifeDist(rng);

        Color emberCol = (k % 3 == 0) ? Color{255, 250, 200, 255} :
                         (k % 3 == 1) ? Color{255, 160, 40, 255} :
                                        Color{255, 60, 40, 255};

        DestructionParticle p;
        p.pos = pPos;
        p.vel = pVel + Vec2{std::cos(a) * spd, std::sin(a) * spd};
        p.color = emberCol;
        p.size = sizeDist(rng);
        p.lifetime = life;
        p.maxLifetime = life;
        destructionParticles.push_back(p);
    }

    // Spawn dynamic shattered fragments if mass is large enough
    if (pMass > 4.0) {
        int fragmentCount = std::clamp(static_cast<int>(3 + pMass * 0.05), 3, 6);
        double fragmentTotalMass = pMass * 0.4; // 40% of mass forms orbiting debris
        double singleFragMass = fragmentTotalMass / fragmentCount;
        double fragRadius = std::max(1.8, pRadius * 0.35);

        for (int f = 0; f < fragmentCount; ++f) {
            float fragAngle = (static_cast<float>(f) / fragmentCount) * 6.2831853f + 0.3f;
            float ejectSpeed = static_cast<float>(20.0 + std::sqrt(G * pMass / pRadius) * 0.6);

            Vec2 fragVel = pVel + Vec2{std::cos(fragAngle) * ejectSpeed, std::sin(fragAngle) * ejectSpeed};
            Vec2 fragPos = pPos + Vec2{std::cos(fragAngle) * (pRadius * 1.2), std::sin(fragAngle) * (pRadius * 1.2)};

            if (roomBounds.enabled) {
                fragPos.y = std::min(fragPos.y, roomBounds.yMax - fragRadius - 2.0);
                if (fragVel.y > 0) fragVel.y = -std::abs(fragVel.y) * 0.6; // Deflect upward off floor
            }

            int fragId = nextParticleBodyId++;
            Body frag(fragId, "Debris #" + std::to_string(fragId),
                      singleFragMass, fragRadius, fragPos, fragVel, pCol, false);
            frag.maxTrailPoints = 80;
            bodies.push_back(frag);
        }
    }

    deleteBody(bodyId);
    computeAccelerations();
}

void GravityEngine::updateParticles(double dt) {
    // 1. Update destruction particles
    for (auto it = destructionParticles.begin(); it != destructionParticles.end();) {
        it->lifetime -= static_cast<float>(dt);
        if (it->lifetime <= 0.0f) {
            it = destructionParticles.erase(it);
            continue;
        }

        // Apply gravitational acceleration from nearby bodies
        Vec2 gravAcc = computeNetAccelerationOnPoint(it->pos);
        it->vel += gravAcc * dt;

        // Subtle atmospheric / vacuum drag
        it->vel *= (1.0 - 0.15 * dt);
        it->pos += it->vel * dt;

        // Bounce sparks off floor and walls in Earth room
        if (roomBounds.enabled) {
            if (it->pos.y >= roomBounds.yMax) {
                it->pos.y = roomBounds.yMax;
                it->vel.y = -it->vel.y * 0.35;
                it->vel.x *= 0.85;
            }
            if (it->pos.x <= roomBounds.xMin) {
                it->pos.x = roomBounds.xMin;
                it->vel.x = -it->vel.x * 0.35;
            }
            if (it->pos.x >= roomBounds.xMax) {
                it->pos.x = roomBounds.xMax;
                it->vel.x = -it->vel.x * 0.35;
            }
        }

        // Fade size and color from hot incandescent to dark cool embers
        float progress = it->lifetime / it->maxLifetime;
        it->color.a = static_cast<unsigned char>(std::clamp(progress * 255.0f, 0.0f, 255.0f));

        ++it;
    }

    // 2. Update shockwave rings
    for (auto it = shockwaves.begin(); it != shockwaves.end();) {
        it->radius += it->speed * static_cast<float>(dt);
        it->alpha = std::clamp(1.0f - (it->radius / it->maxRadius), 0.0f, 1.0f);

        if (it->radius >= it->maxRadius || it->alpha <= 0.01f) {
            it = shockwaves.erase(it);
        } else {
            ++it;
        }
    }

    // 3. Decay camera shake
    cameraShakeIntensity = std::max(0.0f, cameraShakeIntensity - 9.0f * static_cast<float>(dt));
}

void GravityEngine::handleCollisions() {
    if (collisionMode == CollisionMode::None || bodies.size() < 2) {
        return;
    }

    if (collisionMode == CollisionMode::Elastic) {
        const size_t n = bodies.size();
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                double dist = bodies[i].pos.distance(bodies[j].pos);
                double minDist = bodies[i].radius + bodies[j].radius;

                if (dist < minDist && dist > 1e-9) {
                    Vec2 normal = (bodies[j].pos - bodies[i].pos) / dist;
                    Vec2 relVel = bodies[j].vel - bodies[i].vel;
                    double vNorm = relVel.dot(normal);

                    double invM1 = bodies[i].isStatic ? 0.0 : (1.0 / bodies[i].mass);
                    double invM2 = bodies[j].isStatic ? 0.0 : (1.0 / bodies[j].mass);
                    double invMSum = invM1 + invM2;

                    if (invMSum > 1e-14) {
                        if (vNorm < 0.0) { // Moving toward each other
                            double impulse = -(1.0 + restitution) * vNorm / invMSum;
                            if (!bodies[i].isStatic) {
                                bodies[i].vel -= normal * (impulse * invM1);
                            }
                            if (!bodies[j].isStatic) {
                                bodies[j].vel += normal * (impulse * invM2);
                            }

                            // Spawn elastic bounce sparks
                            double energy = 0.5 * (bodies[i].mass * bodies[j].mass / (bodies[i].mass + bodies[j].mass)) * vNorm * vNorm;
                            Vec2 contactPt = bodies[i].pos + normal * bodies[i].radius;
                            spawnImpactSparks(contactPt, normal, energy, bodies[i].color, bodies[j].color, 25);
                        }

                        // Positional overlap resolution
                        double overlap = minDist - dist;
                        if (overlap > 0.0) {
                            if (!bodies[i].isStatic) {
                                bodies[i].pos -= normal * (overlap * (invM1 / invMSum));
                            }
                            if (!bodies[j].isStatic) {
                                bodies[j].pos += normal * (overlap * (invM2 / invMSum));
                            }
                        }
                    }
                }
            }
        }
    } else if (collisionMode == CollisionMode::InelasticMerge || collisionMode == CollisionMode::DestructiveShatter) {
        std::vector<bool> removed(bodies.size(), false);
        bool anyMerged = false;

        for (size_t i = 0; i < bodies.size(); ++i) {
            if (removed[i]) continue;
            for (size_t j = i + 1; j < bodies.size(); ++j) {
                if (removed[j]) continue;

                double dist = bodies[i].pos.distance(bodies[j].pos);
                double minDist = bodies[i].radius + bodies[j].radius;

                if (dist < minDist) {
                    size_t survivor = i;
                    size_t absorbed = j;

                    Vec2 relVel = bodies[j].vel - bodies[i].vel;
                    double vRel = relVel.length();
                    double reducedMass = (bodies[i].mass * bodies[j].mass) / (bodies[i].mass + bodies[j].mass);
                    double impactEnergy = 0.5 * reducedMass * vRel * vRel;

                    // If DestructiveShatter mode and impact energy is severe:
                    if (collisionMode == CollisionMode::DestructiveShatter && (vRel > 15.0 || impactEnergy > 500.0)) {
                        Vec2 contactPoint = (bodies[i].pos * bodies[j].mass + bodies[j].pos * bodies[i].mass) / (bodies[i].mass + bodies[j].mass);
                        spawnShockwave(contactPoint, static_cast<float>(minDist * 4.0f), Color{255, 180, 50, 255}, 220.0f);
                        spawnImpactSparks(contactPoint, relVel.normalized(), impactEnergy, bodies[i].color, bodies[j].color, 90);
                        cameraShakeIntensity = std::clamp(static_cast<float>(vRel * 0.4f), 3.0f, 20.0f);
                    } else {
                        // Impact flash & sparks on merge
                        Vec2 contactPoint = (bodies[i].pos + bodies[j].pos) * 0.5;
                        spawnShockwave(contactPoint, static_cast<float>(minDist * 2.5f), Color{255, 230, 100, 255}, 160.0f);
                        spawnImpactSparks(contactPoint, relVel.normalized(), impactEnergy, bodies[i].color, bodies[j].color, 45);
                    }

                    bool survivorWasStatic = bodies[survivor].isStatic;
                    bool absorbedWasStatic = bodies[absorbed].isStatic;
                    bool shouldTakeAbsorbedIdentity = absorbedWasStatic || (!survivorWasStatic && bodies[absorbed].mass > bodies[survivor].mass);

                    double m1 = bodies[survivor].mass;
                    double m2 = bodies[absorbed].mass;
                    double totalMass = m1 + m2;

                    Vec2 newVel = bodies[survivor].vel;
                    Vec2 newPos = bodies[survivor].pos;

                    if (survivorWasStatic) {
                        // Survivor remains static
                    } else if (absorbedWasStatic) {
                        // Absorbed was static, survivor takes its static anchor
                        bodies[survivor].isStatic = true;
                        newPos = bodies[absorbed].pos;
                        newVel = Vec2{0.0, 0.0};
                    } else {
                        // Exact conservation of linear momentum: (m1*v1 + m2*v2) / (m1 + m2)
                        newVel = (bodies[survivor].vel * m1 + bodies[absorbed].vel * m2) / totalMass;
                        newPos = (bodies[survivor].pos * m1 + bodies[absorbed].pos * m2) / totalMass;
                    }

                    bodies[survivor].vel = newVel;
                    bodies[survivor].pos = newPos;

                    // 2D disk area conservation: R_new = sqrt(r1^2 + r2^2)
                    double newRadius = std::sqrt(bodies[survivor].radius * bodies[survivor].radius +
                                                 bodies[absorbed].radius * bodies[absorbed].radius);
                    bodies[survivor].radius = std::min(newRadius, 140.0);
                    bodies[survivor].mass = totalMass;

                    // Weighted color blend
                    Color c1 = bodies[survivor].color;
                    Color c2 = bodies[absorbed].color;
                    bodies[survivor].color = Color{
                        static_cast<unsigned char>(std::clamp((c1.r * m1 + c2.r * m2) / totalMass, 0.0, 255.0)),
                        static_cast<unsigned char>(std::clamp((c1.g * m1 + c2.g * m2) / totalMass, 0.0, 255.0)),
                        static_cast<unsigned char>(std::clamp((c1.b * m1 + c2.b * m2) / totalMass, 0.0, 255.0)),
                        255
                    };

                    if (shouldTakeAbsorbedIdentity) {
                        bodies[survivor].id = bodies[absorbed].id;
                        bodies[survivor].name = std::move(bodies[absorbed].name);
                        bodies[survivor].trail = std::move(bodies[absorbed].trail);
                    }

                    removed[absorbed] = true;
                    anyMerged = true;
                }
            }
        }

        if (anyMerged) {
            std::vector<Body> remaining;
            remaining.reserve(bodies.size());
            for (size_t i = 0; i < bodies.size(); ++i) {
                if (!removed[i]) {
                    remaining.push_back(std::move(bodies[i]));
                }
            }
            bodies = std::move(remaining);
            computeAccelerations();
        }
    }
}

void GravityEngine::handleRoomCollisions(double dt) {
    if (!roomBounds.enabled) return;

    std::vector<int> bodiesToShatter;

    for (size_t i = 0; i < bodies.size(); ++i) {
        Body& b = bodies[i];
        if (b.isStatic) continue;

        // 1. Floor collision (Bottom: yMax)
        if (b.pos.y + b.radius >= roomBounds.yMax) {
            double impactSpeed = std::abs(b.vel.y);

            // CRITICAL HEIGHT DESTRUCTION: If impact velocity exceeds shatter threshold
            if (impactSpeed >= roomBounds.shatterVelocity) {
                bodiesToShatter.push_back(b.id);
                continue;
            }

            // Normal elastic bounce
            b.pos.y = roomBounds.yMax - b.radius;
            b.vel.y = -b.vel.y * roomBounds.floorRestitution;

            // Resting threshold & rolling friction
            if (std::abs(b.vel.y) < 18.0) {
                b.vel.y = 0.0;
                b.vel.x *= (1.0 - 1.5 * dt);
                if (std::abs(b.vel.x) < 2.0) {
                    b.vel.x = 0.0;
                }
            } else {
                b.vel.x *= 0.96; // Surface friction during bounce
            }

            // Spawn ground impact sparks
            if (impactSpeed > 25.0) {
                double impactEnergy = 0.5 * b.mass * impactSpeed * impactSpeed;
                Vec2 contactPt{b.pos.x, roomBounds.yMax};
                Vec2 normal{0.0, -1.0};
                spawnImpactSparks(contactPt, normal, impactEnergy, b.color, Color{255, 230, 140, 255},
                                  std::clamp(static_cast<int>(impactSpeed * 0.06), 8, 35));
                if (impactSpeed > 320.0) {
                    spawnShockwave(contactPt, static_cast<float>(b.radius * 2.5), Color{255, 210, 100, 220}, 150.0f);
                }
            }
        }

        // 2. Ceiling collision (Top: yMin)
        if (b.pos.y - b.radius <= roomBounds.yMin) {
            double impactSpeed = std::abs(b.vel.y);
            if (impactSpeed >= roomBounds.shatterVelocity) {
                bodiesToShatter.push_back(b.id);
                continue;
            }
            b.pos.y = roomBounds.yMin + b.radius;
            b.vel.y = -b.vel.y * roomBounds.floorRestitution;
            b.vel.x *= 0.96;
            if (impactSpeed > 30.0) {
                spawnImpactSparks(Vec2{b.pos.x, roomBounds.yMin}, Vec2{0.0, 1.0},
                                  0.5 * b.mass * impactSpeed * impactSpeed, b.color, RAYWHITE, 12);
            }
        }

        // 3. Left Wall collision (xMin)
        if (b.pos.x - b.radius <= roomBounds.xMin) {
            double impactSpeed = std::abs(b.vel.x);
            if (impactSpeed >= roomBounds.shatterVelocity) {
                bodiesToShatter.push_back(b.id);
                continue;
            }
            b.pos.x = roomBounds.xMin + b.radius;
            b.vel.x = -b.vel.x * roomBounds.wallRestitution;
            b.vel.y *= 0.98;
            if (impactSpeed > 30.0) {
                spawnImpactSparks(Vec2{roomBounds.xMin, b.pos.y}, Vec2{1.0, 0.0},
                                  0.5 * b.mass * impactSpeed * impactSpeed, b.color, RAYWHITE, 15);
            }
        }

        // 4. Right Wall collision (xMax)
        if (b.pos.x + b.radius >= roomBounds.xMax) {
            double impactSpeed = std::abs(b.vel.x);
            if (impactSpeed >= roomBounds.shatterVelocity) {
                bodiesToShatter.push_back(b.id);
                continue;
            }
            b.pos.x = roomBounds.xMax - b.radius;
            b.vel.x = -b.vel.x * roomBounds.wallRestitution;
            b.vel.y *= 0.98;
            if (impactSpeed > 30.0) {
                spawnImpactSparks(Vec2{roomBounds.xMax, b.pos.y}, Vec2{-1.0, 0.0},
                                  0.5 * b.mass * impactSpeed * impactSpeed, b.color, RAYWHITE, 15);
            }
        }
    }

    // Trigger explosive particle destruction on bodies exceeding shatter speed
    for (int id : bodiesToShatter) {
        triggerDestructionExplosion(id, 140);
    }
}

double GravityEngine::computeKineticEnergy() const {
    double totalK = 0.0;
    for (const auto& body : bodies) {
        if (!body.isStatic) {
            totalK += 0.5 * body.mass * body.vel.lengthSq();
        }
    }
    return totalK;
}

double GravityEngine::computePotentialEnergy() const {
    double totalU = 0.0;

    // Potential energy under uniform downward gravity field: U = sum(m * g * h) where h is height above floor
    if (enableUniformGravity) {
        for (const auto& body : bodies) {
            if (!body.isStatic) {
                double h = roomBounds.yMax - body.pos.y;
                totalU += body.mass * uniformGravity.y * h;
            }
        }
    }

    if (!enableUniformGravity || !disableMutualGravityInRoom) {
        const size_t n = bodies.size();
        const double epsSq = epsilon * epsilon;

        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                double distSq = bodies[i].pos.distanceSq(bodies[j].pos);
                double softenedDist = std::sqrt(distSq + epsSq);
                if (softenedDist > 1e-12) {
                    totalU -= (G * bodies[i].mass * bodies[j].mass) / softenedDist;
                }
            }
        }
    }
    return totalU;
}

double GravityEngine::computeTotalEnergy() const {
    return computeKineticEnergy() + computePotentialEnergy();
}

double GravityEngine::getEnergyDriftPercent() const {
    if (!initialEnergyValid || std::abs(initialEnergy) < 1e-12) {
        return 0.0;
    }
    double currentE = computeTotalEnergy();
    return (std::abs(currentE - initialEnergy) / std::abs(initialEnergy)) * 100.0;
}

void GravityEngine::resetInitialEnergy() {
    initialEnergy = computeTotalEnergy();
    initialEnergyValid = true;
}

std::vector<Vec2> GravityEngine::predictTrajectory(
    Vec2 startPos, Vec2 startVel, double mass, int steps, double dt) const {
    (void)mass;
    std::vector<Vec2> points;
    points.reserve(steps);

    Vec2 p = startPos;
    Vec2 v = startVel;
    const double epsSq = epsilon * epsilon;

    for (int step = 0; step < steps; ++step) {
        points.push_back(p);

        Vec2 acc = enableUniformGravity ? uniformGravity : Vec2{0.0, 0.0};

        if (!enableUniformGravity || !disableMutualGravityInRoom) {
            for (const auto& body : bodies) {
                Vec2 r_vec = body.pos - p;
                double distSq = r_vec.lengthSq();
                double softenedDist = std::sqrt(distSq + epsSq);
                double denom = (distSq + epsSq) * softenedDist;

                if (denom > 1e-12) {
                    acc += r_vec * (G * body.mass / denom);
                }

                // If close enough to hit body, terminate trajectory
                if (distSq < (body.radius * body.radius)) {
                    return points;
                }
            }
        }

        // In Earth room, terminate trajectory when hitting floor or walls
        if (roomBounds.enabled) {
            if (p.y >= roomBounds.yMax || p.y <= roomBounds.yMin ||
                p.x <= roomBounds.xMin || p.x >= roomBounds.xMax) {
                return points;
            }
        }

        v += acc * dt;
        p += v * dt;
    }

    return points;
}

void GravityEngine::addBody(const Body& b) {
    bodies.push_back(b);
    computeAccelerations();
}

void GravityEngine::clearBodies() {
    bodies.clear();
    initialEnergy = 0.0;
    initialEnergyValid = false;
}

void GravityEngine::loadSolarSystem() {
    clearBodies();
    roomBounds.enabled = false;
    enableUniformGravity = false;
    G = 1.0;
    epsilon = 5.0;

    int nextId = 1;

    // Sun (Central anchor)
    bodies.emplace_back(nextId++, "Sun", 200000.0, 24.0,
                        Vec2{0.0, 0.0}, Vec2{0.0, 0.0},
                        Color{255, 210, 30, 255}, true);

    // Helper for circular orbit velocity: v = sqrt(G * M_sun / r)
    auto makeOrbitingBody = [&](const std::string& name, double dist, double mass,
                                double radius, Color color, size_t trailPts = 350) -> Body {
        double v = std::sqrt(G * 200000.0 / dist);
        Body b(nextId++, name, mass, radius, Vec2{dist, 0.0}, Vec2{0.0, v}, color, false);
        b.maxTrailPoints = trailPts;
        return b;
    };

    // 1. Mercury
    bodies.push_back(makeOrbitingBody("Mercury", 80.0, 1.2, 3.5, Color{190, 180, 170, 255}, 250));

    // 2. Venus
    bodies.push_back(makeOrbitingBody("Venus", 130.0, 4.8, 5.5, Color{235, 195, 140, 255}, 300));

    // 3. Earth
    Body earth = makeOrbitingBody("Earth", 200.0, 160.0, 6.5, Color{70, 150, 255, 255}, 350);
    bodies.push_back(earth);

    // Earth's Moon
    double moonDist = 13.0;
    double moonOrbVel = std::sqrt(G * earth.mass / moonDist);
    Body moon(nextId++, "Moon", 0.3, 2.0,
              Vec2{earth.pos.x + moonDist, 0.0},
              Vec2{0.0, earth.vel.y + moonOrbVel},
              Color{210, 215, 220, 255}, false);
    moon.maxTrailPoints = 200;
    bodies.push_back(moon);

    // 4. Mars
    Body mars = makeOrbitingBody("Mars", 280.0, 12.0, 5.0, Color{240, 100, 60, 255}, 350);
    bodies.push_back(mars);

    // Mars Moon (Phobos)
    double phobosDist = 9.0;
    double phobosVel = std::sqrt(G * mars.mass / phobosDist);
    Body phobos(nextId++, "Phobos", 0.05, 1.6,
                Vec2{mars.pos.x + phobosDist, 0.0},
                Vec2{0.0, mars.vel.y + phobosVel},
                Color{180, 160, 150, 255}, false);
    phobos.maxTrailPoints = 120;
    bodies.push_back(phobos);

    // 5. Jupiter
    Body jupiter = makeOrbitingBody("Jupiter", 420.0, 650.0, 13.0, Color{225, 175, 120, 255}, 400);
    bodies.push_back(jupiter);

    // Jupiter's Galilean Moons
    double ioDist = 20.0;
    double ioVel = std::sqrt(G * jupiter.mass / ioDist);
    Body io(nextId++, "Io", 0.3, 2.2,
            Vec2{jupiter.pos.x + ioDist, 0.0},
            Vec2{0.0, jupiter.vel.y + ioVel},
            Color{255, 235, 90, 255}, false);
    io.maxTrailPoints = 180;
    bodies.push_back(io);

    double europaDist = 30.0;
    double europaVel = -std::sqrt(G * jupiter.mass / europaDist);
    Body europa(nextId++, "Europa", 0.25, 2.0,
                Vec2{jupiter.pos.x - europaDist, 0.0},
                Vec2{0.0, jupiter.vel.y + europaVel},
                Color{200, 230, 255, 255}, false);
    europa.maxTrailPoints = 180;
    bodies.push_back(europa);

    // 6. Saturn
    bodies.push_back(makeOrbitingBody("Saturn", 580.0, 300.0, 11.0, Color{240, 220, 160, 255}, 450));

    // 7. Halley-style eccentric comet swooping past the Sun
    double perihelion = 55.0;
    double aphelion = 720.0;
    double semiMajor = (perihelion + aphelion) * 0.5;
    double cometVel = std::sqrt(G * 200000.0 * (2.0 / perihelion - 1.0 / semiMajor));
    Body comet(nextId++, "Comet Halley", 0.1, 2.5,
               Vec2{perihelion, 0.0}, Vec2{0.0, cometVel},
               Color{130, 245, 255, 255}, false);
    comet.maxTrailPoints = 500;
    bodies.push_back(comet);

    computeAccelerations();
    resetInitialEnergy();
}

void GravityEngine::loadFigureEight() {
    clearBodies();
    roomBounds.enabled = false;
    enableUniformGravity = false;
    G = 1.0;
    epsilon = 0.01;

    // Chenciner & Montgomery exact choreographed 3-body solution
    // Scaled by spatial factor L = 260.0
    const double L = 260.0;
    const double mass = 500.0;
    const double V = std::sqrt(G * mass / L);

    const double x1 = -0.97000436;
    const double y1 = -0.24308753;
    const double vx1 = 0.46620531;
    const double vy1 = 0.43236573;

    // Body 1
    Body b1(1, "Alpha", mass, 8.5,
            Vec2{x1 * L, y1 * L},
            Vec2{vx1 * V, vy1 * V},
            Color{0, 245, 255, 255}, false);
    b1.maxTrailPoints = 600;

    // Body 2 (center)
    Body b2(2, "Beta", mass, 8.5,
            Vec2{0.0, 0.0},
            Vec2{-2.0 * vx1 * V, -2.0 * vy1 * V},
            Color{255, 40, 160, 255}, false);
    b2.maxTrailPoints = 600;

    // Body 3
    Body b3(3, "Gamma", mass, 8.5,
            Vec2{-x1 * L, -y1 * L},
            Vec2{vx1 * V, vy1 * V},
            Color{255, 240, 30, 255}, false);
    b3.maxTrailPoints = 600;

    bodies.push_back(b1);
    bodies.push_back(b2);
    bodies.push_back(b3);

    computeAccelerations();
    resetInitialEnergy();
}

void GravityEngine::loadBinaryDisk() {
    clearBodies();
    roomBounds.enabled = false;
    enableUniformGravity = false;
    G = 1.0;
    epsilon = 10.0;

    int nextId = 1;
    const double starMass = 45000.0;
    const double sep = 180.0;
    const double halfSep = sep * 0.5;

    // Mutual circular orbital speed: v = sqrt(G * M / (4 * R)) where R = sep / 2
    const double starSpeed = std::sqrt(G * starMass / (4.0 * halfSep));

    // Star A (Orange / Red Giant)
    Body starA(nextId++, "Star A", starMass, 16.0,
               Vec2{-halfSep, 0.0}, Vec2{0.0, -starSpeed},
               Color{255, 110, 40, 255}, false);
    starA.maxTrailPoints = 300;
    bodies.push_back(starA);

    // Star B (Cyan / Blue Giant)
    Body starB(nextId++, "Star B", starMass, 14.0,
               Vec2{halfSep, 0.0}, Vec2{0.0, starSpeed},
               Color{80, 190, 255, 255}, false);
    starB.maxTrailPoints = 300;
    bodies.push_back(starB);

    // Protoplanetary disk: 220 lightweight debris particles
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> distU(0.0, 1.0);
    std::uniform_real_distribution<double> distAngle(0.0, 2.0 * 3.141592653589793);
    std::uniform_real_distribution<double> distMass(0.05, 0.25);

    const double rMin = 180.0;
    const double rMax = 620.0;
    const int particleCount = 220;

    for (int i = 0; i < particleCount; ++i) {
        double u = distU(rng);
        // Area-weighted distribution: r = sqrt(u * (rMax^2 - rMin^2) + rMin^2)
        double r = std::sqrt(u * (rMax * rMax - rMin * rMin) + rMin * rMin);
        double theta = distAngle(rng);

        Vec2 pos{r * std::cos(theta), r * std::sin(theta)};

        // Keplerian orbital velocity around combined barycentric mass (2 * starMass)
        double speed = std::sqrt(G * (2.0 * starMass) / r);
        Vec2 vel{-speed * std::sin(theta), speed * std::cos(theta)};

        // Color variation based on distance
        double t = (r - rMin) / (rMax - rMin);
        unsigned char red = static_cast<unsigned char>(255 * (1.0 - 0.6 * t));
        unsigned char green = static_cast<unsigned char>(200 + 40 * t);
        unsigned char blue = static_cast<unsigned char>(120 + 130 * t);

        Body p(nextId++, "Particle", distMass(rng), 2.0, pos, vel,
               Color{red, green, blue, 220}, false);
        p.maxTrailPoints = 35;
        bodies.push_back(p);
    }

    computeAccelerations();
    resetInitialEnergy();
}

void GravityEngine::loadEarthRoom() {
    clearBodies();
    G = 1.0;
    epsilon = 5.0;
    integrator = IntegratorType::VelocityVerlet;
    collisionMode = CollisionMode::Elastic; // Elastic bounces inside room

    roomBounds.enabled = true;
    roomBounds.xMin = -460.0;
    roomBounds.xMax = 460.0;
    roomBounds.yMin = -340.0; // Ceiling
    roomBounds.yMax = 290.0;  // Floor
    roomBounds.floorRestitution = 0.82;
    roomBounds.wallRestitution = 0.85;
    roomBounds.shatterVelocity = 680.0; // Fall from height > 320px hits with > 680 px/s and explodes

    enableUniformGravity = true;
    uniformGravity = Vec2{0.0, 720.0}; // Downward uniform acceleration field g (Galileo: a = g)
    disableMutualGravityInRoom = true;

    int nextId = 1;

    // 1. Galileo Falling Bodies Pair:
    // Both dropped side-by-side from high altitude (y = -220, height = 510px).
    // Constant downward acceleration a = g = 720 px/s^2 (Galileo's Equivalence: independent of mass).
    // Terminal impact velocity v = sqrt(2 * 720 * 510) = 857 px/s > 680 px/s (Shatter Threshold)
    // -> BOTH EXPLODE INTO DEBRIS PARTICLES ON THE FLOOR!
    Body anvil(nextId++, "Heavy Anvil (m=500)", 500.0, 18.0,
               Vec2{-180.0, -220.0}, Vec2{0.0, 0.0},
               Color{240, 175, 45, 255}, false);
    anvil.maxTrailPoints = 300;
    bodies.push_back(anvil);

    Body apple(nextId++, "Light Apple (m=0.5)", 0.5, 8.0,
               Vec2{-100.0, -220.0}, Vec2{0.0, 0.0},
               Color{255, 60, 60, 255}, false);
    apple.maxTrailPoints = 300;
    bodies.push_back(apple);

    // 2. Safe Bouncing Rubber Ball:
    // Released from low altitude (y = 130, height = 160px).
    // Terminal velocity v = sqrt(2 * 720 * 160) = 480 px/s < 680 px/s -> BOUNCES SAFELY!
    Body rubberBall(nextId++, "Rubber Ball (m=2)", 2.0, 11.0,
                    Vec2{90.0, 130.0}, Vec2{0.0, 0.0},
                    Color{0, 235, 255, 255}, false);
    rubberBall.maxTrailPoints = 300;
    bodies.push_back(rubberBall);

    // 3. Lateral Projectile Tennis Ball (Demonstrates wall and floor bouncing):
    Body tennisBall(nextId++, "Tennis Ball (m=1)", 1.0, 9.0,
                    Vec2{0.0, 90.0}, Vec2{140.0, -60.0},
                    Color{180, 255, 40, 255}, false);
    tennisBall.maxTrailPoints = 350;
    bodies.push_back(tennisBall);

    // 4. Heavy Bowling Ball (Heavy mass, low drop height: y = 110, height = 180px -> Bounces!):
    Body bowlingBall(nextId++, "Bowling Ball (m=40)", 40.0, 15.0,
                     Vec2{220.0, 110.0}, Vec2{0.0, 0.0},
                     Color{130, 150, 190, 255}, false);
    bowlingBall.maxTrailPoints = 250;
    bodies.push_back(bowlingBall);

    computeAccelerations();
    resetInitialEnergy();
}

void GravityEngine::loadTwoBodyOrbitTest() {
    clearBodies();
    roomBounds.enabled = false;
    enableUniformGravity = false;
    G = 1.0;
    epsilon = 0.0;

    // Body 1: Central body
    Body star(1, "Central", 10000.0, 12.0,
              Vec2{0.0, 0.0}, Vec2{0.0, 0.0},
              Color{255, 220, 0, 255}, true);

    // Body 2: Circular orbit at r = 200.0
    // v = sqrt(G * M / r) = sqrt(10000 / 200) = sqrt(50) = 7.071067811865475
    double r = 200.0;
    double v = std::sqrt(G * star.mass / r);
    Body planet(2, "TestPlanet", 1.0, 4.0,
                Vec2{r, 0.0}, Vec2{0.0, v},
                Color{0, 200, 255, 255}, false);

    bodies.push_back(star);
    bodies.push_back(planet);

    computeAccelerations();
    resetInitialEnergy();
}

const char* GravityEngine::getIntegratorName() const {
    switch (integrator) {
        case IntegratorType::VelocityVerlet: return "Velocity Verlet";
        case IntegratorType::SymplecticEuler: return "Symplectic Euler";
        default: return "Unknown";
    }
}

const char* GravityEngine::getCollisionModeName() const {
    switch (collisionMode) {
        case CollisionMode::None: return "None (Ghost)";
        case CollisionMode::Elastic: return "Elastic (Bounce)";
        case CollisionMode::InelasticMerge: return "Inelastic (Merge)";
        case CollisionMode::DestructiveShatter: return "Destruction (Shatter)";
        default: return "Unknown";
    }
}

Body* GravityEngine::getBodyById(int id) {
    for (auto& b : bodies) {
        if (b.id == id) return &b;
    }
    return nullptr;
}

const Body* GravityEngine::getBodyById(int id) const {
    for (const auto& b : bodies) {
        if (b.id == id) return &b;
    }
    return nullptr;
}

int GravityEngine::findNearestBody(Vec2 worldPos, double maxDist) const {
    int bestId = -1;
    double bestDistSq = maxDist * maxDist;

    for (const auto& b : bodies) {
        double dSq = b.pos.distanceSq(worldPos);
        double effectiveThreshold = std::max(b.radius + 8.0, 16.0);
        double thresholdSq = effectiveThreshold * effectiveThreshold;

        if (dSq < thresholdSq && dSq < bestDistSq) {
            bestDistSq = dSq;
            bestId = b.id;
        }
    }
    return bestId;
}

bool GravityEngine::deleteBody(int id) {
    for (auto it = bodies.begin(); it != bodies.end(); ++it) {
        if (it->id == id) {
            bodies.erase(it);
            computeAccelerations();
            return true;
        }
    }
    return false;
}

Vec2 GravityEngine::computeNetAccelerationOnPoint(Vec2 pos) const {
    Vec2 netAcc = enableUniformGravity ? uniformGravity : Vec2{0.0, 0.0};
    if (!enableUniformGravity || !disableMutualGravityInRoom) {
        const double epsSq = epsilon * epsilon;

        for (const auto& b : bodies) {
            Vec2 r_ij = b.pos - pos;
            double distSq = r_ij.lengthSq();
            double softenedDist = std::sqrt(distSq + epsSq);
            double denom = (distSq + epsSq) * softenedDist;

            if (denom > 1e-12) {
                netAcc += r_ij * (G * b.mass / denom);
            }
        }
    }
    return netAcc;
}

Vec2 GravityEngine::computeCircularOrbitVelocity(Vec2 spawnPos) const {
    if (bodies.empty()) return Vec2{0.0, 0.0};

    const Body* dominant = nullptr;
    double maxPull = -1.0;

    for (const auto& b : bodies) {
        double d = b.pos.distance(spawnPos);
        if (d > 1e-3) {
            double pull = (G * b.mass) / (d * d);
            if (pull > maxPull) {
                maxPull = pull;
                dominant = &b;
            }
        }
    }

    if (!dominant) return Vec2{0.0, 0.0};

    Vec2 delta = spawnPos - dominant->pos;
    double r = delta.length();
    if (r < 1e-3) return Vec2{0.0, 0.0};

    double vOrb = std::sqrt(G * dominant->mass / r);
    Vec2 perp{-delta.y / r, delta.x / r};

    return dominant->vel + (perp * vOrb);
}

void GravityEngine::circularizeOrbit(int bodyId) {
    Body* body = getBodyById(bodyId);
    if (!body || body->isStatic) return;

    const Body* dominant = nullptr;
    double maxPull = -1.0;

    for (const auto& b : bodies) {
        if (b.id == bodyId) continue;
        double d = b.pos.distance(body->pos);
        if (d > 1e-3) {
            double pull = (G * b.mass) / (d * d);
            if (pull > maxPull) {
                maxPull = pull;
                dominant = &b;
            }
        }
    }

    if (!dominant) return;

    Vec2 delta = body->pos - dominant->pos;
    double r = delta.length();
    if (r < 1e-3) return;

    double vOrb = std::sqrt(G * dominant->mass / r);
    Vec2 perp{-delta.y / r, delta.x / r};

    body->vel = dominant->vel + (perp * vOrb);
}
