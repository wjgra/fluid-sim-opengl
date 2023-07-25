#ifndef _FLUID_FLUID_RENDERER_
#define _FLUID_FLUID_RENDERER_

#ifdef __EMSCRIPTEN__
#include <GL/gl.h>
#include <GLES3/gl3.h>
#else
#include "../include/glad/glad.h"
#endif

#include <SDL.h>

#include <iostream>
#include <vector>
#include <string>

#define GLM_FORCE_PURE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../include/texture.hpp"
#include "../include/shader_program.hpp"
#include <glm/gtc/type_ptr.hpp>

class FluidRenderer{
public:
    FluidRenderer(unsigned int width, unsigned int height);
    // 
    unsigned int const screenWidth, screenHeight;
    float const scale = 1; //scale of cube
    int const gridSize = 32;
    float const gValue = 9.81;
    float const rho = 997;
    // cube location - change this to camera...
    float horizRot = 0.0f;
    float vertRot = 0.0f;
    float horizRotSpeed = glm::radians(1500.0f)*1e-6;
    
    struct Drawable{
        Drawable(std::vector<float> const& verts, unsigned int vertexDimension) : vertices{verts}{setUpBuffers(vertexDimension);};
        ~Drawable(){releaseBuffers();}
        void draw(GLint drawingMode = GL_TRIANGLES);
    private:
        void setUpBuffers(unsigned int vertDim = 3);
        void releaseBuffers();
        std::vector<float> const vertices;
        GLuint VBO, VAO;
    };
    Drawable cube {{
        // Back face
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // SW
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // NE
        0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // SE         
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // NE
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // SW
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // NW
        // Front face
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // SW
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // SE
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // NE
        0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // NE
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // NW
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // SW
        // Left face
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // NE
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // NW
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // SW
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // SW
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // SE
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // NE
        // Right face
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // NW
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // SE
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // NE         
        0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // SE
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // NW
        0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // SW     
        // Bottom face
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // NE
        0.5f, -0.5f, -0.5f,  1.0f, 1.0f, // NW
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // SW
        0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // SW
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // SE
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // NE
        // Top face
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // NW
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // SE
        0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // NE     
        0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // SE
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // NW
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f  // SW  
    }, 3u};
    Drawable quad {{
        1.0f, 0.0f,   1.0f, 0.0f,   // SE
        1.0f, 1.0f,   1.0f, 1.0f,   // NE
        0.0f, 0.0f,   0.0f, 0.0f,   // SW
        1.0f, 1.0f,   1.0f, 1.0f,   // NE
        0.0f, 0.0f,   0.0f, 0.0f,   // SW
        0.0f, 1.0f,   0.0f, 1.0f    // NW
    }, 2u};
    struct Camera{
        glm::vec3 pos = glm::vec3(0.0f, 2.0f, -3.0f);
        glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);

        


        glm::mat4 cameraMat; //???
        float yaw = 0.0f, pitch = 1.0f;
        void updateMatrix(){
            //glm::vec3 dir = glm::normalize(pos - target);
            //glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), dir));
            glm::vec3 up = glm::vec3(0.0f, -1.0f, 0.0f);//glm::cross(dir, right);
            cameraMat = glm::lookAt(pos, target, up);
        }
    } camera;

    struct DrawableUniformLocations{
        GLint modelTrans, projTrans, viewTrans;
    } renderFluidUniforms, raycastingPosUniforms; // Possibly separate into different struct types?

    struct RenderTarget{
        RenderTarget(unsigned int width, unsigned int height) : texture{width, height} {setUpBuffers();};
        ~RenderTarget() {releaseBuffers();}
        GLuint FBO;
        Texture texture;
        GLuint uniformTexture;
        void setUpBuffers(); // genBuffers etc.
        void releaseBuffers();
    } frontCube, backCube;

    struct SimulatedQuantity{ // Should this just be scalar? Is velocity different??
        //SimulatedQuantity(int numberOfComponents, std::vector<float> const& initialData); // 1, 2, 3 or 4 - else error! GL_RED, GL_RGB etc.
        GLuint textureCurrent, uniformCurrent, textureNext, uniformNext;
    } levelSet, velocity, pressure;

    ShaderProgram raycastingPosShader, renderFluidShader;//, integrateFluidShader;

    void frame(unsigned int frameTime);
    void handleEvents(SDL_Event const& event);
private:
    void setDrawableUniformValues();
    void setUpFluidData(); // !!Lots of code duplication here
    void integrateFluid(unsigned int timeStep); // Consider making frame-independent. Should this be a member of simulated quantity? Does it differ for different quantities (e.g. velocity) is number of components an issue?
    void renderFluid();
};

#endif