#include "../include/fluid_renderer.hpp"

Drawable::Drawable(std::vector<float> const& verts, unsigned int vertexDimension) : vertices{verts}{
    setUpBuffers(vertexDimension);
}

Drawable::~Drawable(){
    releaseBuffers();
}

// consider ditching these, and having a getVAO fn instead
void Drawable::bindVAO(){
    glBindVertexArray(VAO);
}

/* void Drawable::unbindVAO(){
    glBindVertexArray(0);
} */

void Drawable::draw(GLint drawingMode){
    glDrawArrays(drawingMode, 0, vertices.size());
}

void Drawable::setUpBuffers(unsigned int vertDim){
    if (vertDim < 2 || vertDim > 4){
        throw std::string("Failed to set up buffer object. Vertex dimension must be 2, 3 or 4.\n");
    }
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); 

    // Bind VBO and copy vertex data into VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    unsigned int stride = vertDim + 2; // Length of vert and UV data    
    // Verts
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, vertDim, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)0);
    // UVs
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(vertDim*sizeof(float)));

    glBindVertexArray(0);
}

void Drawable::releaseBuffers(){
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

// Generates a new 3D floating-point texture with the given input as the initial data
void FluidSimulator::SQ::generateTexture(std::vector<float> data, bool scalarQuantity = false){
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
void FluidSimulator::SQ::generateFBOs(){
    for (int zSlice = 0; zSlice < gridSize; ++zSlice){
        glGenFramebuffers(1, &(slabFBOs[zSlice]));
        glBindFramebuffer(GL_FRAMEBUFFER, slabFBOs[zSlice]);
        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, texture, 0, zSlice);
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
    quadUniforms.modelTrans = shader.getUniformLocation("model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(gridSize, gridSize, 1));
    model = glm::translate(model, glm::vec3(0.0f,0.0f,0.0f));
    glUniformMatrix4fv(quadUniforms.modelTrans, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    glm::mat4 projection = glm::ortho(0.0f, (float)gridSize,  0.0f, (float)gridSize, -1.0f, 1.0f);
    quadUniforms.projTrans = shader.getUniformLocation("projection");
    glUniformMatrix4fv(quadUniforms.projTrans, 1, GL_FALSE, glm::value_ptr(projection));
    
    for (unsigned int i = 0 ; i < textureNames.size() ; ++i){
        if (textureNames[i].length() != 0){
            glUniform1i(shader.getUniformLocation(textureNames[i]), i);
        }
    }
}

FluidSimulator::innerSlabOp::innerSlabOp(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames) : 
    SlabOperation(vertexShaderPath, fragmentShaderPath, textureNames)
{
    // Z-Slice and timestep
    uniformZSlice = shader.getUniformLocation("zSlice");
    uniformTimeStep = shader.getUniformLocation("timeStep");

};

FluidSimulator::outerSlabOp::outerSlabOp(const std::string vertexShaderPath, const std::string fragmentShaderPath, std::vector<std::string> textureNames) : 
    SlabOperation(vertexShaderPath, fragmentShaderPath, textureNames)
{
    uniformZSlice = shader.getUniformLocation("zSlice");

};


// ***Issue: ideally these would be a member of slab op, but then they don't have access to quad!
void FluidSimulator::applySlabOp(SlabOperation& slabOp, SQ& quantity, unsigned int frameTime, int layerFrom, int layerTo){
    quad.bindVAO();
    slabOp.shader.useProgram();
    glUniform1f(slabOp.uniformTimeStep, (float)frameTime);
    for (int zSlice = layerFrom; zSlice < layerTo; ++zSlice){
        glBindFramebuffer(GL_FRAMEBUFFER, quantity.slabFBOs[zSlice]);
        glUniform1f(slabOp.uniformZSlice, (float)zSlice);
        quad.draw(GL_TRIANGLES);
    }
}

void FluidSimulator::applyInnerSlabOp(innerSlabOp& slabOp, SQ& quantity, unsigned int frameTime){
    glScissor(1,1,gridSize-2,gridSize-2);
    applySlabOp(slabOp, quantity, frameTime, 1, gridSize-1);
}

void FluidSimulator::applyOuterSlabOp(outerSlabOp& slabOp, SQ& quantity, unsigned int frameTime){
    glScissor(0,0,gridSize,gridSize);
    //glScissor(0,0,1,gridSize);
    applySlabOp(slabOp, quantity, frameTime, 0, gridSize);
}

FluidSimulator::FluidSimulator() : 
    advection(".//shaders//slab_operation.vert", ".//shaders//advect_quantity.frag", {"velocityTexture", "quantityTexture"}),
    advectionVel(".//shaders//slab_operation.vert", ".//shaders//advect_velocity.frag", {"velocityTexture", "quantityTexture"}),
    diffusion(".//shaders//slab_operation.vert", ".//shaders//diffuse_quantity.frag", {"quantityTexture"}),
    forceApplication(".//shaders//slab_operation.vert", ".//shaders//apply_force_to_velocity.frag", {"velocityTexture", "levelSetTexture"}),
    passThrough(".//shaders//slab_operation.vert", ".//shaders//pass_through.frag", {"quantityTexture"}),

    pressurePoisson(".//shaders//slab_operation.vert", ".//shaders//pressure_poisson.frag", {"pressureTexture", "levelSetTexture", "divergenceTexture"}),
    divergence(".//shaders//slab_operation.vert", ".//shaders//divergence.frag", {"velocityTexture"}),
    removeDivergence(".//shaders//slab_operation.vert", ".//shaders//remove_divergence.frag", {"velocityTexture", "pressureTexture"}),
    
    boundaryVelocity(".//shaders//slab_operation.vert", ".//shaders//boundary_velocity.frag", {"velocityTexture"}),
    boundaryLS(".//shaders//slab_operation.vert", ".//shaders//boundary_levelset.frag", {"levelSetTexture"}),
    boundaryPressure(".//shaders//slab_operation.vert", ".//shaders//boundary_pressure.frag", {"pressureTexture"}),
    clearSlabs(".//shaders//slab_operation.vert", ".//shaders//clear_slabs.frag", {})
    
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

void FluidSimulator::frame(unsigned int frameTime){
    integrateFluid(frameTime);
}

bool FluidSimulator::successfullyInitialised() const {
    return m_successfullyInitialised;
}

GLuint FluidSimulator::getCurrentLevelSet() const{
    return levelSetCurrent.texture;
}

void FluidSimulator::initialiseShaders(){
    forceApplication.shader.useProgram();
    uniformGravityDirection = forceApplication.shader.getUniformLocation("gravityDirection");
    glUniform1f(uniformGravityDirection, gravityDirection);

    uniformForce = forceApplication.shader.getUniformLocation("extForce");
    glUniform3f(uniformForce, 0.0f, 0.0f, 0.0f);
    uniformForcePos = forceApplication.shader.getUniformLocation("extForcePos");
    glUniform3f(uniformForce, 0.5f, 0.15f, 0.5f);
}

void FluidSimulator::initialiseTextures(){
        glActiveTexture(GL_TEXTURE0 + 0);
        // Level set - initial surface at z = 0.5f
        // Takes the value of zero on air-water and box-water interfaces
        // ***Issue: Should be signed distance field, but using 0.5f outside due to pressure issue
        levelSetData = std::vector<float>(gridSize*gridSize*gridSize, 0.0f);
        
        for (int k = 0; k < gridSize; ++k){
        for (int j = 0 ; j < gridSize; ++j){
                for (int i = 0; i < gridSize; ++i){
                    //location of (i,j,k) in texture data
                    int index = gridSize * gridSize * k + gridSize * j + i;
                    levelSetData[index] = j - gridSize/2;
                }
            }
        }
        
        levelSetCurrent.generateTexture(levelSetData, true);
        levelSetNext.generateTexture(levelSetData, true);

        // Velocity - initially zero everywhere
        velocityData = std::vector<float>(4*gridSize*gridSize*gridSize, 0.0f);

        velocityCurrent.generateTexture(velocityData, false);
        velocityNext.generateTexture(velocityData, false);
        
        // Pressure - initially zero 
        std::vector<float> tempPressureData(gridSize*gridSize*gridSize, 0.0f);

        pressureCurrent.generateTexture(tempPressureData, true); // convert to scalar once tempVectorQuantity separated for vel/pressure
        pressureNext.generateTexture(tempPressureData, true);

        // Temporary set of buffers for use in Jacobi iteration
        tempVectorQuantity.generateTexture(velocityData, false);
        tempScalarQuantity.generateTexture(tempPressureData, true);
    }

void FluidSimulator::initialiseFramebufferObjects(){
    levelSetCurrent.generateFBOs();
    levelSetNext.generateFBOs();
    velocityCurrent.generateFBOs();
    velocityNext.generateFBOs();
    pressureCurrent.generateFBOs();
    pressureNext.generateFBOs();
    tempVectorQuantity.generateFBOs();
    tempScalarQuantity.generateFBOs();
}

void FluidSimulator::integrateFluid(unsigned int frameTime){
        if(resetLevelSet){
            glBindTexture(GL_TEXTURE_3D, levelSetCurrent.texture);
            glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, levelSetData.data());
            glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);
            glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, gridSize, gridSize, gridSize, 0, GL_RGB, GL_FLOAT, velocityData.data());
            resetLevelSet = false;
        }
        // Update gravity vector
        /* if (resetGravity){
            gravityDirection = 0.0f;
            resetGravity = false;
        }
        else if (gravityRotatingPos){
            gravityDirection += frameTime * gravityRotSpeed;
        }
        else if (gravityRotatingNeg){
            gravityDirection -= frameTime * gravityRotSpeed;
        } */
    
        glDisable(GL_BLEND);
        glViewport(0,0,gridSize, gridSize);
        glEnable(GL_SCISSOR_TEST);
        
        // Apply force to velocity
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_3D, levelSetCurrent.texture);

        /* advection.shader.useProgram();
        glUniform1f(uniformGravityDirAdv, gravityDirection);*/

        forceApplication.shader.useProgram();
        //glUniform3f(uniformGravityDirection, std::sin(gravityDirection), -1.0f * std::cos(gravityDirection), 0.0f);

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

        applyInnerSlabOp(forceApplication, velocityNext, frameTime);
        std::swap(velocityCurrent, velocityNext);

        // Velocity BC
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);
        applyOuterSlabOp(boundaryVelocity, velocityNext, frameTime);
        std::swap(velocityCurrent, velocityNext);

        // Advect Velocity
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);
        applyInnerSlabOp(advectionVel, velocityNext, frameTime);

        // Advect Level Set using old velocity (but with corrected BC)
        glBindTexture(GL_TEXTURE_3D, levelSetCurrent.texture);
        applyInnerSlabOp(advection, levelSetNext, frameTime);

        // Put advected velocity in current
        std::swap(velocityCurrent, velocityNext);

        // Re-bind velocity    
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);

        // pass through current velocity to temp velocity, which is used as 0th iteration
        applyInnerSlabOp(passThrough, tempVectorQuantity, frameTime);
        
        // Re-bind velocity as quantity to be altered in pos = 2
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);

        // Diffuse velocity
        glActiveTexture(GL_TEXTURE0 + 0);
        for (int i = 0; i < numJacobiIterations; ++i){
            glBindTexture(GL_TEXTURE_3D, tempVectorQuantity.texture);
            // Render into next velocity (kth iterate is in temp, k+1th in next)
            applyInnerSlabOp(diffusion, velocityNext, frameTime);
            // swap next and temp velocity, then iterate 
            std::swap(velocityNext, tempVectorQuantity);
            // Velocity BC - do we need to apply this every iteration?
            glBindTexture(GL_TEXTURE_3D, tempVectorQuantity.texture);
            applyOuterSlabOp(boundaryVelocity, velocityNext, frameTime);
            std::swap(velocityNext, tempVectorQuantity);
        }
            
        std::swap(velocityCurrent, tempVectorQuantity); // Swap final iteration into current velocity

        // *Remove divergence from velocity*

        // Apply velocity BC (must be done to ensure correct divergence at bdries)
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);
        applyOuterSlabOp(boundaryVelocity, velocityNext, frameTime);
        std::swap(velocityCurrent, velocityNext);

        // Compute div of currentVelocity (store in textureVelocityTemp)
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);
        applyInnerSlabOp(divergence, tempScalarQuantity, frameTime);

        // Clear pressure texture
        /*applyOuterSlabOperation(clearSlabs, pressure.textureNext);
        std::swap(pressure.textureCurrent, pressure.textureNext);*/


        // Bindings
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_3D, levelSetCurrent.texture);

        // Solve Poisson eqn 
        for (int i = 0; i < numJacobiIterationsPressure; ++i){
            // Pressure BC
            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_3D, pressureCurrent.texture);
            applyOuterSlabOp(boundaryPressure, pressureNext, frameTime);
            std::swap(pressureCurrent, pressureNext);

            // Iteration (kth iteration in current, k+1th in next)
            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_3D, pressureCurrent.texture); // pressure
            glActiveTexture(GL_TEXTURE0 + 2);
            glBindTexture(GL_TEXTURE_3D, tempScalarQuantity.texture); // div(velocity)
            applyInnerSlabOp(pressurePoisson, pressureNext, frameTime);
            std::swap(pressureCurrent, pressureNext);
        }

        // Subtract grad(pressure) from currentVelocity
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_3D, pressureCurrent.texture);
        applyInnerSlabOp(removeDivergence, velocityNext, frameTime);
        std::swap(velocityCurrent, velocityNext);

        //Level set BC
        std::swap(levelSetCurrent, levelSetNext);
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, levelSetCurrent.texture);
        applyOuterSlabOp(boundaryLS, levelSetNext, frameTime);
        std::swap(levelSetCurrent, levelSetNext);


        // Tidy up
        glDisable(GL_SCISSOR_TEST);
        // glViewport(0,0,screenWidth,screenHeight); // moved to renderer frame()
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glActiveTexture(GL_TEXTURE0 + 0);
        glEnable(GL_BLEND);
    }

