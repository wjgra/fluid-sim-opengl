#include "../include/fluid.hpp"

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
    m_clearSlabs(".//shaders//slab_operation.vert", ".//shaders//clear_slabs.frag", {})
    
{
    try{
        m_successfullyInitialised = false;
        initialiseShaders();
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
    return m_levelSetCurrent.m_texture;
}

void FluidSimulator::resetLevelSet() const{
    glBindTexture(GL_TEXTURE_3D, m_levelSetCurrent.m_texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, m_initialLevelSetData.data());
    glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.m_texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, gridSize, gridSize, gridSize, 0, GL_RGB, GL_FLOAT, m_initialVelocityData.data());
}

void FluidSimulator::initialiseShaders(){
    m_forceApplication.shader.useProgram();
    /* uniformGravityDirection = m_forceApplication.shader.getUniformLocation("gravityDirection");
    glUniform1f(uniformGravityDirection, gravityDirection); */

    uniformForce = m_forceApplication.shader.getUniformLocation("extForce");
    glUniform3f(uniformForce, 0.0f, 0.0f, 0.0f);
    uniformForcePos = m_forceApplication.shader.getUniformLocation("extForcePos");
    glUniform3f(uniformForce, 0.5f, 0.15f, 0.5f);
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
        /* if(m_resetLevelSet){
            glBindTexture(GL_TEXTURE_3D, m_levelSetCurrent.m_texture);
            glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, m_initialLevelSetData.data());
            glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.m_texture);
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, gridSize, gridSize, gridSize, 0, GL_RGB, GL_FLOAT, m_initialVelocityData.data());
            m_resetLevelSet = false;
        } */

        glDisable(GL_BLEND);
        glViewport(0,0,gridSize, gridSize);
        glEnable(GL_SCISSOR_TEST);
        
        // Apply force to velocity
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.m_texture);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_3D, m_levelSetCurrent.m_texture);

        m_forceApplication.shader.useProgram();

        if(applyingForce){
            float squareDistance = std::pow(forceMouseEndX - forceMouseStartX, 2) + std::pow(forceMouseEndY - forceMouseStartY, 2);
            float mouseAngle = std::atan2(-(forceMouseEndY - forceMouseStartY), forceMouseEndX - forceMouseStartX);
            float forceSize = std::min(squareDistance, 1000.0f) * 1e-11;
            glUniform3f(uniformForce, forceSize * std::cos(mouseAngle) * std::cos(horizRot), forceSize * std::sin(mouseAngle), -forceSize * std::cos(mouseAngle) * std::sin(horizRot));
            forceMouseStartX = forceMouseEndX;
            forceMouseStartY = forceMouseEndY;
        }
        else{
            glUniform3f(uniformForce, 0.0f, 0.0f, 0.0f);
        }

        applyInnerSlabOp(m_forceApplication, m_velocityNext, frameTime);
        std::swap(m_velocityCurrent, m_velocityNext);

        // Velocity BC
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.m_texture);
        applyOuterSlabOp(m_boundaryVelocity, m_velocityNext, frameTime);
        std::swap(m_velocityCurrent, m_velocityNext);

        // Advect Velocity
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.m_texture);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.m_texture);
        applyInnerSlabOp(m_advectionVelocity, m_velocityNext, frameTime);

        // Advect Level Set using old velocity (but with corrected BC)
        glBindTexture(GL_TEXTURE_3D, m_levelSetCurrent.m_texture);
        applyInnerSlabOp(m_advectionLevelSet, m_levelSetNext, frameTime);

        // Put advected velocity in current
        std::swap(m_velocityCurrent, m_velocityNext);

        // Re-bind velocity    
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.m_texture);

        // pass through current velocity to temp velocity, which is used as 0th iteration
        applyInnerSlabOp(m_passThrough, m_tempVectorQuantity, frameTime);
        
        // Re-bind velocity as quantity to be altered in pos = 2
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.m_texture);

        // Diffuse velocity
        glActiveTexture(GL_TEXTURE0 + 0);
        for (int i = 0; i < numJacobiIterationsDiffusion; ++i){
            glBindTexture(GL_TEXTURE_3D, m_tempVectorQuantity.m_texture);
            // Render into next velocity (kth iterate is in temp, k+1th in next)
            applyInnerSlabOp(m_diffusion, m_velocityNext, frameTime);
            // swap next and temp velocity, then iterate 
            std::swap(m_velocityNext, m_tempVectorQuantity);
            // Velocity BC - do we need to apply this every iteration?
            glBindTexture(GL_TEXTURE_3D, m_tempVectorQuantity.m_texture);
            applyOuterSlabOp(m_boundaryVelocity, m_velocityNext, frameTime);
            std::swap(m_velocityNext, m_tempVectorQuantity);
        }
            
        std::swap(m_velocityCurrent, m_tempVectorQuantity); // Swap final iteration into current velocity

        // *Remove divergence from velocity*

        // Apply velocity BC (must be done to ensure correct divergence at bdries)
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.m_texture);
        applyOuterSlabOp(m_boundaryVelocity, m_velocityNext, frameTime);
        std::swap(m_velocityCurrent, m_velocityNext);

        // Compute div of currentVelocity (store in textureVelocityTemp)
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.m_texture);
        applyInnerSlabOp(m_divergence, m_tempScalarQuantity, frameTime);

        // Clear pressure texture
        /*applyOuterSlabOperation(m_clearSlabs, pressure.textureNext);
        std::swap(pressure.textureCurrent, pressure.textureNext);*/


        // Bindings
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_3D, m_levelSetCurrent.m_texture);

        // Solve Poisson eqn 
        for (int i = 0; i < numJacobiIterationsPressure; ++i){
            // Pressure BC
            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_3D, m_pressureCurrent.m_texture);
            applyOuterSlabOp(m_boundaryPressure, m_pressureNext, frameTime);
            std::swap(m_pressureCurrent, m_pressureNext);

            // Iteration (kth iteration in current, k+1th in next)
            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_3D, m_pressureCurrent.m_texture); // pressure
            glActiveTexture(GL_TEXTURE0 + 2);
            glBindTexture(GL_TEXTURE_3D, m_tempScalarQuantity.m_texture); // div(velocity)
            applyInnerSlabOp(m_pressurePoisson, m_pressureNext, frameTime);
            std::swap(m_pressureCurrent, m_pressureNext);
        }

        // Subtract grad(pressure) from currentVelocity
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, m_velocityCurrent.m_texture);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_3D, m_pressureCurrent.m_texture);
        applyInnerSlabOp(m_removeDivergence, m_velocityNext, frameTime);
        std::swap(m_velocityCurrent, m_velocityNext);

        //Level set BC
        std::swap(m_levelSetCurrent, m_levelSetNext);
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, m_levelSetCurrent.m_texture);
        applyOuterSlabOp(m_boundaryLevelSet, m_levelSetNext, frameTime);
        std::swap(m_levelSetCurrent, m_levelSetNext);


        // Tidy up
        glDisable(GL_SCISSOR_TEST);
        // glViewport(0,0,m_screenWidth,m_screenHeight); // moved to renderer frame()
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glActiveTexture(GL_TEXTURE0 + 0);
        glEnable(GL_BLEND);
    }

