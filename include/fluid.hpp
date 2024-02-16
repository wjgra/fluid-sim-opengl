#ifndef _FLUID_FLUID_HPP_
#define _FLUID_FLUID_HPP_

#ifdef __EMSCRIPTEN__
#include <GL/gl.h>
#include <GLES3/gl3.h>
#else
#include "glad/glad.h"
#endif

#include <SDL.h>

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <stdexcept>
#include <numbers>

#define GLM_FORCE_PURE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "texture.hpp"
#include "shader_program.hpp"
#include <glm/gtc/type_ptr.hpp>

#include "drawable.hpp"
#include "vertex_data.hpp"

/* 
    M - FluidSimulator: integrates fluid each frame, reacts to input provided via member fns
    V - FluidRenderer: takes a reference to the levelset texture, is otherwise unaware of the model, displays the bg etc
    C - Fluid: passes input to the simulator and ensures the renderer has access to the level set
 */

static int const gridSize = 32;

class FluidSimulator{
    float const gravitationalFieldStrength = 9.81;
    float const fluidDensityRho = 997;
    int const numJacobiIterationsDiffusion = 25;
    int const numJacobiIterationsPressure = 50;

    float horizRot = 0.0f; // this is camera rotation - either update this variable (bad) or let controller update force directly as a member

    GLuint uniformForcePos, uniformForce;
    bool applyingForce = false;
    int forceMouseStartX, forceMouseStartY;
    int forceMouseEndX, forceMouseEndY;
    /////////////////////
public:
    FluidSimulator();
    void update(unsigned int frameTime);
    bool successfullyInitialised() const;
    GLuint getCurrentLevelSet() const;
    void resetLevelSet() const;
private:
    void initialiseShaders();
    void initialiseTextures();
    void initialiseFramebufferObjects();
    void integrateFluid(unsigned int frameTime);
    struct SimulatedQuantity{
        GLuint m_texture;
        GLuint m_slabFBOs[gridSize];
        void generateTexture(std::vector<float> data, bool scalarQuantity);
        void generateFBOs();
    };
    struct SlabOperation{
        SlabOperation(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames);
        ShaderProgram shader;
        GLuint uniformZSlice, uniformTimeStep;
        DrawableUniformLocations quadUniforms;
    };
    struct InnerSlabOp : public SlabOperation{
        InnerSlabOp(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames);
    };
    struct OuterSlabOp : public SlabOperation{
        OuterSlabOp(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames);
    };
    void applySlabOp(SlabOperation& slabOp, SimulatedQuantity& quantity, unsigned int frameTime, int layerFrom, int layerTo);
    void applyInnerSlabOp(InnerSlabOp& slabOp, SimulatedQuantity& quantity, unsigned int frameTime);
    void applyOuterSlabOp(OuterSlabOp& slabOp, SimulatedQuantity& quantity, unsigned int frameTime);
private:
    bool m_successfullyInitialised;
    Drawable quad{std::vector<float>(quadVerts, quadVerts + quadVertsSize), 2u};
    SimulatedQuantity m_velocityCurrent, m_velocityNext;
    SimulatedQuantity m_levelSetCurrent, m_levelSetNext;
    SimulatedQuantity m_pressureCurrent, m_pressureNext;
    SimulatedQuantity m_tempVectorQuantity, m_tempScalarQuantity; // for use in performing iterations
    InnerSlabOp m_advectionLevelSet, m_advectionVelocity, m_diffusion, m_forceApplication, m_passThrough, m_pressurePoisson, m_divergence, m_removeDivergence;
    OuterSlabOp m_boundaryVelocity, m_boundaryLevelSet, m_boundaryPressure, m_clearSlabs;
    std::vector<float> m_initialLevelSetData, m_initialVelocityData;
};

