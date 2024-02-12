#ifndef _FLUID_APP_STATE_HPP_
#define _FLUID_APP_STATE_HPP_

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
#include "../include/fluid_renderer.hpp"

class AppState{
private:
    unsigned int const m_windowDisplayScale;
    unsigned int const m_notionalWindowWidth;
    unsigned int const m_notionalWindowHeight;
public:
    AppState(unsigned int w, unsigned int h, unsigned int scale = 1);
    bool successfullyInitialised() const;
    void beginLoop();
    void mainLoop();
    void handleEvents(SDL_Event const&  event);
    void frame(unsigned int frameTime);
    void quitApp();
    bool timeToQuit() const;
private:
    bool m_quitApplication;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_tStart, m_tNow;
    Window m_window;
    Context m_context;
    FluidRenderer m_fluidRenderer;
    GUIState m_guiState;
};
#endif