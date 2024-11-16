#pragma once

#include "imgui.h"
#include <gl3w.h> 
#include <GLFW/glfw3.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "common.hpp"

class HelpOverlay {
public:
    HelpOverlay() : show_help(false), pixelFont(nullptr) {}
    ~HelpOverlay();

    void init(GLFWwindow* window);
    void toggle() { show_help = !show_help; }
    bool isVisible() const { return show_help; }
    void render();

private:
    bool show_help;
    ImFont* pixelFont;
    GLuint backgroundTexture;

    GLuint LoadTexture(const char* filename);
};