# 🌌 2D N-Body Gravity Engine (C++20 & Raylib)

A high-performance, modular 2D N-Body celestial mechanics and gravity simulation engine built with modern C++ (C++20) and Raylib. Features symplectic numerical integrators, momentum-conserving inelastic and elastic collision physics, orbital trail rendering, interactive slingshot trajectory prediction, an object inspector with real-time property editing, and a custom launcher settings menu.

---

## 🚀 Features

- **Physics Solvers:**
  - **Velocity Verlet:** Symplectic 2nd-order time-reversible integrator preserving mechanical energy and orbital phase space over prolonged simulations.
  - **Symplectic Euler:** 1st-order symplectic integrator available for live comparison.
  - **Newtonian Gravity with Plummer Softening:**
    $$F = \frac{G \cdot m_1 \cdot m_2}{(r^2 + \epsilon^2)}$$
    Prevents numerical singularity explosions during hyper-close encounters.
- **Collision Dynamics & Destruction Physics:**
  - **Cataclysmic Destruction & Shattering:** High-energy impacts trigger explosive fragmentation where celestial bodies break apart into dynamic child debris fragments while strictly conserving linear momentum and mass.
  - **Destruction Particle System:** Over 150+ glowing incandescent embers, molten sparks, and expanding shockwave rings with dynamic camera shake upon collision or detonation.
  - **Inelastic Merging:** Total linear momentum conservation ($m_1 \vec{v}_1 + m_2 \vec{v}_2 = (m_1 + m_2)\vec{v}_{new}$), 2D disk area conservation ($R = \sqrt{r_1^2 + r_2^2}$), and collision sparks.
  - **Elastic Bouncing:** Momentum and kinetic energy preserving impulse resolution with contact sparks.
  - **Ghost Mode:** Uncollided pass-through.
- **High-DPI Anti-Aliased Typography:**
  - Modern Segoe UI / Arial TrueType vector typography with bilinear texture filtering for crystal-clear readability across all resolutions and display scaling factors.
- **Object Inspector & Real-Time Property Modifiers:**
  - **Full-Height Docked Sidebar:** Enlarged 480px–560px full-height sidebar with spacious card layouts utilizing the entire screen height.
  - **Hide / Show Option:** Collapse the sidebar anytime with the **`[HIDE >>]`** button or **`H`** key; a glowing **`[<< SHOW SIDEBAR]`** pill tab remains on the right screen edge for instant 1-click re-opening.
  - **Click any body** in space to inspect and lock onto it with a futuristic targeting reticle.
  - **Live Diagnostics:** Surface gravity $g = \frac{G m}{r^2 + \epsilon^2}$, escape velocity $v_{esc} = \sqrt{\frac{2 G m}{r}}$, net gravitational pull $|\vec{a}|$, instantaneous speed $|\vec{v}|$, density $\rho = \frac{m}{\pi r^2}$, coordinates $(X,Y)$, and velocity $(V_x, V_y)$.
  - **Live Adjustments:** Real-time modification of Mass (`-50%` to `+50%`, `x10`, `/10`), Radius, and Density with stepper buttons.
  - **Action Tools:** Lock Camera Track, Toggle Static Anchor, Halt Velocity ($v=0$), Auto Circularize Orbit around dominant attractor, 8-color palette picker, Clear Trail, and Cataclysmic Object Explosion (`K`).
- **Custom Launcher / Spawner Settings Tab (`TAB`):**
  - **Presets:** Quickly switch launch payload between *Asteroid*, *Moon*, *Terrestrial Planet*, *Gas Giant*, *Star*, or *Singularity (Black Hole)*.
  - **Spawn Modes:**
    - *Slingshot (Drag):* Pull back to set direction and velocity with real-time trajectory prediction.
    - *Auto Circular Orbit:* Click anywhere in space to automatically calculate and inject the body into a stable circular orbit around the nearest massive body!
    - *Stationary:* Place stationary floating masses or anchored static stars.
  - **Customizable Launch Properties:** Modify launch mass, radius, density, color palette, anchor state, and trajectory prediction steps.
