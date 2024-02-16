#include "fluid.hpp"

FluidSimulator::FluidSimulator() : 
    m_advectionLevelSet(".//shaders//slab_operation.vert", ".//shaders//advect_quantity.frag", {"velocityTexture", "quantityTexture"}),
    m_advectionVelocity(".//shaders//slab_operation.vert", ".//shaders//advect_velocity.frag", {"velocityTexture", "quantityTexture"}),
    m_diffusion(".//shaders//slab_operation.vert", ".//shaders//diffuse_quantity.frag", {"quantityTexture"}),
    m_forceApplication(".//shaders//slab_operation.vert", ".//shaders//apply_force_to_velocity.frag", {"velocityTexture", "levelSetTexture"}),
    m_passThrough(".//shaders//slab_operation.vert", ".//shaders//pass_through.frag", {"quantityTexture"}),
    m_pressurePoisson(".//shaders//slab_operation.vert", ".//shaders//pressure_poisson.frag", {"pressureTexture", "levelSetTexture", "divergenceTexture"}),
    m_divergence(".//shaders//slab_operation.vert", ".//shaders//divergence.frag", {"velocityTexture"}),
    m_removeDivergence(".//shaders//slab_operation.vert", ".//shaders//remove_divergence.frag", {"velocityTexture", "pressureTexture"}),
    m_boundaryVelocity(".//shaders//slab_operation.vert", ".//shaders//boundary_velocity.frag", {"velocityTexture"}),
    m_boundaryLevelSet(".//shaders//slab_operation.vert", ".//shaders//boundary_levelset.frag", {"levelSetTexture"}),
    m_boundaryPressure(".//shaders//slab_operation.vert", ".//shaders//boundary_pressure.frag", {"pressureTexture"}),
    m_clearSlabs(".//shaders//slab_operation.vert", ".//shaders//clear_slabs.frag", {}),
    m_appliedForce{0.0f, 0.0f, 0.0f}
{
    try{
        m_successfullyInitialised = false;
        initialiseUniforms();
        initialiseTextures();
        initialiseFramebufferObjects();
        m_successfullyInitialised = true;
    }
    catch (std::exception const& e){
        std::cerr << "[ERROR]: " << e.what() << "\n";
        m_successfullyInitialised = false;
    }
}

void FluidSimulator::update(unsigned int frameTime){
    integrateFluid(frameTime);
}

bool FluidSimulator::successfullyInitialised() const {
    return m_successfullyInitialised;
}

GLuint FluidSimulator::getCurrentLevelSet() const{
    return m_levelSetCurrent.texture;
}

void FluidSimulator::resetLevelSet() const{
    glBindTexture(GL_TEXTURE_3D, m_levelSetCurrent.texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, m_initialLevelSetData.data());
    glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, gridSize, gridSize, gridSize, 0, GL_RGB, GL_FLOAT, m_initialVelocityData.data());
}

void FluidSimulator::updateAppliedForce(glm::vec3 force){
    m_appliedForce = force;
}

void FluidSimulator::initialiseUniforms(){
    m_forceApplication.shader.useProgram();
    uniformAppliedForce = m_forceApplication.shader.getUniformLocation("extForce");
    glUniform3f(uniformAppliedForce, 0.0f, 0.0f, 0.0f);
    uniformAppliedForcePosition = m_forceApplication.shader.getUniformLocation("extForcePos");
    glUniform3f(uniformAppliedForce, 0.5f, 0.15f, 0.5f);
}

