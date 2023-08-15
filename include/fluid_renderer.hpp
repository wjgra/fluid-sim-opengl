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
    void frame(unsigned int frameTime);
    void handleEvents(SDL_Event const& event);
private:
    void setDrawableUniformValues();
    void setUpFluidData(); // !!Lots of code duplication here
    void integrateFluid(unsigned int timeStep); // Consider making frame-independent. Should this be a member of simulated quantity? Does it differ for different quantities (e.g. velocity) is number of components an issue?
    void renderFluid();

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
    struct Camera{
        glm::vec3 pos = glm::vec3(0.0f, 2.0f, 3.0f);
        glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::mat4 viewMatrix; //???
        float yaw = 0.0f, pitch = 1.0f;
        void updateMatrix(){
            //glm::vec3 dir = glm::normalize(pos - target);
            //glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), dir));
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);//glm::cross(dir, right);
            viewMatrix = glm::lookAt(pos, target, up);
        }
    } camera;

    struct DrawableUniformLocations{
        GLint modelTrans, projTrans, viewTrans;
    } renderFluidUniforms, raycastingPosUniforms, backgroundPlaneUniforms, integrateFluidUniforms, advectLSUniforms; // Possibly separate into different struct types?

    struct RenderTarget{
        RenderTarget(unsigned int width, unsigned int height) : texture{width, height} {setUpBuffers();};
        ~RenderTarget() {releaseBuffers();}
        GLuint FBO;
        Texture texture;
        GLuint uniformTexture;
        void setUpBuffers(); // genBuffers etc.
        void releaseBuffers();
    } frontCube, backCube;

    // Uniforms may need to be split up - separate shaders for different integrations?
    // One shader: integarte fluid, another: advect scalar quantity
    struct SimulatedQuantity{ // Should this just be scalar? Is velocity different??
        //SimulatedQuantity(int numberOfComponents, std::vector<float> const& initialData); // 1, 2, 3 or 4 - else error! GL_RED, GL_RGB etc.
        GLuint textureCurrent, uniformIntegrateShader, textureNext, uniformAdvectLSShader;
    } levelSet, velocity, pressure;
    GLuint uniformLevelSetFluid; // For renderFluidShader (uniforms above for integration)
    //GLuint uniformVelocityScalar, uniformNextVelocityScalar; // For accessing velocity field when integrating scalars
    GLuint uniformTimeStep;
    GLuint uniformSlice;
    ShaderProgram backgroundPlaneShader, raycastingPosShader, renderFluidShader, integrateFluidShader;


    // -----------
        
    void setUpSlices();
    void releaseSlices();
    GLuint FBOVelocitySlice, FBOLevelSetSlice;
    ShaderProgram advectLevelSetShader;
    GLuint uniformLSVelocity, uniformSliceLS, uniformTimeStepLS;
    GLuint uniformSliceIF, uniformTimeStepIF;


    //void applyInnerSlabOperation(ShaderProgram* const innerShader);
    /*
    class SlabOperation{
    public:
        SlabOperation() = delete;
        // To do: add outershader. alt: inner and outer slab op classes
        SlabOperation(const std::string vertexShaderPath, const std::string fragmentShaderPath) :
            innerShader(vertexShaderPath, fragmentShaderPath)
        {
            uniformTS = innerShader.getUniformLocation("timeStep");
            uniformZS = innerShader.getUniformLocation("zSlice");
        };
        void apply(unsigned int frameTime, GLuint targetTexture){
            innerShader.useProgram();
            glUniform1f(uniformTS, (float)frameTime);

            glGenFramebuffers(1, &FBOSlice);
            glBindFramebuffer(GL_FRAMEBUFFER, FBOSlice);
            
            for (int zSlice = 0; zSlice < gridSize; ++zSlice){
                glUniform1i(uniformZS, zSlice);
                glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, targetTexture, 0, zSlice);
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                        throw std::string("Failed to initialise framebuffer\n");
                quad.draw(GL_TRIANGLES);
            }
        };

    private:
        ShaderProgram innerShader;
        GLuint uniformTS, uniformZS, FBOSlice;
    };// advection, diffusion, 
    */
    struct innerSlabOperation{
        innerSlabOperation(const std::string vertexShaderPath, const std::string fragmentShaderPath) :
            innerShader(vertexShaderPath, fragmentShaderPath)
        {   
            // Model matrix
            innerShader.useProgram();

            quadLocations.modelTrans = innerShader.getUniformLocation("model");
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(gridSize, gridSize, 1));
            model = glm::translate(model, glm::vec3(0.0f,0.0f,0.0f));  // should be (1,1,0) , but without any scaling!
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
    } advection, diffusion, forceApplication;

    void applyInnerSlabOperation(innerSlabOperation& slabOp, unsigned int frameTime, GLuint targetTexture){
        slabOp.innerShader.useProgram();
        glUniform1f(slabOp.uniformTS, (float)frameTime);

        glGenFramebuffers(1, &(slabOp.FBOSlice));
        glBindFramebuffer(GL_FRAMEBUFFER, slabOp.FBOSlice);
        
        for (int zSlice = 0; zSlice < gridSize; ++zSlice){
            glUniform1i(slabOp.uniformZS, zSlice);
            glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, targetTexture, 0, zSlice);
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                    throw std::string("Failed to initialise framebuffer\n");
            quad.draw(GL_TRIANGLES);
        }
    }

};

#endif