// Generates a new 3D floating-point texture with the given input as the initial data
void FluidSimulator::SimulatedQuantity::generateTexture(std::vector<float> data, bool scalarQuantity = false){
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_3D, m_texture);
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
        glGenFramebuffers(1, &(m_slabFBOs[zSlice]));
        glBindFramebuffer(GL_FRAMEBUFFER, m_slabFBOs[zSlice]);
        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, m_texture, 0, zSlice);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::string("Failed to initialise framebuffer\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FluidSimulator::SlabOperation::SlabOperation(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames) :
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

FluidSimulator::InnerSlabOp::InnerSlabOp(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames) : 
    SlabOperation(vertexShaderPath, fragmentShaderPath, textureNames)
{
    // Z-Slice and timestep
    uniformZSlice = shader.getUniformLocation("zSlice");
    uniformTimeStep = shader.getUniformLocation("timeStep");

};

FluidSimulator::OuterSlabOp::OuterSlabOp(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames) : 
    SlabOperation(vertexShaderPath, fragmentShaderPath, textureNames)
{
    uniformZSlice = shader.getUniformLocation("zSlice");

};


// ***Issue: ideally these would be a member of slab op, but then they don't have access to quad!
void FluidSimulator::applySlabOp(SlabOperation& slabOp, SimulatedQuantity& quantity, unsigned int frameTime, int layerFrom, int layerTo){
    quad.bindVAO();
    slabOp.shader.useProgram();
    glUniform1f(slabOp.uniformTimeStep, (float)frameTime);
    for (int zSlice = layerFrom; zSlice < layerTo; ++zSlice){
        glBindFramebuffer(GL_FRAMEBUFFER, quantity.m_slabFBOs[zSlice]);
        glUniform1f(slabOp.uniformZSlice, (float)zSlice);
        quad.draw(GL_TRIANGLES);
    }
}

void FluidSimulator::applyInnerSlabOp(InnerSlabOp& slabOp, SimulatedQuantity& quantity, unsigned int frameTime){
    glScissor(1,1,gridSize-2,gridSize-2);
    applySlabOp(slabOp, quantity, frameTime, 1, gridSize-1);
}

void FluidSimulator::applyOuterSlabOp(OuterSlabOp& slabOp, SimulatedQuantity& quantity, unsigned int frameTime){
    // Issue: may be more efficient to render four quads
    glScissor(0,0,gridSize,gridSize);
    applySlabOp(slabOp, quantity, frameTime, 0, gridSize);
}

FluidRenderer::FluidRenderer(unsigned int width, unsigned int height) : m_screenWidth{width}, m_screenHeight{height}, horizRot{0.0f}, vertRot{0.0f},
    camera(m_cameraIntialPos),
    frontCube{width, height},
    backCube{width, height},
    backgroundPlaneShader{".//shaders//background_plane.vert", ".//shaders//background_plane.frag"},
    raycastingPosShader(".//shaders//raycasting_pos.vert", ".//shaders//raycasting_pos.frag"),
    renderFluidShader(".//shaders//fluid.vert", ".//shaders//fluid.frag")
{
    initialiseShaders();
    setUpSkybox();
    m_successfullyInitialised = true;
}

void FluidRenderer::updateCamera(float cameraHorizontalRotation, float cameraVerticalRotation){
    if (horizRot != cameraHorizontalRotation || vertRot != cameraVerticalRotation){
        horizRot = cameraHorizontalRotation;
        vertRot = cameraVerticalRotation;        
        camera.pos = glm::vec3(glm::rotate(glm::mat4(1.0f), horizRot, glm::vec3(0.0f, 1.0f, 0.0f)) * 
                     glm::rotate(glm::mat4(1.0f), vertRot, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::vec4(0.0f, 1.5f, 3.0f, 1.0f));
        camera.updateMatrix();
    }
}

void FluidRenderer::render(unsigned int frameTime, GLuint currentLevelSetTexture){
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
    raycastingPosShader.useProgram();
    
    // Model matrix
    raycastingPosUniforms.m_modelTransformation = raycastingPosShader.getUniformLocation("model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(m_cubeScale, m_cubeScale, m_cubeScale));
    glUniformMatrix4fv(raycastingPosUniforms.m_modelTransformation, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    raycastingPosUniforms.m_projectionTransformation = raycastingPosShader.getUniformLocation("projection");
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)m_screenWidth/(float)m_screenHeight, 0.1f, 100.0f);
    glUniformMatrix4fv(raycastingPosUniforms.m_projectionTransformation, 1, GL_FALSE, glm::value_ptr(projection));

    // View matrix
    raycastingPosUniforms.m_viewTransformation = raycastingPosShader.getUniformLocation("view");
    camera.updateMatrix();
    //glm::mat4 view = camera.viewMatrix;//::lookAt(glm::vec3(0.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 2.0f, 2.0f));
    glUniformMatrix4fv(raycastingPosUniforms.m_viewTransformation, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));

    
    // Get uniform locations and set values for backgroundPlaneShader
    backgroundPlaneShader.useProgram();
    
    // Model matrix
    backgroundPlaneUniforms.m_modelTransformation = backgroundPlaneShader.getUniformLocation("model");
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(planeSize, planeSize, planeSize));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, (-0.5f + 1.0f / gridSize) * m_cubeScale / planeSize)); // Fluid is not in outer cells - translate to sit on plane surface
    glUniformMatrix4fv(backgroundPlaneUniforms.m_modelTransformation, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    backgroundPlaneUniforms.m_projectionTransformation = backgroundPlaneShader.getUniformLocation("projection");
    glUniformMatrix4fv(backgroundPlaneUniforms.m_projectionTransformation, 1, GL_FALSE, glm::value_ptr(projection));

    // View matrix
    backgroundPlaneUniforms.m_viewTransformation = backgroundPlaneShader.getUniformLocation("view");
    glUniformMatrix4fv(backgroundPlaneUniforms.m_viewTransformation, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));

    // Get uniform locations and set values for renderFluidShader - note symmetry with above (can we condense?)
    renderFluidShader.useProgram();

    // Model matrix
    renderFluidUniforms.m_modelTransformation = renderFluidShader.getUniformLocation("model");
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(m_screenWidth, m_screenHeight, 1)); 
    model = glm::translate(model, glm::vec3(0, 0, 0.0f));
    glUniformMatrix4fv(renderFluidUniforms.m_modelTransformation, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    projection = glm::ortho(0.0f, (float)m_screenWidth,  0.0f, (float)m_screenHeight, -1.0f, 1.0f);
    renderFluidUniforms.m_projectionTransformation = renderFluidShader.getUniformLocation("projection");
    glUniformMatrix4fv(renderFluidUniforms.m_projectionTransformation, 1, GL_FALSE, glm::value_ptr(projection));

    // No view matrix required in orthogonal projection

    // Set up uniforms for 'cube vector' textures
    frontCube.uniformTexture =  renderFluidShader.getUniformLocation("frontTexture");
    backCube.uniformTexture = renderFluidShader.getUniformLocation("backTexture");
    renderFluidShader.useProgram();
    glUniform1i(frontCube.uniformTexture, 0);
    glUniform1i(backCube.uniformTexture, 1);

    // Set up uniform for level set texture
    uniformLevelSetFluid = renderFluidShader.getUniformLocation("levelSetTexture");
    glUniform1i(uniformLevelSetFluid, 2);

    setUpSplines(); // For use in tri-cubic interpolation of normals
    uniformSplineTexture = renderFluidShader.getUniformLocation("splineTexture");
    glUniform1i(uniformSplineTexture, 3);
    uniformSplineDerivTexture = renderFluidShader.getUniformLocation("splineDerivTexture");
    glUniform1i(uniformSplineDerivTexture, 4);

    setUpSkybox();
    uniformSkyBoxTexture = renderFluidShader.getUniformLocation("skyBoxTexture");
    glUniform1i(uniformSkyBoxTexture, 5);
}

