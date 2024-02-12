#include "../include/texture.hpp"

Texture::Texture(unsigned int w, unsigned int h, bool useNearest) : m_width(w), m_height(h){
    glGenTextures(1, &m_texture);
    bind();
    if (!useNearest){
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    unbind();
}

Texture::Texture(const std::string& path){
    glGenTextures(1, &m_texture);
    bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // stbi_set_flip_vertically_on_load(true); 
    unsigned char *data = stbi_load(path.c_str(), &m_width, &m_height, &m_numberOfChannels, 0);
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture " << path << "\n";
    }
    stbi_image_free(data);
    unbind();
}

Texture::~Texture(){
    glDeleteTextures(1, &m_texture);
}

void Texture::bind() const{
    glBindTexture(GL_TEXTURE_2D, m_texture);
}

void Texture::unbind() const{
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint Texture::getLocation() const{
    return m_texture;
}

void Texture::resize(unsigned int width, unsigned int height){
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    m_width = width;
    m_height = height;
    unbind();
}