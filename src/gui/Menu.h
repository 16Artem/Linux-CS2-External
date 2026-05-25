#pragma once

#include <cstdint>

class Menu {
public:
    static bool Setup();
    static void Shutdown();
    static void Render();
    static bool IsRunning();
    static void SetWallhackEnabled(bool enabled);
    static bool IsWallhackEnabled();
    static void SetCrosshairEnabled(bool enabled);
    static bool IsCrosshairEnabled();
    static void SetVisible(bool visible);
    static bool IsVisible();
    static void SetCurrentFPS(float fps);
    static float GetCurrentFPS();
    static void SetWindowSize(int width, int height);
    static void SetCS2Pid(uint32_t pid);
};