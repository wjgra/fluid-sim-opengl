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
    Texture(unsigned int w, unsigned int h);
    Texture(std::string const& path);
    Texture(Texture const& other) = delete;
    Texture& operator=(Texture const& other) = delete;
    Texture(Texture&& other) :
        texture{other.texture},
        width{other.width},
        height{other.height},
        numberOfChannels{other.numberOfChannels}{
        other.texture = 0;
    }
    Texture& operator=(Texture&& other){
        if (this != &other){
            std::swap(texture, other.texture);
        }
        return *this;
    }
    ~Texture();
    void bind();
    void unbind();
    GLuint getLocation();
    void resize(unsigned int width, unsigned int height);
private:
    GLuint texture;
    GLint width, height, numberOfChannels;
};

#endif