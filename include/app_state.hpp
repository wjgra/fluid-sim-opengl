#ifndef _JULIA_APP_STATE_HPP_
#define _JULIA_APP_STATE_HPP_

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GL/gl.h>
#include <GLES3/gl3.h>
#else
#include "../include/glad/glad.h"
#endif

#include <SDL.h>
#include <SDL_opengl.h>

#include <vector>
#include <chrono>

#include "../include/window.hpp"
#include "../include/context.hpp"
#include "../include/shader_program.hpp"

#include "../include/gui_state.hpp"


class AppState{
public:
    AppState(unsigned int scale);
    void beginLoop();
    void mainLoop();
    void handleEvents(SDL_Event const&  event);
    void frame(unsigned int frameTime);
    void quitApp();
    // Window/context state parameters
    bool quit = false;
    SDL_Event event;
    std::chrono::time_point<std::chrono::high_resolution_clock> tStart, tNow;
    // -- Viewport resolution is winScale * notional dimension
    unsigned int const winScale = 1;
    // -- Dimensions of notional window
    int const winWidth = 640;
    int const winHeight = 480;
    // Ordering ensures context and shaders initialised before game objects and destroyed in reverse order
    Window window;
    Context context;
    GUIState guiState;
};
#endif