void FluidSimulator::initialiseTextures(){
    glActiveTexture(GL_TEXTURE0 + 0);
    // Level set - initial surface at z = 0.5f
    // Takes the value of zero on air-water and box-water interfaces
    // ***Issue: Should be signed distance field, but using 0.5f outside due to pressure issue
    m_initialLevelSetData = std::vector<float>(gridSize*gridSize*gridSize, 0.0f);
    
    for (int k = 0; k < gridSize; ++k){
    for (int j = 0 ; j < gridSize; ++j){
            for (int i = 0; i < gridSize; ++i){
                //location of (i,j,k) in texture data
                int index = gridSize * gridSize * k + gridSize * j + i;
                m_initialLevelSetData[index] = j - gridSize/2;
            }
        }
    }
    
    m_levelSetCurrent.generateTexture(m_initialLevelSetData, true);
    m_levelSetNext.generateTexture(m_initialLevelSetData, true);

    // Velocity - initially zero everywhere
    m_initialVelocityData = std::vector<float>(4*gridSize*gridSize*gridSize, 0.0f);

    m_velocityCurrent.generateTexture(m_initialVelocityData, false);
    m_velocityNext.generateTexture(m_initialVelocityData, false);
    
    // Pressure - initially zero 
    std::vector<float> tempPressureData(gridSize*gridSize*gridSize, 0.0f);

    m_pressureCurrent.generateTexture(tempPressureData, true); // convert to scalar once m_tempVectorQuantity separated for vel/pressure
    m_pressureNext.generateTexture(tempPressureData, true);

    // Temporary set of buffers for use in Jacobi iteration
    m_tempVectorQuantity.generateTexture(m_initialVelocityData, false);
    m_tempScalarQuantity.generateTexture(tempPressureData, true);
}

void FluidSimulator::initialiseFramebufferObjects(){
    m_levelSetCurrent.generateFBOs();
    m_levelSetNext.generateFBOs();
    m_velocityCurrent.generateFBOs();
    m_velocityNext.generateFBOs();
    m_pressureCurrent.generateFBOs();
    m_pressureNext.generateFBOs();
    m_tempVectorQuantity.generateFBOs();
    m_tempScalarQuantity.generateFBOs();
}

