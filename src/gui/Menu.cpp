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
static float g_currentFPS = 0.0f;
static GLFWwindow* g_window = nullptr;
static int g_displayWidth = 1280;
static int g_displayHeight = 720;
static int g_miniWidth = 80;
static int g_miniHeight = 30;
static int g_windowPosX = 100;
static int g_windowPosY = 100;
static bool g_dragging = false;
static ImVec2 g_dragOffset;

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

    // Перетаскивание только за область с текстом "CS2 Menu" (левый верхний угол)
    glfwSetMouseButtonCallback(g_window, [](GLFWwindow* win, int button, int action, int) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (action == GLFW_PRESS) {
                double xpos, ypos;
                glfwGetCursorPos(win, &xpos, &ypos);
                // Проверяем, что курсор в области текста "CS2 Menu" (x: 0-120, y: 0-35)
                if (xpos >= 0 && xpos <= 120 && ypos >= 0 && ypos <= 35) {
                    g_dragging = true;
                    g_dragOffset = ImVec2(xpos, ypos);
                }
            } else if (action == GLFW_RELEASE) {
                g_dragging = false;
            }
        }
    });

    glfwSetCursorPosCallback(g_window, [](GLFWwindow* win, double xpos, double ypos) {
        if (g_dragging) {
            int currentX, currentY;
            glfwGetWindowPos(win, &currentX, &currentY);
            int newX = currentX + (xpos - g_dragOffset.x);
            int newY = currentY + (ypos - g_dragOffset.y);
            glfwSetWindowPos(win, newX, newY);
            g_windowPosX = newX;
            g_windowPosY = newY;
        }
    });

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
    colors[ImGuiCol_WindowBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // Белый
    colors[ImGuiCol_Border] = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);     // Светло-серый
    colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);       // Черный текст
    colors[ImGuiCol_CheckMark] = ImVec4(0.2f, 0.6f, 1.0f, 1.0f);   // Синяя галочка
    colors[ImGuiCol_FrameBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
    
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    return true;
}

void Menu::Shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    if (g_window) {
        glfwDestroyWindow(g_window);
        g_window = nullptr;
    }
    
    glfwTerminate();
}

