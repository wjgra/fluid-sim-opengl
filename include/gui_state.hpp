#ifndef _FLUID_GUI_STATE_HPP_
#define _FLUID_GUI_STATE_HPP_

#ifdef __EMSCRIPTEN__
#include <GL/gl.h>
#include <GLES3/gl3.h>
#else
#include "../include/glad/glad.h"
#endif

#include <SDL.h>
#include <SDL_opengl.h>

#include <glm/gtc/type_ptr.hpp>

#include "../include/text_renderer.hpp"

class GUIState{
public:
    GUIState(unsigned int width, unsigned int height);
    void frame(unsigned int frameTime);
    void handleEvents(const SDL_Event& event);
private:
    unsigned int width, height;
    void drawLocation();
    TextRenderer textRen;
};

#endif