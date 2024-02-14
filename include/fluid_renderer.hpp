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
#include <chrono>
#include <stdexcept>

#define GLM_FORCE_PURE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../include/texture.hpp"
#include "../include/shader_program.hpp"
#include <glm/gtc/type_ptr.hpp>

float const cubeVerts[] = {
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
};
unsigned int const cubeVertsSize = std::size(cubeVerts);

float const quadVerts[] = {
    1.0f, 0.0f,   1.0f, 0.0f,   // SE
    1.0f, 1.0f,   1.0f, 1.0f,   // NE
    0.0f, 0.0f,   0.0f, 0.0f,   // SW
    1.0f, 1.0f,   1.0f, 1.0f,   // NE
    0.0f, 0.0f,   0.0f, 0.0f,   // SW
    0.0f, 1.0f,   0.0f, 1.0f    // NW
};
unsigned int const quadVertsSize = std::size(quadVerts);

float const backgroundPlaneVerts[] = {
    -0.5f, -0.5f,      0.0f, 0.0f,
    0.5f,  -0.5f,     1.0f, 0.0f,
    -0.5f,  0.5f,      0.0f, 1.0f,
    -0.5f, 0.5f,     0.0f, 1.0f,
    0.5f, -0.5f,      1.0f, 0.0f,
    0.5f, 0.5,     1.0f, 1.0f
};
unsigned int const backgroundPlaneVertsSize = std::size(backgroundPlaneVerts);
/* 
    M - FluidSimulator: integrates fluid each frame, reacts to input provided via member fns
    V - FluidRenderer: takes a reference to the levelset texture, is otherwise unaware of the model, displays the bg etc
    C- FluidController: passes input to the simulator and ensures the renderer has access to the level set
    Wrapper
 */

struct Drawable{
        Drawable(std::vector<float> const& verts, unsigned int vertexDimension);
        ~Drawable();
        void bindVAO();
        static void unbindVAO(){
            glBindVertexArray(0);
        }
        void draw(GLint drawingMode = GL_TRIANGLES);
    private:
        void setUpBuffers(unsigned int vertDim = 3);
        void releaseBuffers();
        std::vector<float> const vertices;
        GLuint VBO, VAO;
};

struct DrawableUniformLocations{
    GLint modelTrans, projTrans, viewTrans;
};

class FluidSimulator{
    // work out where these are used, then move as appropriate
    Drawable cube {std::vector<float>(cubeVerts, cubeVerts + cubeVertsSize), 3u};
    Drawable quad{std::vector<float>(quadVerts, quadVerts + quadVertsSize), 2u};
    Drawable backgroundPlane{std::vector<float>(backgroundPlaneVerts, backgroundPlaneVerts + backgroundPlaneVertsSize), 2u};

private:
    float const scale = 1.5f; //scale of cube
    static int const gridSize = 32;
    float const gValue = 9.81;
    float const rho = 997;
    int const numJacobiIterations = 25;//30;//25;
    int const numJacobiIterationsPressure = 50;//60;//50

    float horizRot = 0.0f; // this is camera rotation - either update this variable (bad) or let controller update force directly as a member

    float gravityDirection = 0.0f; GLint uniformGravityDirection; 
    float const gravityRotSpeed = glm::radians(6.0f)*1e-5;
    bool gravityRotatingPos = false;
    bool gravityRotatingNeg = false;
    bool resetGravity = false;

    // For force - consider refreshing start point at intervals
    GLuint uniformForcePos, uniformForce;
    bool applyingForce = false;
    int forceMouseStartX, forceMouseStartY;
    int forceMouseEndX, forceMouseEndY;
    bool resetLevelSet = false;
    std::vector<float> levelSetData, velocityData;

    struct SQ{
        GLuint texture;
        GLuint slabFBOs[gridSize];
        void generateTexture(std::vector<float> data, bool scalarQuantity);
        void generateFBOs();
    } velocityCurrent, velocityNext, levelSetCurrent, levelSetNext, pressureCurrent, pressureNext, tempVectorQuantity, tempScalarQuantity;

    struct SlabOperation{
        SlabOperation(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames);
        ShaderProgram shader;
        GLuint uniformZSlice, uniformTimeStep;
        DrawableUniformLocations quadUniforms;
    };

    struct innerSlabOp : public SlabOperation{
        innerSlabOp(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames);
    } advection, advectionVel, diffusion, forceApplication, passThrough, pressurePoisson, divergence, removeDivergence;

    struct outerSlabOp : public SlabOperation{
        outerSlabOp(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames);
    } boundaryVelocity, boundaryLS, boundaryPressure, clearSlabs;