void FluidSimulator::integrateFluid(unsigned int frameTime){
    glDisable(GL_BLEND);
    glViewport(0,0,gridSize, gridSize);
    glEnable(GL_SCISSOR_TEST);
    
    // Apply force to velocity
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.texture);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, m_levelSetCurrent.texture);

    m_forceApplication.shader.useProgram();
    glUniform3fv(uniformAppliedForce, 1, glm::value_ptr(m_appliedForce));

    applyInnerSlabOp(m_forceApplication, m_velocityNext, frameTime);
    std::swap(m_velocityCurrent, m_velocityNext);

    // Velocity BC
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.texture);
    applyOuterSlabOp(m_boundaryVelocity, m_velocityNext, frameTime);
    std::swap(m_velocityCurrent, m_velocityNext);

    // Advect Velocity
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.texture);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.texture);
    applyInnerSlabOp(m_advectionVelocity, m_velocityNext, frameTime);

    // Advect Level Set using old velocity (but with corrected BC)
    glBindTexture(GL_TEXTURE_3D, m_levelSetCurrent.texture);
    applyInnerSlabOp(m_advectionLevelSet, m_levelSetNext, frameTime);

    // Put advected velocity in current
    std::swap(m_velocityCurrent, m_velocityNext);

    // Re-bind velocity    
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.texture);

    // Pass through current velocity to temp velocity, which is used as 0th iteration
    applyInnerSlabOp(m_passThrough, m_tempVectorQuantity, frameTime);
    
    // Re-bind velocity as quantity to be altered in pos = 2
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.texture);

    // Diffuse velocity
    glActiveTexture(GL_TEXTURE0 + 0);
    for (int i = 0; i < numJacobiIterationsDiffusion; ++i){
        glBindTexture(GL_TEXTURE_3D, m_tempVectorQuantity.texture);
        // Render into next velocity (kth iterate is in temp, k+1th in next)
        applyInnerSlabOp(m_diffusion, m_velocityNext, frameTime);
        // swap next and temp velocity, then iterate 
        std::swap(m_velocityNext, m_tempVectorQuantity);
        // Velocity BC - do we need to apply this every iteration?
        glBindTexture(GL_TEXTURE_3D, m_tempVectorQuantity.texture);
        applyOuterSlabOp(m_boundaryVelocity, m_velocityNext, frameTime);
        std::swap(m_velocityNext, m_tempVectorQuantity);
    }
        
    std::swap(m_velocityCurrent, m_tempVectorQuantity); // Swap final iteration into current velocity

    // *Remove divergence from velocity*

    // Apply velocity BC (must be done to ensure correct divergence at bdries)
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.texture);
    applyOuterSlabOp(m_boundaryVelocity, m_velocityNext, frameTime);
    std::swap(m_velocityCurrent, m_velocityNext);

    // Compute div of currentVelocity (store in textureVelocityTemp)
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.texture);
    applyInnerSlabOp(m_divergence, m_tempScalarQuantity, frameTime);

    /* // Clear pressure texture
    applyOuterSlabOp(m_clearSlabs, m_pressureNext, frameTime);
    std::swap(m_pressureCurrent, m_pressureNext); */

    // Solve Poisson eqn 
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, m_levelSetCurrent.texture);
    for (int i = 0; i < numJacobiIterationsPressure; ++i){
        // Pressure BC
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, m_pressureCurrent.texture);
        applyOuterSlabOp(m_boundaryPressure, m_pressureNext, frameTime);
        std::swap(m_pressureCurrent, m_pressureNext);

        // Iteration (kth iteration in current, k+1th in next)
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, m_pressureCurrent.texture); // pressure
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_3D, m_tempScalarQuantity.texture); // div(velocity)
        applyInnerSlabOp(m_pressurePoisson, m_pressureNext, frameTime);
        std::swap(m_pressureCurrent, m_pressureNext);
    }

    // Subtract grad(pressure) from currentVelocity
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.texture);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, m_pressureCurrent.texture);
    applyInnerSlabOp(m_removeDivergence, m_velocityNext, frameTime);
    std::swap(m_velocityCurrent, m_velocityNext);

    //Level set BC
    std::swap(m_levelSetCurrent, m_levelSetNext);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, m_levelSetCurrent.texture);
    applyOuterSlabOp(m_boundaryLevelSet, m_levelSetNext, frameTime);
    std::swap(m_levelSetCurrent, m_levelSetNext);

    // Tidy up
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0 + 0);
    glEnable(GL_BLEND);
}

// Generates a new 3D floating-point texture with the given input as the initial data
void FluidSimulator::SimulatedQuantity::generateTexture(std::vector<float> data, bool scalarQuantity = false){
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_3D, texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    if (!scalarQuantity){
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, gridSize, gridSize, gridSize, 0, GL_RGB, GL_FLOAT, data.data());
    }
    else{
        glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, data.data());
    }
    glBindTexture(GL_TEXTURE_3D, 0);
}

