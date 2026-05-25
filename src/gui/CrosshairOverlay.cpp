#include "CrosshairOverlay.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#include <cstring>
#include <iostream>

void* CrosshairOverlay::display = nullptr;
unsigned long CrosshairOverlay::window = 0;
bool CrosshairOverlay::enabled = false;
int CrosshairOverlay::screen_width = 1920;
int CrosshairOverlay::screen_height = 1080;
int CrosshairOverlay::cs2_x = 0;
int CrosshairOverlay::cs2_y = 0;
int CrosshairOverlay::cs2_width = 1920;
int CrosshairOverlay::cs2_height = 1080;
bool CrosshairOverlay::cs2_found = false;

static cairo_surface_t* surface = nullptr;
static cairo_t* cr = nullptr;

// Функция для поиска окна CS2
static Window FindCS2Window(Display* dpy, Window root) {
    Window parent, *children;
    unsigned int nchildren;
    
    if (!XQueryTree(dpy, root, &root, &parent, &children, &nchildren)) {
        return 0;
    }
    
    Window result = 0;
    
    for (unsigned int i = 0; i < nchildren; i++) {
        char* window_name = nullptr;
        if (XFetchName(dpy, children[i], &window_name)) {
            if (window_name) {
                std::string name(window_name);
                // Ищем окно CS2 (может называться "Counter-Strike 2" или просто "cs2")
                if (name.find("Counter-Strike 2") != std::string::npos || 
                    name.find("cs2") != std::string::npos ||
                    name.find("CS2") != std::string::npos) {
                    result = children[i];
                    XFree(window_name);
                    break;
                }
                XFree(window_name);
            }
        }
        
        // Рекурсивно ищем в дочерних окнах
        if (!result) {
            result = FindCS2Window(dpy, children[i]);
            if (result) break;
        }
    }
    
    if (children) XFree(children);
    return result;
}

bool CrosshairOverlay::Initialize() {
    Display* dpy = XOpenDisplay(nullptr);
    if (!dpy) {
        std::cerr << "[Crosshair] Failed to open X display" << std::endl;
        return false;
    }
    
    display = dpy;
    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);
    
    // Получаем размер экрана
    Screen* scr = DefaultScreenOfDisplay(dpy);
    screen_width = scr->width;
    screen_height = scr->height;
    
    // Пытаемся найти окно CS2
    UpdateCS2WindowPosition();
    
    // Создаем визуал с поддержкой прозрачности
    XVisualInfo vinfo;
    if (!XMatchVisualInfo(dpy, screen, 32, TrueColor, &vinfo)) {
        if (!XMatchVisualInfo(dpy, screen, 24, TrueColor, &vinfo)) {
            std::cerr << "[Crosshair] No appropriate visual found" << std::endl;
            XCloseDisplay(dpy);
            return false;
        }
    }
    
    Colormap colormap = XCreateColormap(dpy, root, vinfo.visual, AllocNone);
    
    XSetWindowAttributes swa;
    swa.colormap = colormap;
    swa.background_pixel = 0;
    swa.border_pixel = 0;
    swa.override_redirect = True;
    
    Window win = XCreateWindow(
        dpy, root,
        0, 0, screen_width, screen_height,
        0, vinfo.depth, InputOutput, vinfo.visual,
        CWColormap | CWBackPixel | CWBorderPixel | CWOverrideRedirect,
        &swa
    );
    
    window = win;
    
    // Делаем окно прозрачным для ввода
    XRectangle rect;
    rect.x = 0;
    rect.y = 0;
    rect.width = 0;
    rect.height = 0;
    
    XserverRegion region = XFixesCreateRegion(dpy, &rect, 1);
    XFixesSetWindowShapeRegion(dpy, win, ShapeInput, 0, 0, region);
    XFixesDestroyRegion(dpy, region);
    
    // Устанавливаем тип окна как dock (всегда поверх)
    Atom window_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    Atom dock_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(dpy, win, window_type, XA_ATOM, 32, PropModeReplace,
                    (unsigned char*)&dock_type, 1);
    
    // Устанавливаем состояние окна (всегда поверх, на всех рабочих столах)
    Atom state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom above = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
    Atom sticky = XInternAtom(dpy, "_NET_WM_STATE_STICKY", False);
    Atom skip_taskbar = XInternAtom(dpy, "_NET_WM_STATE_SKIP_TASKBAR", False);
    Atom skip_pager = XInternAtom(dpy, "_NET_WM_STATE_SKIP_PAGER", False);
    
    Atom states[] = {above, sticky, skip_taskbar, skip_pager};
    XChangeProperty(dpy, win, state, XA_ATOM, 32, PropModeReplace,
                    (unsigned char*)states, 4);
    
    // Создаем Cairo surface для рисования
    surface = cairo_xlib_surface_create(dpy, win, vinfo.visual, screen_width, screen_height);
    cr = cairo_create(surface);
    
    std::cout << "[Crosshair] Overlay initialized (" << screen_width << "x" << screen_height << ")" << std::endl;
    if (cs2_found) {
        std::cout << "[Crosshair] CS2 window found at (" << cs2_x << "," << cs2_y 
                  << ") size: " << cs2_width << "x" << cs2_height << std::endl;
    }
    return true;
}

