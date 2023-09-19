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
//#include <algorithm>
#include <chrono>

#define GLM_FORCE_PURE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../include/texture.hpp"
#include "../include/shader_program.hpp"
#include <glm/gtc/type_ptr.hpp>

class FluidRenderer{
public:
    FluidRenderer(unsigned int width, unsigned int height);
    void frame(unsigned int frameTime);
    void handleEvents(SDL_Event const& event);
private:
    void setUpFluidRenderShaders();
    void setUpFluidSimulationTextures();
    void setUpFluidSimulationFBOs();
    void integrateFluid(unsigned int timeStep);
    void renderFluid();
    void renderBackground();

    // Configuration variables
    unsigned int const screenWidth, screenHeight;
    float const scale = 1.5f; //scale of cube
    static int const gridSize = 32;
    float const gValue = 9.81;
    float const rho = 997;
    // cube location - change this to camera...
    float horizRot = 0.0f;
    float vertRot = 0.0f;
    float horizRotSpeed = glm::radians(600.0f)*1e-6;
    float const planeSize = 10.0f;
    float gravityDirection = 0.0f; GLint uniformGravityDirection; 
    float const gravityRotSpeed = glm::radians(6.0f)*1e-5;
    bool gravityRotatingPos = false;
    bool gravityRotatingNeg = false;
    bool resetGravity = false;

    int const numJacobiIterations = 25;//50;
    int const numJacobiIterationsPressure = 75;

    // Timing variables
    int initTime = 0, bgTime = 0, intTime = 0, renTime = 0;
    int frameNo = 0;

    // FOR RENDERING fluid

    struct Camera{
        glm::vec3 pos = glm::vec3(0.0f, 2.0f, 3.0f);
        glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::mat4 viewMatrix;
        float yaw = 0.0f, pitch = 1.0f;
        void updateMatrix(){
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            viewMatrix = glm::lookAt(pos, target, up);
        }
    } camera;

    struct Drawable{
        Drawable(std::vector<float> const& verts, unsigned int vertexDimension);
        ~Drawable();
        void draw(GLint drawingMode = GL_TRIANGLES);
    private:
        void setUpBuffers(unsigned int vertDim = 3);
        void releaseBuffers();
        std::vector<float> const vertices;
        GLuint VBO, VAO;
    };

    struct DrawableUniformLocations{
        GLint modelTrans, projTrans, viewTrans;
    } renderFluidUniforms, raycastingPosUniforms, backgroundPlaneUniforms; // Possibly separate into different struct types?

    struct RenderTarget{
        RenderTarget(unsigned int width, unsigned int height) : texture{width, height} {setUpBuffers();};
        ~RenderTarget() {releaseBuffers();}
        GLuint FBO;
        Texture texture;
        GLuint uniformTexture;
        void setUpBuffers(); // genBuffers etc.
        void releaseBuffers();
    } frontCube, backCube;


GLuint uniformLevelSetFluid; // For renderFluidShader
ShaderProgram backgroundPlaneShader, raycastingPosShader, renderFluidShader;

///////////////////////////////////////////////////////////
// FOR SIMULATING FLUID
// -----------
    

    struct SQ{
        GLuint texture;
        GLuint slabFBOs[gridSize];
        void generateTexture(std::vector<float> data);
        void generateFBOs();
    } velocityCurrent, velocityNext, levelSetCurrent, levelSetNext, pressureCurrent, pressureNext, tempQuantity;

    /// OLD
 /*    struct SimulatedQuantity{ 
        GLuint textureCurrent, textureNext;
    } levelSet, velocity, pressure;
    GLuint textureVelocityTemp; */

    struct SlabOperation{
        SlabOperation(const std::string vertexShaderPath, const std::string fragmentShaderPath);
        ShaderProgram shader;
        GLuint uniformZSlice, uniformTimeStep;
        DrawableUniformLocations quadUniforms;
    private:

    };