class FluidRenderer{
    float const m_cubeScale = 1.5f;
public:
    FluidRenderer(unsigned int width, unsigned int height);
    void updateCamera(float cameraHorizontalRotation, float cameraVerticalRotation);
    void render(unsigned int frameTime, GLuint currentLevelSetTexture);
    bool successfullyInitialised() const;
private:
    void initialiseShaders();
    void renderBackground();
    void renderFluid(GLuint currentLevelSetTexture);
    void setUpSkybox();
    void setUpSplines();
private:
    bool m_successfullyInitialised;
    unsigned int const m_screenWidth;
    unsigned int const m_screenHeight;

    // some of these may be moved to controller...
    float horizRot;
    float vertRot;
   // float horizRotSpeed = glm::radians(1e-5);
    float const planeSize = 10.0f;
    //bool cameraRotating = true;

    

    // sky box
    GLuint skyBoxTexture, uniformSkyBoxTexture;
    std::vector<std::string> skyBoxPaths = {".//skybox//miramar_lf.tga", ".//skybox//miramar_rt.tga",
                                            ".//skybox//miramar_up.tga", ".//skybox//miramar_dn.tga",
                                            ".//skybox//miramar_ft.tga", ".//skybox//miramar_bk.tga"};
    
    glm::vec3 const m_cameraIntialPos = glm::vec3(0.0f, 1.5f, 3.0f);
    struct Camera{
        Camera(glm::vec3 initPos) : pos{initPos}{}
        glm::vec3 pos;
        glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::mat4 viewMatrix;
        float yaw = 0.0f, pitch = 1.0f;
        void updateMatrix();
    } camera;

    DrawableUniformLocations renderFluidUniforms, raycastingPosUniforms, backgroundPlaneUniforms; // Possibly separate into different struct types?

    struct RenderTarget{
        RenderTarget(unsigned int width, unsigned int height);
        ~RenderTarget();
        GLuint FBO;
        Texture texture;
        GLuint uniformTexture;
        void setUpBuffers();
        void releaseBuffers();
    } frontCube, backCube;

    Drawable cube {std::vector<float>(cubeVerts, cubeVerts + cubeVertsSize), 3u};
    Drawable quad{std::vector<float>(quadVerts, quadVerts + quadVertsSize), 2u};
    Drawable backgroundPlane{std::vector<float>(backgroundPlaneVerts, backgroundPlaneVerts + backgroundPlaneVertsSize), 2u};
    
    GLuint splineTexture, uniformSplineTexture, splineDerivTexture, uniformSplineDerivTexture; // move...
    int const splineRes = 128;

    GLuint uniformLevelSetFluid; // For renderFluidShader
    ShaderProgram backgroundPlaneShader, raycastingPosShader, renderFluidShader;
};

class Fluid{
    float const m_cameraRotationSpeed = glm::radians(2e-5);
    float const m_cameraMaxVerticalRotation = 0.2 * std::numbers::pi_v<float>;
    float const m_cameraMinVerticalRotation = -0.2 * std::numbers::pi_v<float>;
public:
    Fluid() = delete;
    Fluid(unsigned int w, unsigned int h);
    Fluid(Fluid&) = delete;
    Fluid(Fluid&&) = delete;
    Fluid& operator=(Fluid&) = delete;
    Fluid& operator=(Fluid&&) = delete;
    ~Fluid() = default;
    bool successfullyInitialised() const;
    void handleEvents(SDL_Event const& event);
    void frame(unsigned int frameTime);
private:
    void updateCamera(unsigned int frameTime);
    bool m_successfullyInitialised;
    FluidSimulator m_simulator;
    FluidRenderer m_renderer;
    int m_cameraHorizontalRotationDirection;
    float m_cameraHorizontalRotation;
    int m_cameraVerticalRotationDirection;
    float m_cameraVerticalRotation;
    bool m_applyingForce;
    int m_forceMouseStartX, m_forceMouseStartY;
    int m_forceMouseEndX, m_forceMouseEndY;
};
#endif