void CrosshairOverlay::Shutdown() {
    if (!display) return;
    
    Display* dpy = (Display*)display;
    
    if (cr) {
        cairo_destroy(cr);
        cr = nullptr;
    }
    
    if (surface) {
        cairo_surface_destroy(surface);
        surface = nullptr;
    }
    
    if (window) {
        XDestroyWindow(dpy, window);
        window = 0;
    }
    
    XCloseDisplay(dpy);
    display = nullptr;
    
    std::cout << "[Crosshair] Overlay shutdown" << std::endl;
}

void CrosshairOverlay::UpdateCS2WindowPosition() {
    if (!display) return;
    
    Display* dpy = (Display*)display;
    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);
    
    // Ищем окно CS2
    Window cs2_window = FindCS2Window(dpy, root);
    
    if (cs2_window) {
        XWindowAttributes attrs;
        if (XGetWindowAttributes(dpy, cs2_window, &attrs)) {
            // Получаем абсолютные координаты окна
            Window child;
            int abs_x, abs_y;
            XTranslateCoordinates(dpy, cs2_window, root, 0, 0, &abs_x, &abs_y, &child);
            
            cs2_x = abs_x;
            cs2_y = abs_y;
            cs2_width = attrs.width;
            cs2_height = attrs.height;
            
            if (!cs2_found) {
                std::cout << "[Crosshair] CS2 window found at (" << cs2_x << "," << cs2_y 
                          << ") size: " << cs2_width << "x" << cs2_height << std::endl;
            }
            cs2_found = true;
        }
    } else {
        if (cs2_found) {
            std::cout << "[Crosshair] CS2 window lost, using screen center" << std::endl;
        }
        cs2_found = false;
        // Если окно не найдено, используем центр экрана
        cs2_x = 0;
        cs2_y = 0;
        cs2_width = screen_width;
        cs2_height = screen_height;
    }
}

void CrosshairOverlay::Render() {
    if (!display || !window || !enabled || !cr) return;
    
    Display* dpy = (Display*)display;
    Window win = window;
    
    // Обновляем позицию окна CS2 каждый кадр
    UpdateCS2WindowPosition();
    
    // Показываем окно если оно скрыто
    XWindowAttributes attrs;
    XGetWindowAttributes(dpy, win, &attrs);
    if (attrs.map_state != IsViewable) {
        XMapWindow(dpy, win);
        XRaiseWindow(dpy, win);
    }
    
    // Убеждаемся что окно поверх всех
    XRaiseWindow(dpy, win);
    
    // Очищаем поверхность (полностью прозрачный фон)
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);
    
    // Вычисляем центр окна CS2
    float cx = cs2_x + (cs2_width * 0.5f);
    float cy = cs2_y + (cs2_height * 0.5f);
    
    // Настройки прицела
    float size = 15.0f;      // Длина линий
    float gap = 5.0f;        // Зазор в центре
    float thickness = 3.0f;  // Толщина линий
    float outlineThickness = 5.0f;  // Толщина обводки
    
    // Рисуем черную обводку для лучшей видимости
    cairo_set_line_width(cr, outlineThickness);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.9);
    
    // Горизонтальная линия (обводка)
    cairo_move_to(cr, cx - size, cy);
    cairo_line_to(cr, cx - gap, cy);
    cairo_move_to(cr, cx + gap, cy);
    cairo_line_to(cr, cx + size, cy);
    
    // Вертикальная линия (обводка)
    cairo_move_to(cr, cx, cy - size);
    cairo_line_to(cr, cx, cy - gap);
    cairo_move_to(cr, cx, cy + gap);
    cairo_line_to(cr, cx, cy + size);
    
    cairo_stroke(cr);
    
    // Рисуем основной прицел (яркий зеленый)
    cairo_set_line_width(cr, thickness);
    cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 1.0);
    
    // Горизонтальная линия
    cairo_move_to(cr, cx - size, cy);
    cairo_line_to(cr, cx - gap, cy);
    cairo_move_to(cr, cx + gap, cy);
    cairo_line_to(cr, cx + size, cy);
    
    // Вертикальная линия
    cairo_move_to(cr, cx, cy - size);
    cairo_line_to(cr, cx, cy - gap);
    cairo_move_to(cr, cx, cy + gap);
    cairo_line_to(cr, cx, cy + size);
    
    cairo_stroke(cr);
    
    // Рисуем центральную точку для точности
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.9);
    cairo_arc(cr, cx, cy, 2.0, 0, 2 * 3.14159);
    cairo_fill(cr);
    
    cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, 1.0);
    cairo_arc(cr, cx, cy, 1.0, 0, 2 * 3.14159);
    cairo_fill(cr);
    
    // Применяем изменения
    cairo_surface_flush(surface);
    XFlush(dpy);
}

void CrosshairOverlay::SetEnabled(bool en) {
    if (enabled == en) return;
    
    enabled = en;
    
    if (!display || !window) return;
    
    Display* dpy = (Display*)display;
    Window win = window;
    
    if (enabled) {
        XMapWindow(dpy, win);
        XRaiseWindow(dpy, win);
        std::cout << "[Crosshair] Enabled" << std::endl;
    } else {
        XUnmapWindow(dpy, win);
        std::cout << "[Crosshair] Disabled" << std::endl;
    }
    
    XFlush(dpy);
}

bool CrosshairOverlay::IsEnabled() {
    return enabled;
}