// Generates an array of FBOs for using slab operations to render into the simulated quantity texture
void FluidSimulator::SimulatedQuantity::generateFBOs(){
    for (int zSlice = 0; zSlice < gridSize; ++zSlice){
        glGenFramebuffers(1, &(slabFBOs[zSlice]));
        glBindFramebuffer(GL_FRAMEBUFFER, slabFBOs[zSlice]);
        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, texture, 0, zSlice);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::runtime_error("Failed to initialise framebuffer\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FluidSimulator::SlabOperation::SlabOperation(std::string const& vertexShaderPath, std::string const& fragmentShaderPath, std::vector<std::string> const& textureNames) :
    shader(vertexShaderPath, fragmentShaderPath)
{   
    shader.useProgram();

    // Model matrix
    quadUniforms.m_modelTransformation = shader.getUniformLocation("model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(gridSize, gridSize, 1));
    model = glm::translate(model, glm::vec3(0.0f,0.0f,0.0f));
    glUniformMatrix4fv(quadUniforms.m_modelTransformation, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    glm::mat4 projection = glm::ortho(0.0f, (float)gridSize,  0.0f, (float)gridSize, -1.0f, 1.0f);
    quadUniforms.m_projectionTransformation = shader.getUniformLocation("projection");
    glUniformMatrix4fv(quadUniforms.m_projectionTransformation, 1, GL_FALSE, glm::value_ptr(projection));
    
    for (unsigned int i = 0 ; i < textureNames.size() ; ++i){
        if (textureNames[i].length() != 0){
            glUniform1i(shader.getUniformLocation(textureNames[i]), i);
        }
    }
}

FluidSimulator::InnerSlabOperation::InnerSlabOperation(std::string const& vertexShaderPath, std::string const& fragmentShaderPath, std::vector<std::string> const& textureNames) : 
    SlabOperation(vertexShaderPath, fragmentShaderPath, textureNames)
{
    uniformZSlice = shader.getUniformLocation("zSlice");
    uniformTimeStep = shader.getUniformLocation("timeStep");
}

FluidSimulator::OuterSlabOperation::OuterSlabOperation(std::string const& vertexShaderPath, std::string const& fragmentShaderPath, std::vector<std::string> const& textureNames) : 
    SlabOperation(vertexShaderPath, fragmentShaderPath, textureNames)
{
    uniformZSlice = shader.getUniformLocation("zSlice");
}

void FluidSimulator::applySlabOp(SlabOperation const& slabOp, SimulatedQuantity const& quantity, unsigned int frameTime, int layerFrom, int layerTo) const{
    m_quad.bindVAO();
    slabOp.shader.useProgram();
    glUniform1f(slabOp.uniformTimeStep, (float)frameTime);
    for (int zSlice = layerFrom; zSlice < layerTo; ++zSlice){
        glBindFramebuffer(GL_FRAMEBUFFER, quantity.slabFBOs[zSlice]);
        glUniform1f(slabOp.uniformZSlice, (float)zSlice);
        m_quad.draw(GL_TRIANGLES);
    }
}

void FluidSimulator::applyInnerSlabOp(InnerSlabOperation const& slabOp, SimulatedQuantity const& quantity, unsigned int frameTime) const{
    glScissor(1,1,gridSize-2,gridSize-2);
    applySlabOp(slabOp, quantity, frameTime, 1, gridSize-1);
}

void FluidSimulator::applyOuterSlabOp(OuterSlabOperation const& slabOp, SimulatedQuantity const& quantity, unsigned int frameTime) const{
    // Issue: probably more efficient to render four quads
    glScissor(0,0,gridSize,gridSize);
    applySlabOp(slabOp, quantity, frameTime, 0, gridSize);
}

FluidRenderer::FluidRenderer(unsigned int width, unsigned int height) : 
    m_screenWidth{width}, m_screenHeight{height}, 
    m_cameraHorizontalRotation{0.0f}, m_cameraVerticalRotation{0.0f},
    m_camera(m_cameraIntialPos),
    m_frontCube{width, height},
    m_backCube{width, height},
    m_backgroundPlaneShader{".//shaders//background_plane.vert", ".//shaders//background_plane.frag"},
    m_raycastingPosShader(".//shaders//raycasting_pos.vert", ".//shaders//raycasting_pos.frag"),
    m_renderFluidShader(".//shaders//fluid.vert", ".//shaders//fluid.frag")
{
    try{
        initialiseShaders();
        setUpSkybox();
        m_successfullyInitialised = true;
    }
    catch (std::exception const& e){
        std::cerr << "[ERROR]: " << e.what() << "\n";
        m_successfullyInitialised = false;
    }
}

void FluidRenderer::updateCamera(float cameraHorizontalRotation, float cameraVerticalRotation){
    if (m_cameraHorizontalRotation != cameraHorizontalRotation || m_cameraVerticalRotation != cameraVerticalRotation){
        m_cameraHorizontalRotation = cameraHorizontalRotation;
        m_cameraVerticalRotation = cameraVerticalRotation;        
        m_camera.position = glm::vec3(glm::rotate(glm::mat4(1.0f), m_cameraHorizontalRotation, glm::vec3(0.0f, 1.0f, 0.0f))
                     * glm::rotate(glm::mat4(1.0f), m_cameraVerticalRotation, glm::vec3(1.0f, 0.0f, 0.0f))
                     * glm::vec4(0.0f, 1.5f, 3.0f, 1.0f));
        m_camera.updateMatrix();
    }
}

void FluidRenderer::render(GLuint currentLevelSetTexture) const{
    glDisable(GL_CULL_FACE); // Check...
    glViewport(0,0,m_screenWidth,m_screenHeight);
    renderBackground();
    renderFluid(currentLevelSetTexture);
}

bool FluidRenderer::successfullyInitialised() const {
    return m_successfullyInitialised;
}

void FluidRenderer::initialiseShaders(){
    // Get uniform locations and set values for raycastingPosShader
    m_raycastingPosShader.useProgram();
    
    // Model matrix
    m_raycastingPosUniforms.m_modelTransformation = m_raycastingPosShader.getUniformLocation("model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(m_cubeScale, m_cubeScale, m_cubeScale));
    glUniformMatrix4fv(m_raycastingPosUniforms.m_modelTransformation, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    m_raycastingPosUniforms.m_projectionTransformation = m_raycastingPosShader.getUniformLocation("projection");
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)m_screenWidth/(float)m_screenHeight, 0.1f, 100.0f);
    glUniformMatrix4fv(m_raycastingPosUniforms.m_projectionTransformation, 1, GL_FALSE, glm::value_ptr(projection));

    // View matrix
    m_raycastingPosUniforms.m_viewTransformation = m_raycastingPosShader.getUniformLocation("view");
    m_camera.updateMatrix();
    //glm::mat4 view = m_camera.viewMatrix;//::lookAt(glm::vec3(0.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 2.0f, 2.0f));
    glUniformMatrix4fv(m_raycastingPosUniforms.m_viewTransformation, 1, GL_FALSE, glm::value_ptr(m_camera.viewMatrix));
    
    // Get uniform locations and set values for backgroundPlaneShader
    m_backgroundPlaneShader.useProgram();
    
    // Model matrix
    m_backgroundPlaneUniforms.m_modelTransformation = m_backgroundPlaneShader.getUniformLocation("model");
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(planeSize, planeSize, planeSize));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, (-0.5f + 1.0f / gridSize) * m_cubeScale / planeSize)); // Fluid is not in outer cells - translate to sit on plane surface
    glUniformMatrix4fv(m_backgroundPlaneUniforms.m_modelTransformation, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    m_backgroundPlaneUniforms.m_projectionTransformation = m_backgroundPlaneShader.getUniformLocation("projection");
    glUniformMatrix4fv(m_backgroundPlaneUniforms.m_projectionTransformation, 1, GL_FALSE, glm::value_ptr(projection));

    // View matrix
    m_backgroundPlaneUniforms.m_viewTransformation = m_backgroundPlaneShader.getUniformLocation("view");
    glUniformMatrix4fv(m_backgroundPlaneUniforms.m_viewTransformation, 1, GL_FALSE, glm::value_ptr(m_camera.viewMatrix));

    // Get uniform locations and set values for renderFluidShader - note symmetry with above (can we condense?)
    m_renderFluidShader.useProgram();

    // Model matrix
    m_renderFluidUniforms.m_modelTransformation = m_renderFluidShader.getUniformLocation("model");
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(m_screenWidth, m_screenHeight, 1)); 
    model = glm::translate(model, glm::vec3(0, 0, 0.0f));
    glUniformMatrix4fv(m_renderFluidUniforms.m_modelTransformation, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    projection = glm::ortho(0.0f, (float)m_screenWidth,  0.0f, (float)m_screenHeight, -1.0f, 1.0f);
    m_renderFluidUniforms.m_projectionTransformation = m_renderFluidShader.getUniformLocation("projection");
    glUniformMatrix4fv(m_renderFluidUniforms.m_projectionTransformation, 1, GL_FALSE, glm::value_ptr(projection));

    // No view matrix required in orthogonal projection

    // Set up uniforms for 'cube vector' textures
    m_frontCube.uniformTexture =  m_renderFluidShader.getUniformLocation("frontTexture");
    m_backCube.uniformTexture = m_renderFluidShader.getUniformLocation("backTexture");
    m_renderFluidShader.useProgram();
    glUniform1i(m_frontCube.uniformTexture, 0);
    glUniform1i(m_backCube.uniformTexture, 1);

    // Set up uniform for level set texture
    m_uniformLevelSetFluid = m_renderFluidShader.getUniformLocation("levelSetTexture");
    glUniform1i(m_uniformLevelSetFluid, 2);

    setUpSplines(); // For use in tri-cubic interpolation of normals
    m_uniformSplineTexture = m_renderFluidShader.getUniformLocation("splineTexture");
    glUniform1i(m_uniformSplineTexture, 3);
    m_uniformSplineDerivTexture = m_renderFluidShader.getUniformLocation("splineDerivTexture");
    glUniform1i(m_uniformSplineDerivTexture, 4);

    setUpSkybox();
    m_uniformSkyBoxTexture = m_renderFluidShader.getUniformLocation("skyBoxTexture");
    glUniform1i(m_uniformSkyBoxTexture, 5);
}

void FluidRenderer::setUpSkybox(){
    glGenTextures(1, &m_skyBoxTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyBoxTexture);
    int w, h, components;
    for (unsigned int i = 0 ; i < m_skyBoxPaths.size() ; ++i){
        //stbi_set_flip_vertically_on_load(true); 
        unsigned char * data = stbi_load(m_skyBoxPaths[i].c_str(), &w, &h, &components, 0);
        //stbi_set_flip_vertically_on_load(false); 
        if (data){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else{
            stbi_image_free(data);
            throw std::runtime_error("Failed to load cubemap texture at " + m_skyBoxPaths[i]);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void FluidRenderer::renderBackground() const{
    m_backgroundPlaneShader.useProgram();
    glUniformMatrix4fv(m_backgroundPlaneUniforms.m_viewTransformation, 1, GL_FALSE, glm::value_ptr(m_camera.viewMatrix));
    m_backgroundPlane.bindVAO();
    m_backgroundPlane.draw(GL_TRIANGLES);
}

void FluidRenderer::renderFluid(GLuint currentLevelSetTexture) const{
    // Coordinates of entry/exit points of camera ray through the cube are rendered as RGB values to texture
    m_raycastingPosShader.useProgram();
    glUniformMatrix4fv(m_raycastingPosUniforms.m_viewTransformation, 1, GL_FALSE, glm::value_ptr(m_camera.viewMatrix));
    // Draw front of cube (cull back faces) in RGB to texture
    glBindFramebuffer(GL_FRAMEBUFFER, m_frontCube.FBO);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    m_cube.bindVAO();
    m_cube.draw(GL_TRIANGLES);

    // Draw back of cube (cull front faces) in RGB to texture
    glBindFramebuffer(GL_FRAMEBUFFER, m_backCube.FBO);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    m_cube.draw(GL_TRIANGLES);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_CULL_FACE); 
    
    // Render fluid by marching using front/back RGB values as entry/exit point coordinates
    m_renderFluidShader.useProgram();
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, m_frontCube.texture.getLocation());
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, m_backCube.texture.getLocation());
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, currentLevelSetTexture);
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_1D, m_splineTexture);
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_1D, m_splineDerivTexture);
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyBoxTexture);
    m_quad.bindVAO();
    m_quad.draw(GL_TRIANGLES);

    // Tidy up texture bindings
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE0 + 0);
}