    struct innerSlabOp : public SlabOperation{
        innerSlabOp(const std::string vertexShaderPath, const std::string fragmentShaderPath) : 
            SlabOperation(vertexShaderPath, fragmentShaderPath){
                // Z-Slice and timestep
                uniformZSlice = shader.getUniformLocation("zSlice");
                uniformTimeStep = shader.getUniformLocation("timeStep");

                // Textures
                glUniform1i(shader.getUniformLocation("velocityTexture"), 0);
                glUniform1i(shader.getUniformLocation("levelSetTexture"), 1);
                glUniform1i(shader.getUniformLocation("quantityTexture"), 2); // ***Issue: make this configurable

            };
    } advection, diffusion, forceApplication, passThrough, divergence, pressurePoisson, removeDivergence;

    struct outerSlabOp : public SlabOperation{
        outerSlabOp(const std::string vertexShaderPath, const std::string fragmentShaderPath) : 
            SlabOperation(vertexShaderPath, fragmentShaderPath){
                uniformZSlice = shader.getUniformLocation("zSlice");
                glUniform1i(shader.getUniformLocation("quantityTexture"), 2); // ***Issue: make this configurable

            };
    } boundaryVelocity, boundaryLS, boundaryPressure, clearSlabs;

    void applySlabOp(SlabOperation slabOp, SQ quantity, unsigned int frameTime, int layerFrom, int layerTo);
    void applyInnerSlabOp(innerSlabOp slabOp, SQ quantity, unsigned int frameTime);
    void applyOuterSlabOp(outerSlabOp slabOp, SQ quantity, unsigned int frameTime);
/* 
    struct innerSlabOperation{
        innerSlabOperation(const std::string vertexShaderPath, const std::string fragmentShaderPath) :
            innerShader(vertexShaderPath, fragmentShaderPath)
        {   
            // Model matrix
            innerShader.useProgram();

            quadLocations.modelTrans = innerShader.getUniformLocation("model");
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(gridSize, gridSize, 1));
            model = glm::translate(model, glm::vec3(0.0f,0.0f,0.0f));
            glUniformMatrix4fv(quadLocations.modelTrans, 1, GL_FALSE, glm::value_ptr(model));

            // Projection matrix
            glm::mat4 projection = glm::ortho(0.0f, (float)gridSize,  0.0f, (float)gridSize, -1.0f, 1.0f);
            quadLocations.projTrans = innerShader.getUniformLocation("projection");
            glUniformMatrix4fv(quadLocations.projTrans, 1, GL_FALSE, glm::value_ptr(projection));
            
            uniformTS = innerShader.getUniformLocation("timeStep");
            uniformZS = innerShader.getUniformLocation("zSlice");

            glUniform1i(innerShader.getUniformLocation("velocityTexture"), 0);
            glUniform1i(innerShader.getUniformLocation("levelSetTexture"), 1);
            glUniform1i(innerShader.getUniformLocation("quantityTexture"), 2);
        };
        ShaderProgram innerShader;
        GLuint uniformTS, uniformZS, FBOSlice;
        DrawableUniformLocations quadLocations;
    } advection, diffusion, forceApplication, passThrough, divergence, pressurePoisson, removeDivergence;

    void applyInnerSlabOperation(innerSlabOperation& slabOp, unsigned int frameTime, GLuint targetTexture){
        glScissor(1,1,gridSize-2,gridSize-2);
        slabOp.innerShader.useProgram();
        glUniform1f(slabOp.uniformTS, (float)frameTime);

        glGenFramebuffers(1, &(slabOp.FBOSlice));
        glBindFramebuffer(GL_FRAMEBUFFER, slabOp.FBOSlice);
        
        for (int zSlice = 1; zSlice < gridSize-1; ++zSlice){ //not first/last layers
            glUniform1i(slabOp.uniformZS, zSlice);
            glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, targetTexture, 0, zSlice);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                    throw std::string("Failed to initialise framebuffer\n");
            quad.draw(GL_TRIANGLES);
        }
    }

    struct outerSlabOperation{
        outerSlabOperation(const std::string vertexShaderPath, const std::string fragmentShaderPath) :
            outerShader(vertexShaderPath, fragmentShaderPath)
        {   
            // Model matrix
            outerShader.useProgram();

            quadLocations.modelTrans = outerShader.getUniformLocation("model");
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(gridSize, gridSize, 1));
            model = glm::translate(model, glm::vec3(0.0f,0.0f,0.0f));
            glUniformMatrix4fv(quadLocations.modelTrans, 1, GL_FALSE, glm::value_ptr(model));

            // Projection matrix
            glm::mat4 projection = glm::ortho(0.0f, (float)gridSize,  0.0f, (float)gridSize, -1.0f, 1.0f);
            quadLocations.projTrans = outerShader.getUniformLocation("projection");
            glUniformMatrix4fv(quadLocations.projTrans, 1, GL_FALSE, glm::value_ptr(projection));
            
            uniformZS = outerShader.getUniformLocation("zSlice");
            glUniform1i(outerShader.getUniformLocation("quantityTexture"), 2);//pos=2
        };
        ShaderProgram outerShader;
        GLuint uniformZS, FBOSlice;
        DrawableUniformLocations quadLocations;
    } boundaryVelocity, boundaryLS, boundaryPressure, clearSlabs;
    
    void applyOuterSlabOperation(outerSlabOperation& slabOp, GLuint targetTexture){
        glScissor(0,0,gridSize,gridSize); // use discard in shader temporarily
        slabOp.outerShader.useProgram();

        glGenFramebuffers(1, &(slabOp.FBOSlice));
        glBindFramebuffer(GL_FRAMEBUFFER, slabOp.FBOSlice);
        
        for (int zSlice = 0; zSlice < gridSize; ++zSlice){
            glUniform1i(slabOp.uniformZS, zSlice);
            glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, targetTexture, 0, zSlice);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                    throw std::string("Failed to initialise framebuffer\n");
            quad.draw(GL_TRIANGLES);
        }
    } */

