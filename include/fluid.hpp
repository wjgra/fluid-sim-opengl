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
#include <glm/gtc/type_ptr.hpp>

#include "texture.hpp"
#include "shader_program.hpp"
#include "drawable.hpp"
#include "vertex_data.hpp"

/* 
    M - FluidSimulator: Integrates the fluid each frame, reacting to input provided via public interface.
    V - FluidRenderer: Takes a reference to the levelset texture and renders the fluid and background. It is otherwise unaware of the model.
    C - Fluid: Passes input to the simulator via interface and ensures the renderer has access to the level set.
 */

static int const gridSize = 32;

class FluidSimulator{
    float const gravitationalFieldStrength = 9.81;
    float const fluidDensityRho = 997;
    int const numJacobiIterationsDiffusion = 25;
    int const numJacobiIterationsPressure = 50;
public:
    FluidSimulator();
    void update(unsigned int frameTime);
    bool successfullyInitialised() const;
    GLuint getCurrentLevelSet() const;
    void resetLevelSet() const;
    void updateAppliedForce(glm::vec3 force);
private:
    void initialiseUniforms();
    void initialiseTextures();
    void initialiseFramebufferObjects();
    void integrateFluid(unsigned int frameTime);
    struct SimulatedQuantity{
        GLuint texture;
        GLuint slabFBOs[gridSize];
        void generateTexture(std::vector<float> data, bool scalarQuantity);
        void generateFBOs();
    };
    struct SlabOperation{
        SlabOperation(std::string const& vertexShaderPath, std::string const& fragmentShaderPath, std::vector<std::string> const& textureNames);
        ShaderProgram shader;
        GLuint uniformZSlice, uniformTimeStep;
        DrawableUniformLocations quadUniforms;
    };
    struct InnerSlabOperation : public SlabOperation{
        InnerSlabOperation(std::string const& vertexShaderPath, std::string const& fragmentShaderPath, std::vector<std::string> const& textureNames);
    };
    struct OuterSlabOperation : public SlabOperation{
        OuterSlabOperation(std::string const& vertexShaderPath, std::string const& fragmentShaderPath, std::vector<std::string> const& textureNames);
    };
    void applySlabOp(SlabOperation const& slabOp, SimulatedQuantity const& quantity, unsigned int frameTime, int layerFrom, int layerTo) const;
    void applyInnerSlabOp(InnerSlabOperation const& slabOp, SimulatedQuantity const& quantity, unsigned int frameTime) const;
    void applyOuterSlabOp(OuterSlabOperation const& slabOp, SimulatedQuantity const& quantity, unsigned int frameTime) const;
private:
    bool m_successfullyInitialised;
    Drawable m_quad{std::vector<float>(quadVerts, quadVerts + quadVertsSize), 2u};
    SimulatedQuantity m_velocityCurrent, m_velocityNext;
    SimulatedQuantity m_levelSetCurrent, m_levelSetNext;
    SimulatedQuantity m_pressureCurrent, m_pressureNext;
    SimulatedQuantity m_tempVectorQuantity, m_tempScalarQuantity; // for use in performing iterations
    InnerSlabOperation m_advectionLevelSet, m_advectionVelocity, m_diffusion, m_forceApplication, m_passThrough, m_pressurePoisson, m_divergence, m_removeDivergence;
    OuterSlabOperation m_boundaryVelocity, m_boundaryLevelSet, m_boundaryPressure, m_clearSlabs;
    std::vector<float> m_initialLevelSetData, m_initialVelocityData;
    GLuint uniformAppliedForcePosition, uniformAppliedForce;
    glm::vec3 m_appliedForce;
};

class FluidRenderer{
    float const m_cubeScale = 1.5f;
    float const planeSize = 10.0f;
    int const m_splineResolution = 128;
    glm::vec3 const m_cameraIntialPos = glm::vec3(0.0f, 1.5f, 3.0f);
    unsigned int const m_screenWidth;
    unsigned int const m_screenHeight;
public:
    FluidRenderer(unsigned int width, unsigned int height);
    void updateCamera(float cameraHorizontalRotation, float cameraVerticalRotation);
    void render(GLuint currentLevelSetTexture) const;
    bool successfullyInitialised() const;
private:
    void initialiseShaders();
    void setUpSkybox();
    void renderBackground() const;
    void renderFluid(GLuint currentLevelSetTexture) const;
    void setUpSplines();
private:
    bool m_successfullyInitialised;
    float m_cameraHorizontalRotation;
    float m_cameraVerticalRotation;
    struct Camera{
        Camera(glm::vec3 initialPosition) : position{initialPosition}{}
        glm::vec3 position;
        glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::mat4 viewMatrix;
        float yaw = 0.0f, pitch = 1.0f;
        void updateMatrix();
    } m_camera;
    GLuint m_skyBoxTexture, m_uniformSkyBoxTexture;
    std::vector<std::string> m_skyBoxPaths = {".//skybox//miramar_lf.tga", ".//skybox//miramar_rt.tga",
                                            ".//skybox//miramar_up.tga", ".//skybox//miramar_dn.tga",
                                            ".//skybox//miramar_ft.tga", ".//skybox//miramar_bk.tga"};
    struct RenderTarget{
        RenderTarget(unsigned int width, unsigned int height);
        RenderTarget(RenderTarget const&) = delete;
        RenderTarget(RenderTarget const&&) = delete;
        RenderTarget& operator=(RenderTarget const&) = delete;
        RenderTarget& operator=(RenderTarget const&&) = delete;
        ~RenderTarget();
        GLuint FBO;
        Texture texture;
        GLuint uniformTexture;
        void setUpBuffers();
        void releaseBuffers();
    } m_frontCube, m_backCube;
    Drawable m_cube{std::vector<float>(cubeVerts, cubeVerts + cubeVertsSize), 3u};
    Drawable m_quad{std::vector<float>(quadVerts, quadVerts + quadVertsSize), 2u};
    Drawable m_backgroundPlane{std::vector<float>(backgroundPlaneVerts, backgroundPlaneVerts + backgroundPlaneVertsSize), 2u};
    DrawableUniformLocations m_renderFluidUniforms, m_raycastingPosUniforms, m_backgroundPlaneUniforms;
    GLuint m_uniformLevelSetFluid;
    GLuint m_splineTexture, m_uniformSplineTexture, m_splineDerivTexture, m_uniformSplineDerivTexture;
    ShaderProgram m_backgroundPlaneShader, m_raycastingPosShader, m_renderFluidShader;
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
    void updateForce();
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