// N.B. CULL_FRONT_FACES doesn't hide the text, so the quad here has the opposite winding to expected (should be CCW)!


#ifndef _FLUID_TEXT_RENDERER_HPP_
#define _FLUID_TEXT_RENDERER_HPP_

#ifdef __EMSCRIPTEN__
#include <GL/gl.h>
#include <GLES3/gl3.h>
#else
#include "../include/glad/glad.h"
#endif

#include <iostream>
#include <vector>
#include <string>

#define GLM_FORCE_PURE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../include/texture.hpp"
#include "../include/shader_program.hpp"

class TextRenderer{
public:
    TextRenderer(unsigned int w, unsigned int h);
    TextRenderer(TextRenderer const&) = delete;
    TextRenderer(TextRenderer const&&) = delete;
    TextRenderer& operator=(TextRenderer const&) = delete;
    TextRenderer& operator=(TextRenderer const&&) = delete;
    ~TextRenderer();
private:
    void setUpBuffers();
    void releaseBuffers();
    void setChar(char toDraw) const;
    void setPosition(float scale, float xPos, float yPos) const;
public:
    bool successfullyInitialised() const;
    void drawString(std::string const& toDraw, float scale, float xPos, float yPos) const;
    void drawStringCentred(std::string const& toDraw, float scale, float xPos, float yPos) const;
private:
    ShaderProgram m_shader;
    Texture m_texture;
    bool m_successfullyInitialised = false;
    std::vector<float> const m_quadVertexData {
        1.0f, 0.0f,   0.125f, 0.0f,   
        1.0f, 1.0f,   0.125f, 0.125f,   
        0.0f, 0.0f,   0.0f, 0.0f,   
        0.0f, 1.0f,   0.0f, 0.125f  
    };
    std::vector<GLuint> const m_quadElementData {0, 1, 2, 3};
    GLuint m_VBO, m_EBO, m_VAO;

    GLint m_uniformModelTrans, m_uniformProjTrans, m_uniformTextureCoordOffset;
};

#endif