    // Issue: could make these member functions if we make quad static inline (does it need to be const? may have to convert to array)
    void applySlabOp(SlabOperation& slabOp, SQ& quantity, unsigned int frameTime, int layerFrom, int layerTo);
    void applyInnerSlabOp(innerSlabOp& slabOp, SQ& quantity, unsigned int frameTime);
    void applyOuterSlabOp(outerSlabOp& slabOp, SQ& quantity, unsigned int frameTime);

public:
    FluidSimulator();
    void frame(unsigned int frameTime);
    bool successfullyInitialised() const;
    GLuint getCurrentLevelSet() const;
private:
    void initialiseShaders();
    void initialiseTextures();
    void initialiseFramebufferObjects();

    void integrateFluid(unsigned int frameTime);
private:
    bool m_successfullyInitialised;
};

class NewFluidRenderer{
    // work out where these are used, then move as appropriate
    Drawable cube {std::vector<float>(cubeVerts, cubeVerts + cubeVertsSize), 3u};
    Drawable quad{std::vector<float>(quadVerts, quadVerts + quadVertsSize), 2u};
    Drawable backgroundPlane{std::vector<float>(backgroundPlaneVerts, backgroundPlaneVerts + backgroundPlaneVertsSize), 2u};
public:
    NewFluidRenderer(unsigned int width, unsigned int height);

    void frame(unsigned int frameTime, GLuint currentLevelSetTexture);
    bool successfullyInitialised() const;
private:
    void initialiseShaders();
    void renderBackground();
    void renderFluid(GLuint currentLevelSetTexture);
    void setUpSkybox();
    void setUpSplines();
private:
    bool m_successfullyInitialised;
    unsigned int const screenWidth, screenHeight;

    float horizRot = 0.0f;
    float vertRot = 0.0f;
    float horizRotSpeed = glm::radians(1e-5);
    float const planeSize = 10.0f;
    bool cameraRotating = true;

    float const scale = 1.5f; //scale of cube
    static int const gridSize = 32;

    // sky box
    GLuint skyBoxTexture, uniformSkyBoxTexture;
    std::vector<std::string> skyBoxPaths = {".//skybox//miramar_lf.tga", ".//skybox//miramar_rt.tga",
                                            ".//skybox//miramar_up.tga", ".//skybox//miramar_dn.tga",
                                            ".//skybox//miramar_ft.tga", ".//skybox//miramar_bk.tga"};
    
    struct Camera{
        glm::vec3 pos = glm::vec3(0.0f, 2.0f, 3.0f);
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

    GLuint splineTexture, uniformSplineTexture, splineDerivTexture, uniformSplineDerivTexture; // move...
    int const splineRes = 128;

