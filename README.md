# 🌌 2D N-Body Gravity Engine (C++20 & Raylib)

A high-performance, modular 2D N-Body celestial mechanics and gravity simulation engine built with modern C++ (C++20) and Raylib. Features symplectic numerical integrators, momentum-conserving inelastic and elastic collision physics, orbital trail rendering, interactive slingshot trajectory prediction, and real-time energy conservation diagnostics.

---

## 🚀 Features

- **Physics Solvers:**
  - **Velocity Verlet:** Symplectic 2nd-order time-reversible integrator preserving mechanical energy and orbital phase space over prolonged simulations.
  - **Symplectic Euler:** 1st-order symplectic integrator available for live comparison.
  - **Newtonian Gravity with Plummer Softening:**
    $$F = \frac{G \cdot m_1 \cdot m_2}{(r^2 + \epsilon^2)}$$
    Prevents numerical singularity explosions during hyper-close encounters.
- **Collision Dynamics:**
  - **Inelastic Merging:** Total linear momentum conservation ($m_1 \vec{v}_1 + m_2 \vec{v}_2 = (m_1 + m_2)\vec{v}_{new}$), 2D disk volume conservation ($R = \sqrt{r_1^2 + r_2^2}$), mass-weighted barycentric positioning and color blending.
  - **Elastic Bouncing:** Momentum and kinetic energy preserving impulse resolution with overlap correction.
  - **Ghost Mode:** Uncollided pass-through.
- **Decoupled Fixed-Timestep Simulation:**
  - Fixed physics step ($\Delta t = 0.002\,\text{s}$) with an accumulator sub-stepping loop decoupled from monitor refresh rates.
  - Variable time-dilation scaling ($0.25\times$ to $10.0\times$).
- **Orbital Trail & Visual Diagnostics:**
  - Fading historical orbital trajectories.
  - Velocity vector arrows indicating instantaneous momentum.
  - Real-time Sci-Fi HUD monitoring FPS, active integrator, collision mode, total mechanical energy ($E = K + U$), and drift percentage.
- **Interactive Slingshot Spawner:**
  - Left-click and drag in space to pull back a slingshot vector.
  - Real-time numerical trajectory prediction arc previewing the body's future orbital path before release.
  - Mass adjustment via mouse wheel or `+`/`-` keys.
- **Preconfigured Presets:**
  1. **Solar System:** Central star, Mercury, Venus, Earth + Moon system, Mars + Phobos, Jupiter + Galilean moons (Io & Europa), Saturn, and a high-eccentricity Halley-like comet.
  2. **Figure-8 Three-Body Orbit:** The celebrated Chenciner & Montgomery choreographed stable solution.
  3. **Binary Star System & Protoplanetary Disk:** Two massive co-orbiting stars surrounded by an accretion disk of 220+ debris particles.

---

## 🛠️ Project Structure

```
├── CMakeLists.txt              # Build configuration (local raylib or FetchContent)
├── include/
│   ├── Vec2.hpp                # Lightweight 2D double-precision vector math
│   ├── Body.hpp                # Celestial body struct & trail history
│   ├── GravityEngine.hpp       # Physics solver & celestial presets
│   └── Renderer.hpp            # Raylib camera, trails, bodies & HUD renderer
├── src/
│   ├── GravityEngine.cpp       # Verlet/Euler integration, collision & energy math
│   ├── Renderer.cpp            # Visual rendering implementation
│   └── main.cpp                # App loop, accumulator, input & headless CLI test
└── README.md
```

---

## ⌨️ Controls & Keybindings

| Key / Input | Action |
|---|---|
| `SPACE` | Pause / Resume simulation |
| `1` | Load **Solar System** Preset |
| `2` | Load **Figure-8 Three-Body** Preset |
| `3` | Load **Binary Star + Protoplanetary Disk** Preset |
| `I` | Toggle Integrator (**Velocity Verlet** $\leftrightarrow$ **Symplectic Euler**) |
| `C` | Cycle Collision Mode (**Inelastic Merge** $\rightarrow$ **Elastic** $\rightarrow$ **Ghost**) |
| `T` | Toggle Orbital Trails |
| `V` | Toggle Velocity Vectors |
| `G` | Toggle Coordinate Grid & Distance Rings |
| `F` | Toggle Follow Heaviest Body (Auto-centering camera) |
| `R` | Reset Camera Position and Zoom |
| `X` | Clear all bodies from space |
| `[` / `]` or `,` / `.` | Adjust Time Scale ($0.25\times$ - $10\times$) |
| `+` / `-` | Increase / Decrease Spawning Mass |
| `Left Click + Drag` | Slingshot spawn a new body (with trajectory preview) |
| `Right Click + Drag` | Pan camera across space |
| `WASD` or Arrow Keys | Pan camera |
| `Mouse Wheel` | Smooth zoom towards mouse cursor |

---

## 🔨 Building and Running

### Prerequisites
- A C++20 compliant compiler (GCC 11+, Clang 13+, or MSVC 2019+).
- CMake 3.20+.
- Raylib 5.0+ (CMake will automatically pull Raylib via `FetchContent` if not found locally).

### Build Commands
```bash
cmake -B build -G "Ninja"
cmake --build build
```

### Running the Graphical Application
```bash
./build/GravityEngine.exe
```

### Running the Headless Unit Test
```bash
./build/GravityEngine.exe --test
```
Asserts that total mechanical energy drift over 1,000 steps of a two-body orbit is $< 0.01\%$.
