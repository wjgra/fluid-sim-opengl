#ifndef _FLUID_DRAWABLE_HPP_
#define _FLUID_DRAWABLE_HPP_

#ifdef __EMSCRIPTEN__
#include <GL/gl.h>
#include <GLES3/gl3.h>
#else
#include "glad/glad.h"
#endif

#include <iostream>
#include <vector>
#include <stdexcept>

class Drawable{
public:
    Drawable() = delete;
    Drawable(std::vector<float> const& verts, unsigned int vertexDimension);
    Drawable(Drawable const&) = delete;
    Drawable(Drawable const&&) = delete;
    Drawable& operator=(Drawable const&) = delete;
    Drawable& operator=(Drawable const&&) = delete;
    ~Drawable();
    void bindVAO() const;
    static void unbindVAO();
    void draw(GLint drawingMode = GL_TRIANGLES) const;
    bool successfullyInitialised() const;
private:
    void setUpBuffers(unsigned int vertexDimension = 3);
    void releaseBuffers();
    bool m_successfullyInitialised;
    std::vector<float> const m_vertices;
    GLuint m_VBO, m_VAO;
};

struct DrawableUniformLocations{
    GLint m_modelTransformation, m_projectionTransformation, m_viewTransformation;
};
#endif