void Menu::Render() {
    glfwPollEvents();
    
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
        ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "%s", text.c_str());
        
        if (ImGui::IsMouseClicked(0) && 
            ImGui::GetIO().MousePos.x > windowPos.x && 
            ImGui::GetIO().MousePos.x < windowPos.x + windowWidth &&
            ImGui::GetIO().MousePos.y > windowPos.y && 
            ImGui::GetIO().MousePos.y < windowPos.y + windowHeight) {
            g_minimized = false;
            glfwSetWindowSize(g_window, g_displayWidth, g_displayHeight);
        }
        
    } else {
        // Верхняя панель (только фон, без возможности перетаскивания за всю панель)
        drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + windowWidth, windowPos.y + 35), 
                                IM_COL32(240, 240, 240, 255), 0);
        
        // Кнопка закрытия
        ImVec2 closePos(windowPos.x + windowWidth - 35, windowPos.y + 5);
        
        bool closeHovered = (ImGui::GetIO().MousePos.x > closePos.x && 
                             ImGui::GetIO().MousePos.x < closePos.x + 25 &&
                             ImGui::GetIO().MousePos.y > closePos.y && 
                             ImGui::GetIO().MousePos.y < closePos.y + 25);
        
        if (closeHovered) {
            drawList->AddRectFilled(closePos, ImVec2(closePos.x + 25, closePos.y + 25), 
                                    IM_COL32(255, 100, 100, 255), 0);
            if (ImGui::IsMouseClicked(0)) {
                g_running = false;
            }
        } else {
            drawList->AddRectFilled(closePos, ImVec2(closePos.x + 25, closePos.y + 25), 
                                    IM_COL32(230, 230, 230, 255), 0);
        }
        drawList->AddLine(ImVec2(closePos.x + 8, closePos.y + 8), 
                          ImVec2(closePos.x + 17, closePos.y + 17), 
                          IM_COL32(100, 100, 100, 255), 2.0f);
        drawList->AddLine(ImVec2(closePos.x + 17, closePos.y + 8), 
                          ImVec2(closePos.x + 8, closePos.y + 17), 
                          IM_COL32(100, 100, 100, 255), 2.0f);
        
        // Кнопка минимизации
        ImVec2 minusPos(windowPos.x + windowWidth - 65, windowPos.y + 5);
        
        bool minusHovered = (ImGui::GetIO().MousePos.x > minusPos.x && 
                             ImGui::GetIO().MousePos.x < minusPos.x + 25 &&
                             ImGui::GetIO().MousePos.y > minusPos.y && 
                             ImGui::GetIO().MousePos.y < minusPos.y + 25);
        
        if (minusHovered) {
            drawList->AddRectFilled(minusPos, ImVec2(minusPos.x + 25, minusPos.y + 25), 
                                    IM_COL32(220, 220, 220, 255), 0);
            if (ImGui::IsMouseClicked(0)) {
                g_minimized = true;
                glfwSetWindowSize(g_window, g_miniWidth, g_miniHeight);
            }
        } else {
            drawList->AddRectFilled(minusPos, ImVec2(minusPos.x + 25, minusPos.y + 25), 
                                    IM_COL32(230, 230, 230, 255), 0);
        }
        drawList->AddLine(ImVec2(minusPos.x + 8, minusPos.y + 12), 
                          ImVec2(minusPos.x + 17, minusPos.y + 12), 
                          IM_COL32(100, 100, 100, 255), 2.0f);
        
        // Область с текстом "CS2 Menu" - за нее можно перетаскивать окно
        // Рисуем подсветку при наведении для индикации, что здесь можно перетаскивать
        ImVec2 dragAreaPos(windowPos.x, windowPos.y);
        ImVec2 dragAreaEnd(windowPos.x + 120, windowPos.y + 35);
        
        bool dragAreaHovered = (ImGui::GetIO().MousePos.x > dragAreaPos.x && 
                                ImGui::GetIO().MousePos.x < dragAreaEnd.x &&
                                ImGui::GetIO().MousePos.y > dragAreaPos.y && 
                                ImGui::GetIO().MousePos.y < dragAreaEnd.y);
        
        if (dragAreaHovered) {
            drawList->AddRectFilled(dragAreaPos, dragAreaEnd, IM_COL32(230, 230, 250, 80), 0);
        }
        
        // Название в левом верхнем углу (за эту область можно перетаскивать)
        ImGui::SetCursorPos(ImVec2(15, 10));
        ImGui::SetWindowFontScale(1.2f);
        ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "CS2 Menu");
        ImGui::SetWindowFontScale(1.0f);
        
        // Разделительная линия под заголовком
        drawList->AddLine(ImVec2(windowPos.x, windowPos.y + 35), 
                          ImVec2(windowPos.x + windowWidth, windowPos.y + 35), 
                          IM_COL32(220, 220, 220, 255), 1.0f);
        
        // Переключатель Wallhack: текст слева, переключатель справа от текста
        ImGui::SetCursorPos(ImVec2(20, 55));
        ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 1.0f), "Wallhack");
        
        // Переключатель рядом с текстом
        ImGui::SameLine(0.0f, 10.0f);
        ImGui::Checkbox("##Wallhack", &g_wallhack_enabled);
        
        // Статус ON/OFF рядом с переключателем
        ImGui::SameLine(0.0f, 10.0f);
        if (g_wallhack_enabled) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "ON");
        } else {
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "OFF");
        }
        
        // FPS снизу
        ImGui::SetCursorPos(ImVec2(20, windowHeight - 30));
        std::ostringstream fpsStream;
        fpsStream << "FPS: " << std::fixed << std::setprecision(0) << g_currentFPS;
        std::string fpsText = fpsStream.str();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", fpsText.c_str());
        
        // Размер окна снизу справа
        ImGui::SetCursorPos(ImVec2(windowWidth - 120, windowHeight - 30));
        std::string resolutionText = std::to_string(g_displayWidth) + "x" + std::to_string(g_displayHeight);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", resolutionText.c_str());
    }

    ImGui::End();

    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(g_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);  // Белый фон
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(g_window);
}

bool Menu::IsRunning() {
    return g_running && !glfwWindowShouldClose(g_window);
}

void Menu::SetWallhackEnabled(bool enabled) {
    g_wallhack_enabled = enabled;
}

bool Menu::IsWallhackEnabled() {
    return g_wallhack_enabled;
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