void FluidRenderer::renderBackground(){
    backgroundPlaneShader.useProgram();
    glUniformMatrix4fv(backgroundPlaneUniforms.m_viewTransformation, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));
    backgroundPlane.bindVAO();
    backgroundPlane.draw(GL_TRIANGLES);
}

void FluidRenderer::renderFluid(GLuint currentLevelSetTexture){
        // Coordinates of entry/exit points of camera ray through the cube are rendered as RGB values to texture
        raycastingPosShader.useProgram();
        glUniformMatrix4fv(raycastingPosUniforms.m_viewTransformation, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));
        // Draw front of cube (cull back faces) in RGB to texture
        glBindFramebuffer(GL_FRAMEBUFFER, frontCube.FBO);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        cube.bindVAO();
        cube.draw(GL_TRIANGLES);

        // Draw back of cube (cull front faces) in RGB to texture
        glBindFramebuffer(GL_FRAMEBUFFER, backCube.FBO);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        cube.draw(GL_TRIANGLES);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_CULL_FACE); 
        
        // Render fluid by marching using front/back RGB values as entry/exit point coordinates
        renderFluidShader.useProgram();
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, frontCube.texture.getLocation());
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, backCube.texture.getLocation());
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_3D, currentLevelSetTexture);
        glActiveTexture(GL_TEXTURE0 + 3);
        glBindTexture(GL_TEXTURE_1D, splineTexture);
        glActiveTexture(GL_TEXTURE0 + 4);
        glBindTexture(GL_TEXTURE_1D, splineDerivTexture);
        glActiveTexture(GL_TEXTURE0 + 5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxTexture);
        quad.bindVAO();
        quad.draw(GL_TRIANGLES);

        // Tidy up texture bindings
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_3D, 0);
        glActiveTexture(GL_TEXTURE0 + 0);
    }

