## Linux CS2 External

WH memory patch for CS2 on Linux.

![Menu](https://github.com/16Artem/Linux-CS2-External/blob/main/img/input.png?raw=true)

### Features:
- **Wallhack (WH)**: See enemies through walls with glow effect
- **Custom Crosshair Overlay**: Always-on-top crosshair that works over CS2 fullscreen
  - Bright green crosshair with black outline for maximum visibility
  - Click-through overlay (doesn't interfere with game input)
  - **Automatically detects CS2 window position and centers crosshair**
  - Works in fullscreen, windowed, and borderless modes
  - Dynamically adjusts when you move or resize CS2 window
  - Falls back to screen center if CS2 window is not detected

### Run CS2 and open terminal, type this:
```bash
sudo pacman -S base-devel cmake git glfw-x11 glew mesa libxfixes cairo
cd ~
git clone https://github.com/16Artem/Linux-CS2-External.git
cd Linux-CS2-External/
mkdir -p build && cd build/
cmake ..
make -j$(nproc)
sudo ./Linux-CS2
```

**For Ubuntu/Debian users:**
```bash
sudo apt install build-essential cmake git libglfw3-dev libglew-dev mesa-common-dev libxfixes-dev libcairo2-dev
```

### Usage:
1. Start CS2 first
2. Run the program with `sudo ./Linux-CS2`
3. Use the menu to toggle features:
   - **Wallhack checkbox**: Enable/disable wallhack
   - **Crosshair checkbox**: Enable/disable custom crosshair overlay
4. The crosshair will automatically detect CS2 window and center itself
5. The crosshair follows CS2 window if you move it or change resolution
6. The overlay is transparent to mouse clicks, so it won't interfere with gameplay

### Troubleshooting:
- If crosshair doesn't appear over CS2, make sure you're running the program with `sudo`
- The crosshair automatically detects CS2 window by name ("Counter-Strike 2", "cs2", or "CS2")
- If CS2 window is not detected, crosshair will use screen center as fallback
- Works with fullscreen, windowed, and borderless window modes
- The crosshair dynamically updates position when you move or resize CS2 window

#### Changelog from 24 May 2026:
 [+] Added blood WH glow when you hit enemy (with new CS2 update).
 
 [+] Updated Offsets.h for last version.
 
 [+] Improved crosshair overlay to work properly over CS2 fullscreen.
 
 [+] Enhanced crosshair visibility with outline and center dot.

Youtube video: https://youtu.be/M0zOoZ9-VBs

The offset in Offsets.h may need to be updated over time.

t.me/islavikhome
