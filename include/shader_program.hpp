#ifndef _FLUID_SHADER_PROGRAM_HPP_
#define _FLUID_SHADER_PROGRAM_HPP_

#ifdef __EMSCRIPTEN__
#include <GL/gl.h>
#include <GLES3/gl3.h>
#else
#include "glad/glad.h"
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
    ShaderProgram(ShaderProgram const&) = delete;
    ShaderProgram(ShaderProgram const&&) = delete;
    ShaderProgram& operator=(ShaderProgram const&) = delete;
    ShaderProgram& operator=(ShaderProgram const&&) = delete;
    ~ShaderProgram();
    GLuint getID() const;
    void useProgram() const;
    GLint getUniformLocation(const std::string &name) const;
private:
    GLuint m_programID;
    GLuint compileShader(const char *source, GLenum shaderType);
};
  
#endif