NewFluidRenderer::NewFluidRenderer(unsigned int width, unsigned int height) : screenWidth{width}, screenHeight{height},
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

void NewFluidRenderer::frame(unsigned int frameTime, GLuint currentLevelSetTexture){
    if (cameraRotating){
        horizRot += frameTime * horizRotSpeed;
        camera.pos = glm::vec3(glm::rotate(glm::mat4(1.0f), horizRot, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0.0f, 1.5f, 3.0f, 1.0f));
        camera.updateMatrix();
    }
    glDisable(GL_CULL_FACE);

    glViewport(0,0,screenWidth,screenHeight);
    // Render level set using splines
    renderBackground();
    renderFluid(currentLevelSetTexture);
}

bool NewFluidRenderer::successfullyInitialised() const {
    return m_successfullyInitialised;
}

void NewFluidRenderer::initialiseShaders(){
    // Get uniform locations and set values for raycastingPosShader
    raycastingPosShader.useProgram();
    
    // Model matrix
    raycastingPosUniforms.modelTrans = raycastingPosShader.getUniformLocation("model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(scale, scale, scale));
    glUniformMatrix4fv(raycastingPosUniforms.modelTrans, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    raycastingPosUniforms.projTrans = raycastingPosShader.getUniformLocation("projection");
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth/(float)screenHeight, 0.1f, 100.0f);
    glUniformMatrix4fv(raycastingPosUniforms.projTrans, 1, GL_FALSE, glm::value_ptr(projection));

    // View matrix
    raycastingPosUniforms.viewTrans = raycastingPosShader.getUniformLocation("view");
    camera.updateMatrix();
    glm::mat4 view = camera.viewMatrix;//::lookAt(glm::vec3(0.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 2.0f, 2.0f));
    glUniformMatrix4fv(raycastingPosUniforms.viewTrans, 1, GL_FALSE, glm::value_ptr(view));

    // Get uniform locations and set values for backgroundPlaneShader
    backgroundPlaneShader.useProgram();
    
    // Model matrix
    backgroundPlaneUniforms.modelTrans = backgroundPlaneShader.getUniformLocation("model");
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(planeSize, planeSize, planeSize));
    model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, (-0.5f + 1.0f / gridSize) * scale / planeSize)); // Fluid is not in outer cells - translate to sit on plane surface
    glUniformMatrix4fv(backgroundPlaneUniforms.modelTrans, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    backgroundPlaneUniforms.projTrans = backgroundPlaneShader.getUniformLocation("projection");
    glUniformMatrix4fv(backgroundPlaneUniforms.projTrans, 1, GL_FALSE, glm::value_ptr(projection));

    // View matrix
    backgroundPlaneUniforms.viewTrans = backgroundPlaneShader.getUniformLocation("view");
    glUniformMatrix4fv(backgroundPlaneUniforms.viewTrans, 1, GL_FALSE, glm::value_ptr(view));

    // Get uniform locations and set values for renderFluidShader - note symmetry with above (can we condense?)
    renderFluidShader.useProgram();

    // Model matrix
    renderFluidUniforms.modelTrans = renderFluidShader.getUniformLocation("model");
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(screenWidth, screenHeight, 1)); 
    model = glm::translate(model, glm::vec3(0, 0, 0.0f));
    glUniformMatrix4fv(renderFluidUniforms.modelTrans, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    projection = glm::ortho(0.0f, (float)screenWidth,  0.0f, (float)screenHeight, -1.0f, 1.0f);
    renderFluidUniforms.projTrans = renderFluidShader.getUniformLocation("projection");
    glUniformMatrix4fv(renderFluidUniforms.projTrans, 1, GL_FALSE, glm::value_ptr(projection));

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

void NewFluidRenderer::renderBackground(){
    backgroundPlaneShader.useProgram();
    glUniformMatrix4fv(backgroundPlaneUniforms.viewTrans, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));
    backgroundPlane.bindVAO();
    backgroundPlane.draw(GL_TRIANGLES);
}

