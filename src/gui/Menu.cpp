#include "Menu.h"
#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_glfw.h"
#include "../imgui/backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <string>
#include <sstream>
#include <iomanip>

static bool g_running = true;
static bool g_minimized = false;
static bool g_wallhack_enabled = false;
static bool g_crosshair_enabled = false;
static float g_currentFPS = 0.0f;
static GLFWwindow* g_window = nullptr;
static GLFWwindow* g_overlay_window = nullptr;
static int g_screen_width = 1920;
static int g_screen_height = 1080;
static int g_displayWidth = 1280;
static int g_displayHeight = 720;
static int g_miniWidth = 80;
static int g_miniHeight = 30;
static int g_windowPosX = 100;
static int g_windowPosY = 100;
static bool g_dragging = false;
static double g_dragStartX = 0, g_dragStartY = 0;
static int g_windowStartX = 0, g_windowStartY = 0;

static void RenderCrosshairOverlay() {
    if (!g_overlay_window) return;

    bool wantVisible = g_crosshair_enabled;
    bool isVisible = glfwGetWindowAttrib(g_overlay_window, GLFW_VISIBLE) != 0;
    if (wantVisible && !isVisible) {
        glfwShowWindow(g_overlay_window);
    } else if (!wantVisible && isVisible) {
        glfwHideWindow(g_overlay_window);
        return;
    }
    if (!wantVisible) return;

    // Keep the overlay sized to the primary monitor (handles resolution
    // changes from CS2) and pinned above other windows each frame.
    if (GLFWmonitor* primary = glfwGetPrimaryMonitor()) {
        if (const GLFWvidmode* mode = glfwGetVideoMode(primary)) {
            int mx = 0, my = 0;
            glfwGetMonitorPos(primary, &mx, &my);
            int cur_w, cur_h, cur_x, cur_y;
            glfwGetWindowSize(g_overlay_window, &cur_w, &cur_h);
            glfwGetWindowPos(g_overlay_window, &cur_x, &cur_y);
            if (cur_w != mode->width || cur_h != mode->height) {
                glfwSetWindowSize(g_overlay_window, mode->width, mode->height);
                g_screen_width = mode->width;
                g_screen_height = mode->height;
            }
            if (cur_x != mx || cur_y != my) {
                glfwSetWindowPos(g_overlay_window, mx, my);
            }
        }
    }
    glfwSetWindowAttrib(g_overlay_window, GLFW_FLOATING, GLFW_TRUE);
#ifdef GLFW_MOUSE_PASSTHROUGH
    glfwSetWindowAttrib(g_overlay_window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
#endif

    glfwMakeContextCurrent(g_overlay_window);

    int ow, oh;
    glfwGetFramebufferSize(g_overlay_window, &ow, &oh);
    glViewport(0, 0, ow, oh);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double)ow, (double)oh, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float cx = ow * 0.5f;
    float cy = oh * 0.5f;
    float size = 10.0f;
    float gap = 3.0f;

    glLineWidth(2.0f);
    glColor4f(0.2f, 1.0f, 0.2f, 1.0f);
    glBegin(GL_LINES);
        glVertex2f(cx - size, cy); glVertex2f(cx - gap, cy);
        glVertex2f(cx + gap, cy);  glVertex2f(cx + size, cy);
        glVertex2f(cx, cy - size); glVertex2f(cx, cy - gap);
        glVertex2f(cx, cy + gap);  glVertex2f(cx, cy + size);
    glEnd();

    glfwSwapBuffers(g_overlay_window);
    glfwMakeContextCurrent(g_window);
}