// Using scissor and six slab op commands to draw bdry was actually slower!
void FluidRenderer::setUpSkybox(){
    glGenTextures(1, &skyBoxTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyBoxTexture);
    int w, h, components;
    for (unsigned int i = 0 ; i < skyBoxPaths.size() ; ++i){
        //stbi_set_flip_vertically_on_load(true); 
        unsigned char * data = stbi_load(skyBoxPaths[i].c_str(), &w, &h, &components, 0);
        //stbi_set_flip_vertically_on_load(false); 
        if (data){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else{
            stbi_image_free(data);
            throw std::string("Failed to load cubemap texture at " + skyBoxPaths[i]);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}


// Populate a 1D texture with cubic interpolation coefficients/offsets
// RGBA: (g0, g1, h0, h1), where f(x) = g0 * f(i - h0) + g1 * f(i + h1), where i = floor(x)
void FluidRenderer::setUpSplines(){
    // For interpolating values
    auto w0 = [](float a){return (-a * a * a + 3 * a * a - 3 * a + 1) / 6.0f;};
    auto w1 = [](float a){return (3 * a * a * a - 6 * a * a + 4) / 6.0f;};
    auto w2 = [](float a){return (-3 * a * a * a + 3 * a * a + 3 * a + 1) / 6.0f;};
    auto w3 = [](float a){return (a * a * a / 6.0f);}; 

    std::vector<float> splineData(4 * splineRes);
    for (int i = 0 ; i < splineRes ; ++i){
        float alpha = (i + 0.5f) / splineRes;
        splineData[4 * i] = w0(alpha) + w1(alpha);
        splineData[4 * i + 1] = w2(alpha) + w3(alpha); // Technically redundnant as g0 + g1 = 1
        splineData[4 * i + 2] = -(1 + alpha - w1(alpha) / (w0(alpha) + w1(alpha))); // Negative h0 makes fragment shader simpler
        splineData[4 * i + 3] = 1 - alpha + w3(alpha) / (w2(alpha) + w3(alpha));
    }
    glGenTextures(1, &splineTexture);
    glBindTexture(GL_TEXTURE_1D, splineTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, splineRes, 0, GL_RGBA, GL_FLOAT, splineData.data());
    glBindTexture(GL_TEXTURE_1D, 0);

    // For interpolating derivatives
    auto v0 = [](float a){return (-a * a + 2 * a - 1) / 2.0f;};
    auto v1 = [](float a){return (3 * a * a - 4 * a) / 2.0f;};
    auto v2 = [](float a){return (-3 * a * a + 2 * a + 1) / 2.0f;};
    auto v3 = [](float a){return (a * a / 2.0f);};

    for (int i = 0 ; i < splineRes ; ++i){
        float alpha = (i + 0.5f) / splineRes;
        splineData[4 * i] = v0(alpha) + v1(alpha);
        splineData[4 * i + 1] = v2(alpha) + v3(alpha); // Technically redundnant as g0 + g1 = 0
        splineData[4 * i + 2] = -(1 + alpha - v1(alpha) / (v0(alpha) + v1(alpha))); // Negative h0 makes fragment shader simpler
        splineData[4 * i + 3] = 1 - alpha + v3(alpha) / (v2(alpha) + v3(alpha));
    }
    glGenTextures(1, &splineDerivTexture);
    glBindTexture(GL_TEXTURE_1D, splineDerivTexture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, splineRes, 0, GL_RGBA, GL_FLOAT, splineData.data());
    glBindTexture(GL_TEXTURE_1D, 0);
}

void FluidRenderer::Camera::updateMatrix(){
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    viewMatrix = glm::lookAt(pos, target, up);
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

    // Check complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::string("Failed to initialise framebuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FluidRenderer::RenderTarget::releaseBuffers(){
    glDeleteFramebuffers(1, &FBO);
}

Fluid::Fluid(unsigned int w, unsigned int h) : m_simulator{}, m_renderer(w, h), m_cameraHorizontalRotationDirection{0}, m_cameraHorizontalRotation{0.0f},
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
    m_simulator.update(frameTime);
    updateCamera(frameTime);
    m_renderer.render(frameTime, m_simulator.getCurrentLevelSet());
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