void NewFluidRenderer::renderFluid(GLuint currentLevelSetTexture){
        // Coordinates of entry/exit points of camera ray through the cube are rendered as RGB values to texture
        raycastingPosShader.useProgram();
        glUniformMatrix4fv(raycastingPosUniforms.viewTrans, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));
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
void NewFluidRenderer::setUpSkybox(){
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
void NewFluidRenderer::setUpSplines(){
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

void NewFluidRenderer::Camera::updateMatrix(){
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    viewMatrix = glm::lookAt(pos, target, up);
}

NewFluidRenderer::RenderTarget::RenderTarget(unsigned int width, unsigned int height) : texture{width, height} {
    setUpBuffers();
};

NewFluidRenderer::RenderTarget::~RenderTarget(){
    releaseBuffers();
}

void NewFluidRenderer::RenderTarget::setUpBuffers(){
    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.getLocation(), 0);

    // Check complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::string("Failed to initialise framebuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void NewFluidRenderer::RenderTarget::releaseBuffers(){
    glDeleteFramebuffers(1, &FBO);
}

Fluid::Fluid(unsigned int w, unsigned int h) : m_simulator{}, m_renderer(w, h)
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
    // need to add links between modules to enable controls
/*         switch(event.type){
        case SDL_KEYDOWN:
            switch(event.key.keysym.scancode){
                case SDL_SCANCODE_R:
                    resetLevelSet = true;
                    break;
                default:
                break;
            }
            break;
        case SDL_KEYUP:
            switch(event.key.keysym.scancode){
                break;
                case SDL_SCANCODE_SPACE:
                    cameraRotating = !cameraRotating;
                break;
                default:
                break;
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
                applyingForce = true;
                SDL_GetMouseState(&forceMouseStartX, &forceMouseStartY);
                forceMouseEndX = forceMouseStartX;
                forceMouseEndY = forceMouseStartY;
            break;
        case SDL_MOUSEMOTION:
                if (applyingForce){SDL_GetMouseState(&forceMouseEndX, &forceMouseEndY);}
            break;
        case SDL_MOUSEBUTTONUP:
                applyingForce = false;
            break;
        default:
        break;
    } */
}

void Fluid::frame(unsigned int frameTime){
    m_simulator.frame(frameTime);
    m_renderer.frame(frameTime, m_simulator.getCurrentLevelSet());
}