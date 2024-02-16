#ifndef _FLUID_CONTEXT_HPP_
#define _FLUID_CONTEXT_HPP_

#ifdef __EMSCRIPTEN__
#include <GL/gl.h>
#include <GLES3/gl3.h>
#else
#include "glad/glad.h"
#endif

#include <stdexcept>
#include <iostream>
#include <SDL.h>
#include <SDL_opengl.h>

class Context{
public:
    bool const useVsync = true;
    unsigned int const viewportWidth;
    unsigned int const viewportHeight;
public:
    Context(SDL_Window* window, unsigned int width, unsigned int height);
    ~Context();
    Context(Context const&) = delete;
    Context(Context const&&) = delete;
    Context& operator=(Context const&) = delete;
    Context& operator=(Context const&&) = delete;
    SDL_GLContext getContext() const;
    bool successfullyInitialised() const;
private:
    SDL_GLContext context = nullptr;
    bool m_successfullyInitialised = false;   
};

#endif