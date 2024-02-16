#include "shader_program.hpp"

// To do: move file reading out of the constructor
ShaderProgram::ShaderProgram(const std::string vertexPath, const std::string fragmentPath){

    std::string vertexSource, fragmentSource;
    std::ifstream vertexFile, fragmentFile;

    vertexFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fragmentFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try 
    {
        #ifndef __EMSCRIPTEN__
        vertexFile.open(vertexPath.c_str());
        fragmentFile.open(fragmentPath.c_str());
        #else
        vertexFile.open((".//shaders_web//"+vertexPath.substr(10)).c_str());
        fragmentFile.open((".//shaders_web//"+fragmentPath.substr(10)).c_str());
        #endif

        std::stringstream vertexStream, fragmentStream;

        vertexStream << vertexFile.rdbuf();
        fragmentStream << fragmentFile.rdbuf();		

        vertexFile.close();
        fragmentFile.close();

        vertexSource   = vertexStream.str();
        fragmentSource = fragmentStream.str();		
    }
    catch(std::ifstream::failure const& e)
    {
        std::cerr << "Failed to load shader from file." << std::endl;
    }

    GLuint vertexShaderID = compileShader((const char*)vertexSource.c_str(), GL_VERTEX_SHADER);
    GLuint fragmentShaderID = compileShader((const char*)fragmentSource.c_str(), GL_FRAGMENT_SHADER);


    m_programID = glCreateProgram();

    glAttachShader(m_programID, vertexShaderID);
    glAttachShader(m_programID, fragmentShaderID);
    glLinkProgram(m_programID);

    int success, logLength;
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramiv(m_programID, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> errorLog((logLength > 1)? logLength : 1);
        glGetProgramInfoLog(m_programID, logLength, NULL, errorLog.data());
        std::cout << "Failed to link shader program.\n" << errorLog.data() << std::endl;
    }

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
}

// Compiles an individual shader of the given type (e.g. GL_VERTEX_SHADER)
GLuint ShaderProgram::compileShader(const char *source, GLenum shaderType){
    // Compile shader from source
    unsigned int shaderID;
    shaderID = glCreateShader(shaderType);
    #ifndef __EMSCRIPTEN__
    glShaderSource(shaderID, 1, &source, NULL);
    #else
    glShaderSource(shaderID, 1, (const char**)&source, NULL);
    #endif
    glCompileShader(shaderID);
    // Check for errors in compiling shader
    int success, logLength;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> errorLog((logLength > 1)? logLength : 1);
        glGetShaderInfoLog(shaderID, logLength, NULL, errorLog.data());
        std::cout << "Shader (" << source << ") failed to compile.\n" << errorLog.data() << std::endl;
    }
    return shaderID;
}

ShaderProgram::~ShaderProgram(){
    glDeleteProgram(m_programID);
}

GLuint ShaderProgram::getID() const{
    return m_programID;
}

void ShaderProgram::useProgram() const{
    glUseProgram(m_programID);
}

GLint ShaderProgram::getUniformLocation(const std::string &name) const{
    return glGetUniformLocation(m_programID, name.c_str());
}