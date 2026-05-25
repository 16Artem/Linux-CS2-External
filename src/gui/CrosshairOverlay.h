#pragma once

class CrosshairOverlay {
public:
    static bool Initialize();
    static void Shutdown();
    static void Render();
    static void SetEnabled(bool enabled);
    static bool IsEnabled();
    static void UpdateCS2WindowPosition();
    
private:
    static void* display;
    static unsigned long window;
    static bool enabled;
    static int screen_width;
    static int screen_height;
    static int cs2_x;
    static int cs2_y;
    static int cs2_width;
    static int cs2_height;
    static bool cs2_found;
};