    GLuint uniformLevelSetFluid; // For renderFluidShader
    ShaderProgram backgroundPlaneShader, raycastingPosShader, renderFluidShader;
};

class Fluid{
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
    bool m_successfullyInitialised;
    FluidSimulator m_simulator;
    NewFluidRenderer m_renderer;

};

/* 
class FluidController{
public:
    FluidController() = delete;
    FluidController(FluidSimulator& simulator, NewFluidRenderer& renderer) : m_simulator{simulator}, m_renderer{renderer}{
        try{
            if (!m_simulator.successfullyInitialised()){
                throw std::runtime_error("Failed to create FluidController: must pass a valid simulator instance");
            }
            if (!m_renderer.successfullyInitialised()){
                throw std::runtime_error("Failed to create FluidController: must pass a valid renderer instance");
            }
            m_successfullyInitialised = true;
        }
        catch (std::exception const& e){
            std::cerr << "[ERROR]: " << e.what() << "\n";
            m_successfullyInitialised = false;
        }
    }
    FluidController(FluidController&) = delete;
    FluidController(FluidController&&) = delete;
    FluidController& operator=(FluidController&) = delete;
    FluidController& operator=(FluidController&&) = delete;
    ~FluidController() = default;
public:
    void frame(unsigned int frameTime){
        m_simulator.frame(frameTime);
        m_renderer.frame(0);
    }
    bool successfullyInitialised() const {return m_successfullyInitialised;}
    void handleEvents(SDL_Event const& event);
private:
    bool m_successfullyInitialised;
    FluidSimulator& m_simulator;
    NewFluidRenderer& m_renderer;
};
 */
/* class FluidRenderer{
public:
    FluidRenderer(unsigned int width, unsigned int height);
    void frame(); // new!!!
    void frame(unsigned int frameTime);
    void handleEvents(SDL_Event const& event);
    bool successfullyInitialised() const { return m_successfullyInitialised;}; /////////////
private:
    void setUpFluidRenderShaders();
    void setUpFluidSimulationTextures();
    void setUpFluidSimulationFBOs();
    void integrateFluid(unsigned int timeStep);
    void renderFluid();
    void renderBackground();

    bool m_successfullyInitialised;

    // Configuration variables
    unsigned int const screenWidth, screenHeight;
    float const scale = 1.5f; //scale of cube
    static int const gridSize = 32;
    float const gValue = 9.81;
    float const rho = 997;
    // cube location - change this to camera...
    float horizRot = 0.0f;
    float vertRot = 0.0f;
    float horizRotSpeed = glm::radians(1e-5);
    float const planeSize = 10.0f;
    float gravityDirection = 0.0f; GLint uniformGravityDirection; 
    float const gravityRotSpeed = glm::radians(6.0f)*1e-5;
    bool gravityRotatingPos = false;
    bool gravityRotatingNeg = false;
    bool resetGravity = false;
    bool cameraRotating = true;

    int const numJacobiIterations = 25;//30;//25;
    int const numJacobiIterationsPressure = 50;//60;//50

    // Timing variables
    int initTime = 0, bgTime = 0, intTime = 0, renTime = 0;
    int frameNo = 0;

    // Sky box
    GLuint skyBoxTexture, uniformSkyBoxTexture;
    std::vector<std::string> skyBoxPaths = {".//skybox//miramar_lf.tga", ".//skybox//miramar_rt.tga",
                                            ".//skybox//miramar_up.tga", ".//skybox//miramar_dn.tga",
                                            ".//skybox//miramar_ft.tga", ".//skybox//miramar_bk.tga"};
    void setUpSkybox();


    // For force - consider refreshing start point at intervals
    GLuint uniformForcePos, uniformForce;
    bool applyingForce = false;
    int forceMouseStartX, forceMouseStartY;
    int forceMouseEndX, forceMouseEndY;
    bool resetLevelSet = false;
    std::vector<float> levelSetData, velocityData;

    //GLuint uniformGravityDirAdv;
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
        void bindVAO();
        static void unbindVAO();
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

    GLuint splineTexture, uniformSplineTexture, splineDerivTexture, uniformSplineDerivTexture; // move...
    int const splineRes = 128;
    void setUpSplines();

    GLuint uniformLevelSetFluid; // For renderFluidShader
    ShaderProgram backgroundPlaneShader, raycastingPosShader, renderFluidShader;

    ///////////////////////////////////////////////////////////
    // FOR SIMULATING FLUID
    // -----------
    

    struct SQ{
        GLuint texture;
        GLuint slabFBOs[gridSize];
        void generateTexture(std::vector<float> data, bool scalarQuantity);
        void generateFBOs();
    } velocityCurrent, velocityNext, levelSetCurrent, levelSetNext, pressureCurrent, pressureNext, tempVectorQuantity, tempScalarQuantity;


    struct SlabOperation{
        SlabOperation(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames);
        ShaderProgram shader;
        GLuint uniformZSlice, uniformTimeStep;
        DrawableUniformLocations quadUniforms;
    };

    struct innerSlabOp : public SlabOperation{
        innerSlabOp(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames) : 
            SlabOperation(vertexShaderPath, fragmentShaderPath, textureNames){
                // Z-Slice and timestep
                uniformZSlice = shader.getUniformLocation("zSlice");
                uniformTimeStep = shader.getUniformLocation("timeStep");

            };
    } advection, advectionVel, diffusion, forceApplication, passThrough, pressurePoisson, divergence, removeDivergence;

    struct outerSlabOp : public SlabOperation{
        outerSlabOp(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames) : 
            SlabOperation(vertexShaderPath, fragmentShaderPath, textureNames){
                uniformZSlice = shader.getUniformLocation("zSlice");

            };
    } boundaryVelocity, boundaryLS, boundaryPressure, clearSlabs;

    // Issue: could make these member functions if we make quad static inline (does it need to be const? may have to convert to array)
    void applySlabOp(SlabOperation& slabOp, SQ& quantity, unsigned int frameTime, int layerFrom, int layerTo);
    void applyInnerSlabOp(innerSlabOp& slabOp, SQ& quantity, unsigned int frameTime);
    void applyOuterSlabOp(outerSlabOp& slabOp, SQ& quantity, unsigned int frameTime);

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

        -0.5f, -0.5f,      0.0f, 0.0f,
        0.5f,  -0.5f,     1.0f, 0.0f,
        -0.5f,  0.5f,      0.0f, 1.0f,

        -0.5f, 0.5f,     0.0f, 1.0f,
        0.5f, -0.5f,      1.0f, 0.0f,
        0.5f, 0.5,     1.0f, 1.0f


    }, 2u};
    /////////////////////////////////////////////////////////////


}; */

#endif