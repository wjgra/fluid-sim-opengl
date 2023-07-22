#include "../include/texture.hpp"

Texture::Texture(unsigned int w, unsigned int h) : width{w}, height{h}{
    glGenTextures(1, &texture);
    //glBindTexture(GL_TEXTURE_2D, texture);
    bind();
    // Temp: different config for blank textures to save into than those used for text
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //const unsigned int tileSize{512};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    //glBindTexture(GL_TEXTURE_2D, 0);
    unbind();
}

Texture::Texture(const std::string& path){
    glGenTextures(1, &texture);
    //glBindTexture(GL_TEXTURE_2D, texture);
    bind();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR/*GL_NEAREST_MIPMAP_NEAREST*/);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR/*GL_NEAREST*/);

    // stbi_set_flip_vertically_on_load(true); 
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &numberOfChannels, 0);
    // change to throw
    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
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
}

void Texture::bind(){
    glBindTexture(GL_TEXTURE_2D, texture);
}

void Texture::unbind(){
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint Texture::getLocation(){
    return texture;
}

void Texture::resize(unsigned int width, unsigned int height){
    bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    unbind();
}