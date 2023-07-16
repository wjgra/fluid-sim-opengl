#ifndef _FLUID_SHADER_PROGRAM_HPP_
#define _FLUID_SHADER_PROGRAM_HPP_

#ifdef __EMSCRIPTEN__
#include <GL/gl.h>
#include <GLES3/gl3.h>
#else
#include "../include/glad/glad.h"
#endif
  
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector> 

class ShaderProgram
{
public:
    ShaderProgram(const std::string vertexPath, const std::string fragmentPath);
    ~ShaderProgram();
    GLuint getID() const;
    void useProgram();
    GLint getUniformLocation(const std::string &name) const;
    // to do: overloaded functions to set uniform values (e.g. b/o https://registry.khronos.org/OpenGL-Refpages/gl4/html/glUniform.xhtml)
private:
    GLuint programID;
    GLuint compileShader(const char *source, GLenum shaderType);
};
  
#endif