bool Menu::Setup() {
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    
    g_window = glfwCreateWindow(g_displayWidth, g_displayHeight, "CS2 Menu", nullptr, nullptr);
    if (!g_window) {
        glfwTerminate();
        return false;
    }

    glfwSetWindowPos(g_window, g_windowPosX, g_windowPosY);
    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Прямые углы без закруглений
    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.TabRounding = 0.0f;
    
    style.WindowBorderSize = 1.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.ItemSpacing = ImVec2(8, 8);
    style.FramePadding = ImVec2(8, 4);
    
    // Белый фон для окна
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    colors[ImGuiCol_Border] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
    colors[ImGuiCol_Button] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
    
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Click-through transparent fullscreen overlay for the crosshair.
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = primary ? glfwGetVideoMode(primary) : nullptr;
    int monitor_x = 0, monitor_y = 0;
    if (primary) glfwGetMonitorPos(primary, &monitor_x, &monitor_y);
    if (mode) {
        g_screen_width = mode->width;
        g_screen_height = mode->height;
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
#ifdef GLFW_MOUSE_PASSTHROUGH
    glfwWindowHint(GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
#endif

    g_overlay_window = glfwCreateWindow(g_screen_width, g_screen_height,
                                        "Crosshair", nullptr, nullptr);
    if (g_overlay_window) {
        glfwSetWindowPos(g_overlay_window, monitor_x, monitor_y);
        glfwSetWindowAttrib(g_overlay_window, GLFW_FLOATING, GLFW_TRUE);
#ifdef GLFW_MOUSE_PASSTHROUGH
        glfwSetWindowAttrib(g_overlay_window, GLFW_MOUSE_PASSTHROUGH, GLFW_TRUE);
#endif
        glfwMakeContextCurrent(g_overlay_window);
        glfwSwapInterval(0);
    }

    glfwMakeContextCurrent(g_window);

    return true;
}

void Menu::Shutdown() {
    // Сначала очищаем ImGui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    // Затем уничтожаем окно
    if (g_overlay_window) {
        glfwDestroyWindow(g_overlay_window);
        g_overlay_window = nullptr;
    }
    if (g_window) {
        glfwDestroyWindow(g_window);
        g_window = nullptr;
    }
    
    // Завершаем GLFW
    glfwTerminate();
    
    // Устанавливаем флаг остановки
    g_running = false;
}

void Menu::Render() {
    // Проверяем, нужно ли завершить работу
    if (!g_running) {
        return;
    }
    
    glfwPollEvents();
    
    // Проверяем, не запрошено ли закрытие окна
    if (glfwWindowShouldClose(g_window)) {
        g_running = false;
        return;
    }
    
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int currentWidth, currentHeight;
    glfwGetWindowSize(g_window, &currentWidth, &currentHeight);
    
    ImGui::SetNextWindowSize(ImVec2(currentWidth, currentHeight), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    
    ImGui::Begin("Main", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | 
                 ImGuiWindowFlags_NoResize | 
                 ImGuiWindowFlags_NoMove | 
                 ImGuiWindowFlags_NoCollapse | 
                 ImGuiWindowFlags_NoSavedSettings);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    float windowWidth = ImGui::GetWindowWidth();
    float windowHeight = ImGui::GetWindowHeight();
    
    if (g_minimized) {
        std::string text = "OPEN";
        float textWidth = ImGui::CalcTextSize(text.c_str()).x;
        
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::SetCursorPosY((windowHeight - ImGui::GetTextLineHeight()) * 0.5f);
        
        // Кнопка OPEN
        if (ImGui::Button(text.c_str(), ImVec2(60, 25))) {
            g_minimized = false;
            glfwSetWindowSize(g_window, g_displayWidth, g_displayHeight);
        }
        
    } else {
        // Верхняя панель
        ImGui::SetCursorPos(ImVec2(0, 0));
        
        // Создаем невидимую кнопку для перетаскивания
        ImGui::InvisibleButton("title_bar", ImVec2(windowWidth, 35));
        
        // Обработка перетаскивания
        if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!g_dragging) {
                // Начинаем перетаскивание
                g_dragging = true;
                g_dragStartX = ImGui::GetMousePos().x;
                g_dragStartY = ImGui::GetMousePos().y;
                g_windowStartX = g_windowPosX;
                g_windowStartY = g_windowPosY;
            } else {
                // Перемещаем окно
                ImVec2 mouseDelta;
                mouseDelta.x = ImGui::GetMousePos().x - g_dragStartX;
                mouseDelta.y = ImGui::GetMousePos().y - g_dragStartY;
                
                int newX = g_windowStartX + (int)mouseDelta.x;
                int newY = g_windowStartY + (int)mouseDelta.y;
                glfwSetWindowPos(g_window, newX, newY);
                g_windowPosX = newX;
                g_windowPosY = newY;
            }
        } else {
            g_dragging = false;
        }
        
        // Рисуем фон верхней панели
        drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + windowWidth, windowPos.y + 35), 
                                IM_COL32(240, 240, 240, 255), 0);
        
        // Подсветка при наведении
        if (ImGui::IsItemHovered()) {
            drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + windowWidth, windowPos.y + 35), 
                                    IM_COL32(230, 230, 250, 80), 0);
        }
        
        // Название
        ImGui::SetCursorPos(ImVec2(15, 10));
        ImGui::SetWindowFontScale(1.2f);
        ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "CS2 Menu");
        ImGui::SetWindowFontScale(1.0f);
        
        // Кнопка закрытия (X) в правом верхнем углу
        float closeButtonX = windowWidth - 35;
        float closeButtonY = 5;
        ImGui::SetCursorPos(ImVec2(closeButtonX, closeButtonY));
        
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
        
        if (ImGui::Button("X", ImVec2(25, 25))) {
            g_running = false;
            glfwSetWindowShouldClose(g_window, true);
        }
        
        ImGui::PopStyleColor(3);
        
        // Разделительная линия
        drawList->AddLine(ImVec2(windowPos.x, windowPos.y + 35), 
                          ImVec2(windowPos.x + windowWidth, windowPos.y + 35), 
                          IM_COL32(220, 220, 220, 255), 1.0f);
        
        // Контент
        ImGui::SetCursorPos(ImVec2(20, 55));
        ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "Wallhack");
        
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::Checkbox("##Wallhack", &g_wallhack_enabled);
        
        ImGui::SameLine(0.0f, 10.0f);
        if (g_wallhack_enabled) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "ON");
        } else {
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "OFF");
        }
        
        // Прицел
        ImGui::SetCursorPos(ImVec2(20, 85));
        ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "Crosshair");
        
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::Checkbox("##Crosshair", &g_crosshair_enabled);
        
        ImGui::SameLine(0.0f, 10.0f);
        if (g_crosshair_enabled) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "ON");
        } else {
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "OFF");
        }
        
        // FPS
        ImGui::SetCursorPos(ImVec2(20, windowHeight - 30));
        std::ostringstream fpsStream;
        fpsStream << "FPS: " << std::fixed << std::setprecision(0) << g_currentFPS;
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", fpsStream.str().c_str());
        
        // Размер окна
        ImGui::SetCursorPos(ImVec2(windowWidth - 120, windowHeight - 30));
        std::string resolutionText = std::to_string(g_displayWidth) + "x" + std::to_string(g_displayHeight);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", resolutionText.c_str());
    }

    ImGui::End();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(g_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(g_window);

    RenderCrosshairOverlay();
}

bool Menu::IsRunning() {
    return g_running && g_window != nullptr && !glfwWindowShouldClose(g_window);
}

void Menu::SetWallhackEnabled(bool enabled) {
    g_wallhack_enabled = enabled;
}

bool Menu::IsWallhackEnabled() {
    return g_wallhack_enabled;
}

void Menu::SetCrosshairEnabled(bool enabled) {
    g_crosshair_enabled = enabled;
}

bool Menu::IsCrosshairEnabled() {
    return g_crosshair_enabled;
}

void Menu::SetVisible(bool visible) {
    if (visible) {
        glfwSetWindowSize(g_window, g_displayWidth, g_displayHeight);
        g_minimized = false;
    } else {
        glfwSetWindowSize(g_window, g_miniWidth, g_miniHeight);
        g_minimized = true;
    }
}

bool Menu::IsVisible() {
    return !g_minimized;
}

void Menu::SetCurrentFPS(float fps) {
    g_currentFPS = fps;
}

float Menu::GetCurrentFPS() {
    return g_currentFPS;
}

void Menu::SetWindowSize(int width, int height) {
    g_displayWidth = width;
    g_displayHeight = height;
    if (!g_minimized) {
        glfwSetWindowSize(g_window, width, height);
    }
}