    /////////////////
    //// 
    /////////////////////////////////////////////////////////////////////////////////
    
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
    Drawable quad{{
        1.0f, 0.0f,   1.0f, 0.0f,   // SE
        1.0f, 1.0f,   1.0f, 1.0f,   // NE
        0.0f, 0.0f,   0.0f, 0.0f,   // SW
        1.0f, 1.0f,   1.0f, 1.0f,   // NE
        0.0f, 0.0f,   0.0f, 0.0f,   // SW
        0.0f, 1.0f,   0.0f, 1.0f    // NW
    }, 2u};
    Drawable backgroundPlane{{
        /*// First quadrant
        0.0f, 0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f,     0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,     0.0f, 0.0f,
        // Second quadrant
        0.0f, 0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,     0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f, 0.0f,    0.0f, 0.0f,
        // Third quadrant
        0.0f, 0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f, 0.0f,    0.0f, 0.0f,
        0.0f,  0.0f,-1.0f, 0.0f,    0.0f, 0.0f,
        // Fourth quadrant
        0.0f, 0.0f, 0.0f, 1.0f,     0.0f, 0.0f,
        0.0f, 0.0f,-1.0f, 0.0f,     0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 0.0f,     0.0f, 0.0f*/

        -0.5f, -0.5f,      0.0f, 0.0f,
        0.5f,  -0.5f,     1.0f, 0.0f,
        -0.5f,  0.5f,      0.0f, 1.0f,

        -0.5f, 0.5f,     0.0f, 1.0f,
        0.5f, -0.5f,      1.0f, 0.0f,
        0.5f, 0.5,     1.0f, 1.0f


    }, 2u};
    /////////////////////////////////////////////////////////////


};

#endif