- **Decoupled Fixed-Timestep Simulation:**
  - Fixed physics step ($\Delta t = 0.002\,\text{s}$) with an accumulator sub-stepping loop decoupled from monitor refresh rates.
  - Variable time-dilation scaling ($0.25\times$ to $10.0\times$).
- **Orbital Trail & Visual Diagnostics:**
  - Fading historical orbital trajectories and velocity vector arrows.
  - Real-time Sci-Fi HUD monitoring FPS, active integrator, collision mode, total mechanical energy ($E = K + U$), and drift percentage.
- **Preconfigured Presets:**
  1. **Solar System:** Central star, Mercury, Venus, Earth + Moon system, Mars + Phobos, Jupiter + Galilean moons (Io & Europa), Saturn, and a high-eccentricity Halley-like comet.
  2. **Figure-8 Three-Body Orbit:** The celebrated Chenciner & Montgomery choreographed stable solution.
  3. **Binary Star System & Protoplanetary Disk:** Two massive co-orbiting stars surrounded by an accretion disk of 220+ debris particles.
  4. **Earth Gravity Laboratory Room:** A physical containment chamber on Earth subject to a uniform downward acceleration field $\vec{g} = (0, 9.81\ \text{m/s}^2)$. Demonstrates Galileo's principle where all objects (e.g. a 500 kg anvil and a 0.5 kg apple) fall at the exact same rate independent of mass. Objects bounce elastically with restitution, and when dropped from high elevation exceeding the critical shatter threshold ($h > 320\ \text{px}$, $v > 680\ \text{px/s}$), they explode violently into debris fragments, fire sparks, and shockwaves upon floor impact.

---

## 🛠️ Project Structure

```
├── CMakeLists.txt              # Build configuration (local raylib or FetchContent)
├── include/
│   ├── Vec2.hpp                # Lightweight 2D double-precision vector math
│   ├── Body.hpp                # Celestial body struct & trail history
│   ├── LaunchSettings.hpp      # Spawner configuration, presets & modes
│   ├── GravityEngine.hpp       # Physics solver, celestial presets & querying
│   └── Renderer.hpp            # Raylib camera, trails, bodies, HUD & UI panels
├── src/
│   ├── GravityEngine.cpp       # Verlet/Euler integration, collision & energy math
│   ├── Renderer.cpp            # Visual rendering & interactive GUI panels
│   └── main.cpp                # App loop, accumulator, input routing & headless CLI test
└── README.md
```

---

## ⌨️ Controls & Keybindings

| Key / Input | Action |
|---|---|
| `Left Click Body` | Select and open **Object Inspector** in sidebar |
| `H` | Hide / Show **Right Sidebar** |
| `TAB` | Toggle between **Inspector** and **Spawner** tabs |
| `ESC` | Deselect body or hide sidebar |
| `SPACE` | Pause / Resume simulation |
| `1` | Load **Solar System** Preset |
| `2` | Load **Figure-8 Three-Body** Preset |
| `3` | Load **Binary Star + Protoplanetary Disk** Preset |
| `4` | Load **Earth Room Laboratory** (Uniform Downward $g$, Galileo Fall, Bounce & Height Shatter) |
| `I` | Toggle Integrator (**Velocity Verlet** $\leftrightarrow$ **Symplectic Euler**) |
| `C` | Cycle Collision Mode (**Inelastic Merge** $\rightarrow$ **Destruction Shatter** $\rightarrow$ **Elastic** $\rightarrow$ **Ghost**) |
| `K` | Cataclysmic Destruction of selected body (Explosion + Particles + Shockwave) |
| `T` | Toggle Orbital Trails |
| `V` | Toggle Velocity Vectors |
| `G` | Toggle Coordinate Grid & Distance Rings |
| `F` | Toggle Follow Heaviest Body |
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
