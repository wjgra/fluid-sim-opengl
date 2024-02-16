#ifndef _FLUID_APP_STATE_HPP_
#define _FLUID_APP_STATE_HPP_

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <GL/gl.h>
#include <GLES3/gl3.h>
#else
#include "glad/glad.h"
#endif

#include <SDL.h>
#include <SDL_opengl.h>

#include <vector>
#include <chrono>

#include "window.hpp"
#include "context.hpp"
#include "shader_program.hpp"

#include "gui_state.hpp"
#include "fluid.hpp"

class AppState{
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
    Fluid m_fluid;
    /* FluidSimulator m_fluidSimulator;
    FluidRenderer m_fluidRenderer;
    NewFluidRenderer m_newFluidRenderer;
    FluidController m_fluidController; */
    GUIState m_guiState;
};
#endif