#ifndef _FLUID_GUI_STATE_HPP_
#define _FLUID_GUI_STATE_HPP_

#ifdef __EMSCRIPTEN__
#include <GL/gl.h>
#include <GLES3/gl3.h>
#else
#include "glad/glad.h"
#endif

#include <SDL.h>
#include <SDL_opengl.h>

#include "text_renderer.hpp"

class GUIState{
public:
    GUIState(unsigned int width, unsigned int height);
    bool successfullyInitialised() const;
    void frame();
private:
    TextRenderer m_textRen;
};

#endif