// Populate a 1D texture with cubic interpolation coefficients/offsets
// RGBA: (g0, g1, h0, h1), where f(x) = g0 * f(i - h0) + g1 * f(i + h1), where i = floor(x)
void FluidRenderer::setUpSplines(){
    // For interpolating values
    auto w0 = [](float a){return (-a * a * a + 3 * a * a - 3 * a + 1) / 6.0f;};
    auto w1 = [](float a){return (3 * a * a * a - 6 * a * a + 4) / 6.0f;};
    auto w2 = [](float a){return (-3 * a * a * a + 3 * a * a + 3 * a + 1) / 6.0f;};
    auto w3 = [](float a){return (a * a * a / 6.0f);}; 

    std::vector<float> splineData(4 * m_splineResolution);
    for (int i = 0 ; i < m_splineResolution ; ++i){
        float alpha = (i + 0.5f) / m_splineResolution;
        splineData[4 * i] = w0(alpha) + w1(alpha);
        splineData[4 * i + 1] = w2(alpha) + w3(alpha); // Technically redundnant as g0 + g1 = 1
        splineData[4 * i + 2] = -(1 + alpha - w1(alpha) / (w0(alpha) + w1(alpha))); // Negative h0 makes fragment shader simpler
        splineData[4 * i + 3] = 1 - alpha + w3(alpha) / (w2(alpha) + w3(alpha));
    }
    glGenTextures(1, &m_splineTexture);
    glBindTexture(GL_TEXTURE_1D, m_splineTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, m_splineResolution, 0, GL_RGBA, GL_FLOAT, splineData.data());
    glBindTexture(GL_TEXTURE_1D, 0);

    // For interpolating derivatives
    auto v0 = [](float a){return (-a * a + 2 * a - 1) / 2.0f;};
    auto v1 = [](float a){return (3 * a * a - 4 * a) / 2.0f;};
    auto v2 = [](float a){return (-3 * a * a + 2 * a + 1) / 2.0f;};
    auto v3 = [](float a){return (a * a / 2.0f);};

    for (int i = 0 ; i < m_splineResolution ; ++i){
        float alpha = (i + 0.5f) / m_splineResolution;
        splineData[4 * i] = v0(alpha) + v1(alpha);
        splineData[4 * i + 1] = v2(alpha) + v3(alpha); // Technically redundnant as g0 + g1 = 0
        splineData[4 * i + 2] = -(1 + alpha - v1(alpha) / (v0(alpha) + v1(alpha))); // Negative h0 makes fragment shader simpler
        splineData[4 * i + 3] = 1 - alpha + v3(alpha) / (v2(alpha) + v3(alpha));
    }
    glGenTextures(1, &m_splineDerivTexture);
    glBindTexture(GL_TEXTURE_1D, m_splineDerivTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, m_splineResolution, 0, GL_RGBA, GL_FLOAT, splineData.data());
    glBindTexture(GL_TEXTURE_1D, 0);
}

