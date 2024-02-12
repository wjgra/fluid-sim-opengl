#ifndef _FLUID_TEXTURE_HPP_
#define _FLUID_TEXTURE_HPP_

#ifdef __EMSCRIPTEN__
#include <GL/gl.h>
#include <GLES3/gl3.h>
#else
#include "../include/glad/glad.h"
#endif

#include "../include/stb_image.h"

#include <iostream>
#include <string>

class Texture{
public:
    Texture(unsigned int w, unsigned int h, bool useNearest = false);
    Texture(std::string const& path);
    Texture(Texture const&) = delete;
    Texture(Texture const&&) = delete;
    Texture& operator=(Texture const&) = delete;
    Texture& operator=(Texture const&&) = delete;
    ~Texture();
    void bind() const;
    void unbind() const;
    GLuint getLocation() const;
    void resize(unsigned int width, unsigned int height);
private:
    GLuint m_texture;
    GLint m_width, m_height, m_numberOfChannels;
};

#endif