# 🌊 Fluid Simulation using Smoothed Particle Hydrodynamics (SPH) 🌊

A real-time 2D fluid dynamics simulator implemented in C using SDL2, based on the Smoothed Particle Hydrodynamics method. This project features interactive controls, real-time parameter adjustment, and visual feedback for understanding fluid behavior.

![Fluid Simulation Demo](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![SDL2](https://img.shields.io/badge/SDL2-Graphics-green?style=for-the-badge)

## 🌊 Features

### Physics Simulation
- **Smoothed Particle Hydrodynamics (SPH)** algorithm for realistic fluid dynamics
- **Pressure calculation** using density-based kernel functions
- **Viscosity forces** for smooth fluid motion
- **Collision detection** with boundary handling
- **Spatial hashing** for optimized neighbor particle lookup

### Visual Features
- **Particle visualization** with velocity-based color gradient (blue → red)
- **Water surface rendering** with smooth Catmull-Rom spline interpolation
- **Gradient depth shading** for realistic water appearance
- **Grid overlay** showing spatial partitioning cells
- **Density visualization** mode for debugging

### Interactive Controls
- **Mouse interaction**: Apply attractive/repulsive forces to particles
- **Real-time parameter adjustment** via interactive sliders
- **Pause/Resume** simulation
- **Frame-by-frame stepping** for detailed analysis
- **Dark/Light mode** toggle

## 🎮 Controls

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `SPACE` | Pause/Resume simulation |
| `F5` | Restart simulation |
| `F11` | Toggle fullscreen |
| `→` | Step forward (10 frames) |
| `←` | Step backward (10 frames) |
| `I` | Toggle dark/white mode |
| `R` | Reset all parameters to defaults |
| `S` | Toggle particle visibility |
| `C` | Switch interaction mode |
| `G` | Trigger right shift animation |
| `U` | Trigger upward shift animation |
| `H` | Show/hide help menu |

### Mouse Controls

**Interaction Mode (C key to toggle):**
- **Left Click + Drag**: Apply repulsive force
- **Right Click + Drag**: Apply attractive force
- **Click on sliders**: Adjust simulation parameters in real-time

**Examination Mode:**
- **Left Click**: Display density at clicked location
- **Right Click**: Visualize density field

## 🔧 Adjustable Parameters

The right panel features interactive sliders for:

- **FPS**: Simulation frame rate (1-120)
- **h (Smoothing Radius)**: Kernel influence radius (1-100)
- **m (Mass)**: Particle mass (1-11)
- **t_dens (Target Density)**: Desired fluid density (0-0.01)
- **p (Pressure Multiplier)**: Pressure force strength (0-100000)
- **visco (Viscosity)**: Fluid viscosity coefficient (0-10)

## 📊 Statistics Display

The interface shows real-time statistics:
- **FPS**: Current frame rate
- **Operations per frame**: Computational complexity indicator
- **Elapsed time**: Simulation runtime
- **Particle count**: Number of particles in simulation bounds
- **Complexity**: Big-O notation estimate (O(n), O(n log n), O(n²))

## 🛠️ Installation

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install libsdl2-dev libsdl2-ttf-dev

# macOS
brew install sdl2 sdl2_ttf

### Building

```bash
# Clone the repository
git clone https://github.com/yourusername/fluid-simulation-sph.git
cd fluid-simulation-sph

# Compile
gcc TIPE.c -lSDL2 -lSDL2_ttf -lm -Wno-format -Wall -Werror -Wpedantic -o fluid_sim

# Run
./fluid_sim
```



## 🎨 Visualization Modes

### Particle Mode (default)
Individual particles colored by velocity magnitude:
- **Blue**: Slow/stationary
- **Purple/Red**: Fast-moving

### Water Surface Mode (press S)
Smooth surface rendering with:
- Catmull-Rom spline interpolation
- Depth-based gradient shading
- Surface normal detection




## 📄 License

This project is open source and available under the MIT License.

## 🙏 Acknowledgments

Based on the SPH algorithm described in:
- Müller et al., "Particle-Based Fluid Simulation for Interactive Applications" (SCA 2003)
- Smoothing kernel implementations from Matthias Müller's research

## 📸 Screenshots

### Main Simulation View
![Simulation View](assets/main_view.png)
*Particles colored by velocity with spatial grid overlay*

### Surface Rendering Mode
![Surface Mode](assets/surface_mode.png)
*Smooth water surface with depth-gradient shading*

### Density Visualization
![Density View](assets/density_view.png)
*Heat map showing pressure distribution*

### Interactive Parameter Control
![Controls](assets/controls.png)
*Real-time parameter adjustment sliders*

## 🎥 Video Demonstration

[![Demo Video](assets/video_thumbnail.png)](assets/demo_video.mp4)