void FluidRenderer::Camera::updateMatrix(){
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    viewMatrix = glm::lookAt(position, target, up);
}

FluidRenderer::RenderTarget::RenderTarget(unsigned int width, unsigned int height) : texture{width, height} {
    setUpBuffers();
};

FluidRenderer::RenderTarget::~RenderTarget(){
    releaseBuffers();
}

void FluidRenderer::RenderTarget::setUpBuffers(){
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.getLocation(), 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        throw std::runtime_error("Failed to initialise framebuffer");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FluidRenderer::RenderTarget::releaseBuffers(){
    glDeleteFramebuffers(1, &FBO);
}

Fluid::Fluid(unsigned int w, unsigned int h) : m_simulator{}, m_renderer(w, h),
    m_cameraHorizontalRotationDirection{0}, m_cameraHorizontalRotation{0.0f},
    m_cameraVerticalRotationDirection{0}, m_cameraVerticalRotation{0.0f},
    m_applyingForce{false}
{   
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

bool Fluid::successfullyInitialised() const{
    return m_successfullyInitialised;
}

void Fluid::handleEvents(SDL_Event const& event){
    switch(event.type){
        case SDL_KEYDOWN:
            switch(event.key.keysym.scancode){
                case SDL_SCANCODE_RIGHT:
                case SDL_SCANCODE_D:
                    m_cameraHorizontalRotationDirection = 1;
                    break;
                case SDL_SCANCODE_LEFT:
                case SDL_SCANCODE_A:
                    m_cameraHorizontalRotationDirection = -1;
                    break;
                case SDL_SCANCODE_UP:
                case SDL_SCANCODE_W:
                    m_cameraVerticalRotationDirection = -1;
                    break;
                case SDL_SCANCODE_DOWN:
                case SDL_SCANCODE_S:
                    m_cameraVerticalRotationDirection = 1;
                    break;
                case SDL_SCANCODE_R:
                    m_simulator.resetLevelSet();
                    break;
                default:
                    break;
            }
            break;
        case SDL_KEYUP:
            switch(event.key.keysym.scancode){
                case SDL_SCANCODE_RIGHT:
                case SDL_SCANCODE_D:
                    m_cameraHorizontalRotationDirection = 0;
                    break;
                case SDL_SCANCODE_LEFT:
                case SDL_SCANCODE_A:
                    m_cameraHorizontalRotationDirection = 0;
                    break;
                case SDL_SCANCODE_UP:
                case SDL_SCANCODE_W:
                    m_cameraVerticalRotationDirection = 0;
                    break;
                case SDL_SCANCODE_DOWN:
                case SDL_SCANCODE_S:
                    m_cameraVerticalRotationDirection = 0;
                    break;
                default:
                    break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
                m_applyingForce = true;
                SDL_GetMouseState(&m_forceMouseStartX, &m_forceMouseStartY);
                m_forceMouseEndX = m_forceMouseStartX;
                m_forceMouseEndY = m_forceMouseStartY;
                break;
        case SDL_MOUSEMOTION:
                if (m_applyingForce){SDL_GetMouseState(&m_forceMouseEndX, &m_forceMouseEndY);}
                break;
        case SDL_MOUSEBUTTONUP:
                m_applyingForce = false;
                break;
        default:
            break;
    }
}

void Fluid::frame(unsigned int frameTime){
    updateForce();
    m_simulator.update(frameTime);
    updateCamera(frameTime);
    m_renderer.render(m_simulator.getCurrentLevelSet());
}

void Fluid::updateForce(){
    if(m_applyingForce){
        float squareDistance = std::pow(m_forceMouseEndX - m_forceMouseStartX, 2) + std::pow(m_forceMouseEndY - m_forceMouseStartY, 2);
        float mouseAngle = std::atan2(-(m_forceMouseEndY - m_forceMouseStartY), m_forceMouseEndX - m_forceMouseStartX);
        float forceSize = std::min(squareDistance, 1000.0f) * 1e-11;
        m_simulator.updateAppliedForce(glm::vec3{forceSize * std::cos(mouseAngle) * std::cos(m_cameraHorizontalRotation), 
                                                 forceSize * std::sin(mouseAngle), 
                                                -forceSize * std::cos(mouseAngle) * std::sin(m_cameraHorizontalRotation)});
        m_forceMouseStartX = m_forceMouseEndX;
        m_forceMouseStartY = m_forceMouseEndY;
    }
    else{
        m_simulator.updateAppliedForce(glm::vec3{0.0f, 0.0f, 0.0f});
    }
}

void Fluid::updateCamera(unsigned int frameTime){
    m_cameraHorizontalRotation += frameTime * m_cameraRotationSpeed * m_cameraHorizontalRotationDirection;
    m_cameraVerticalRotation += frameTime * m_cameraRotationSpeed * m_cameraVerticalRotationDirection;
    if (m_cameraVerticalRotation >= m_cameraMaxVerticalRotation){
        m_cameraVerticalRotation = m_cameraMaxVerticalRotation;
    }
    else if (m_cameraVerticalRotation <= m_cameraMinVerticalRotation){
        m_cameraVerticalRotation = m_cameraMinVerticalRotation;
    }
    m_renderer.updateCamera(m_cameraHorizontalRotation, m_cameraVerticalRotation);
}