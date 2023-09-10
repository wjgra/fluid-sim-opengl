#include "../include/fluid_renderer.hpp"

FluidRenderer::FluidRenderer(unsigned int width, unsigned int height) :
    screenWidth{width},
    screenHeight{height},
    frontCube{width, height},
    backCube{width, height},
    backgroundPlaneShader{".//shaders//background_plane.vert", ".//shaders//background_plane.frag"},
    raycastingPosShader(".//shaders//raycasting_pos.vert", ".//shaders//raycasting_pos.frag"),
    renderFluidShader(".//shaders//fluid.vert", ".//shaders//fluid.frag"),
    // Slab operations
    advection(".//shaders//slab_operation.vert", ".//shaders//advect_velocity.frag"),
    diffusion(".//shaders//slab_operation.vert", ".//shaders//diffuse_quantity.frag"),
    forceApplication(".//shaders//slab_operation.vert", ".//shaders//apply_force_to_quantity.frag"),
    passThrough(".//shaders//slab_operation.vert", ".//shaders//pass_through.frag"),
    boundaryVelocity(".//shaders//slab_operation.vert", ".//shaders//boundary_velocity.frag"),
    boundaryLS(".//shaders//slab_operation.vert", ".//shaders//boundary_levelset.frag"),
    boundaryPressure(".//shaders//slab_operation.vert", ".//shaders//boundary_pressure.frag"),
    pressurePoisson(".//shaders//slab_operation.vert", ".//shaders//pressure_poisson.frag"),
    divergence(".//shaders//slab_operation.vert", ".//shaders//divergence.frag"),
    removeDivergence(".//shaders//slab_operation.vert", ".//shaders//remove_divergence.frag"),
    clearSlabs(".//shaders//slab_operation.vert", ".//shaders//clear_slabs.frag")
{   
    setUpFluidRenderShaders();
    setUpFluidSimulationTextures();
    setUpFluidSimulationFBOs();
};

void FluidRenderer::setUpFluidRenderShaders(){
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
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -0.5f * scale / planeSize));
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

    forceApplication.shader.useProgram();
    uniformGravityDirection = forceApplication.shader.getUniformLocation("gravityDirection");
    glUniform1f(uniformGravityDirection, gravityDirection);

}

// Generate 3D textures and set initial values for all simulated quantities
void FluidRenderer::setUpFluidSimulationTextures(){    
    glActiveTexture(GL_TEXTURE0 + 0);

    // Level set - initial surface at z = 0.5f
    // Takes the value of zero on air-water and box-water interfaces
    // ***Issue: Should be signed distance field, but using 0.5f outside due to pressure issue
    std::vector<float> tempSetData(4*gridSize*gridSize*gridSize, 0.0f);
    
    for (int k = 0; k < gridSize; ++k){
       for (int j = 0 ; j < gridSize; ++j){
            for (int i = 0; i < gridSize; ++i){
                //location of (i,j,k) in texture data is 4 * gridSize * gridSize * k + 4 * gridSize * j + 4 * i
                int index = 4 * gridSize * gridSize * k + 4 * gridSize * j + 4 * i;
                // index = 4 * gridSize * gridSize * k + 4 * gridSize * (gridSize - 1 - j) + 4 * i; // flip fluid in y direction
                // bottom layer is just outside fluid
                if ( j == 0){
                    tempSetData[index] = 0.5f;// Must be 0.5 so = 0 on bdry
                }

                // top 'half' decrements to 0.5 in j=gridSize/2 + 1 layer
                else if (j > gridSize/2){
                    tempSetData[index] = 0.5f; //same everywhere outside?
                    //tempSetData[index] = j - gridSize/2 - 0.5f;
                }

                // bottom 'half': j in [1,gridSize/2] 0.5f around outside, then -0.5f (to ensure 0 on bdry), then decrement inwards
                else{
                    if (i == 0 || k == 0 || i == gridSize-1 || k == gridSize-1){//edges
                        tempSetData[index] = 0.5f;
                    }
                    else{// Manhattan distance to edge

                        int tempDist = std::min(i-1, gridSize-2-i);
                        tempDist = std::min(tempDist, std::min(k-1, gridSize-2-k));
                        tempDist = std::min(tempDist, j-1);
                        tempDist = std::min(tempDist, gridSize/2-j);

                        tempDist += 0.5f;// Bdry condition correction
                        tempSetData[index] = -tempDist;
                    }
                }
            }
        }
    }
    
    levelSetCurrent.generateTexture(tempSetData);
    levelSetNext.generateTexture(tempSetData);

    // Velocity - initially zero everywhere
    std::vector<float> tempVelocityData(4*gridSize*gridSize*gridSize, 0.0f);

    velocityCurrent.generateTexture(tempVelocityData);
    velocityNext.generateTexture(tempVelocityData);
    
    // Pressure - initially zero 
    pressureCurrent.generateTexture(tempVelocityData);
    pressureNext.generateTexture(tempVelocityData);

    // Temporary set of buffers for use in Jacobi iteration
    tempQuantity.generateTexture(tempVelocityData);
    
};

// Generates a new 3D RGBA32F texture with the given input as the initial data
void FluidRenderer::SQ::generateTexture(std::vector<float> data){
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_3D, texture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, gridSize, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, data.data());
    glBindTexture(GL_TEXTURE_3D, 0);
}

// Generate FBOs for each simulated quantity for use with slab operations
void FluidRenderer::setUpFluidSimulationFBOs(){
    levelSetCurrent.generateFBOs();
    levelSetNext.generateFBOs();
    velocityCurrent.generateFBOs();
    velocityNext.generateFBOs();
    pressureCurrent.generateFBOs();
    pressureNext.generateFBOs();
    tempQuantity.generateFBOs();
}

// Generates an array of FBOs for using slab operations to render into the simulated quantity texture
void FluidRenderer::SQ::generateFBOs(){
    for (int zSlice = 0; zSlice < gridSize; ++zSlice){
        glGenFramebuffers(1, &(slabFBOs[zSlice]));
        glBindFramebuffer(GL_FRAMEBUFFER, slabFBOs[zSlice]);
        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, texture, 0, zSlice);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::string("Failed to initialise framebuffer\n");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FluidRenderer::frame(unsigned int frameTime){
    // ***Refactor: Update camera
    horizRot += frameTime * horizRotSpeed;
    camera.pos = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(horizRot), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0.0f, 2.0f, 3.0f, 1.0f));
    camera.updateMatrix();
    
    glDisable(GL_CULL_FACE); // is this needed????

    renderBackground();
    integrateFluid(frameTime);
    renderFluid();
}

// Renders a patterned plane below the fluid using the current camera viewpoint
void FluidRenderer::renderBackground(){
    backgroundPlaneShader.useProgram();
    glUniformMatrix4fv(backgroundPlaneUniforms.viewTrans, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));
    backgroundPlane.draw(GL_TRIANGLES);
}

// 
void FluidRenderer::integrateFluid(unsigned int frameTime){
    //std::cout << "1\n";
    // Update gravity vector ///// should this be here?
    if (resetGravity){
        gravityDirection = 0.0f;
        resetGravity = false;
    
    }
    else if (gravityRotatingPos){
        gravityDirection += frameTime * gravityRotSpeed;
    }
    else if (gravityRotatingNeg){
        gravityDirection -= frameTime * gravityRotSpeed;
    }
    ///////////////////
    
    
   
    glDisable(GL_BLEND);
    glViewport(0,0,gridSize, gridSize);
    glEnable(GL_SCISSOR_TEST);
    
    
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, levelSetCurrent.texture);

    // Apply force to velocity
    forceApplication.shader.useProgram();
    glUniform1f(uniformGravityDirection, gravityDirection); ////// gravity!!!

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture); // pos = 2
    applyInnerSlabOp(forceApplication, velocityNext, frameTime);
    std::swap(velocityCurrent, velocityNext);

    //std::cout << "2\n";
    // Velocity BC
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture); // bind advected quantity to pos = 2
    
    applyOuterSlabOp(boundaryVelocity, velocityNext, frameTime);
    std::swap(velocityCurrent, velocityNext);

    // Advect Velocity
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, levelSetCurrent.texture);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);
    applyInnerSlabOp(advection, velocityNext, frameTime);

    //Level set BC

    glBindTexture(GL_TEXTURE_3D, levelSetCurrent.texture); // Bind to pos 2
    
    applyOuterSlabOp(boundaryLS, levelSetNext, frameTime);
    std::swap(levelSetCurrent, levelSetNext);

     
    // Advect Level Set using old velocity (but with corrected BC)
    glBindTexture(GL_TEXTURE_3D, levelSetCurrent.texture);
    applyInnerSlabOp(advection, levelSetNext, frameTime);

    // Put advected velocity in current
    std::swap(velocityCurrent, velocityNext);

    // Re-bind velocity    
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);

    // Re-bind velocity as quantity to be altered in pos = 2
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);

    // pass through current velocity to temp velocity, which is used as 0th iteration
    applyInnerSlabOp(passThrough, tempQuantity, frameTime);
    // then bind temp velocity to pos = 2 
    //glBindTexture(GL_TEXTURE_3D, textureVelocityTemp);

    // Diffuse velocity
    for (int i = 0; i < numJacobiIterations; ++i){
        glBindTexture(GL_TEXTURE_3D, tempQuantity.texture);
        // Render into next velocity (kth iterate is in temp, k+1th in next)
        applyInnerSlabOp(diffusion, velocityNext, frameTime);
        // swap next and temp velocity, then iterate 
        std::swap(velocityNext, tempQuantity);
    }
        
    std::swap(velocityCurrent, tempQuantity); // Swap final iteration into current velocity

    // *** force is currently based on old level set - move swap to here and rebind LS to base it on new LS
    //std::swap(levelSet.textureCurrent, levelSet.textureNext);
    //glActiveTexture(GL_TEXTURE0 + 1);
   /// glBindTexture(GL_TEXTURE_3D, levelSet.textureCurrent);
    // ***
    // Apply force to velocity
    //glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent); // pos = 2
    //applyInnerSlabOperation(forceApplication, frameTime, velocity.textureNext);
    //std::swap(velocity.textureCurrent, velocity.textureNext);

    // *Remove divergence from velocity*

    // Apply velocity BC (must be done to ensure correct divergence at bdries)
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);
    
    applyOuterSlabOp(boundaryVelocity, velocityNext, frameTime);
    std::swap(velocityCurrent, velocityNext);

    // Compute div of currentVelocity (store in textureVelocityTemp)
    glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);
    applyInnerSlabOp(divergence, tempQuantity, frameTime);

    // Clear pressure texture
    /*applyOuterSlabOperation(clearSlabs, pressure.textureNext);
    std::swap(pressure.textureCurrent, pressure.textureNext);*/


    // Bindings
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, pressureCurrent.texture);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, levelSetCurrent.texture);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, tempQuantity.texture); // div(velocity)

    // Solve Poisson eqn 
    for (int i = 0; i < numJacobiIterationsPressure; ++i){
        // Pressure BC
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_3D, pressureCurrent.texture);

        //glActiveTexture(GL_TEXTURE0 + 1);
        //glBindTexture(GL_TEXTURE_3D, pressure.textureNext);
        
        applyOuterSlabOp(boundaryPressure, pressureNext, frameTime);
        std::swap(pressureCurrent, pressureNext);

        // Iteration (kth iteration in current, k+1th in next)
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, pressureCurrent.texture);
         glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_3D, tempQuantity.texture);

        //glActiveTexture(GL_TEXTURE0 + 1);
        //glBindTexture(GL_TEXTURE_3D, pressure.textureNext);
        // above re-binding could be avoided by relabelling in shader...
        applyInnerSlabOp(pressurePoisson, pressureNext, frameTime);

        std::swap(pressureCurrent, pressureNext);
    }

    // Subtract grad(pressure) from currentVelocity
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, velocityCurrent.texture);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, pressureCurrent.texture);

    applyInnerSlabOp(removeDivergence, velocityNext, frameTime);
    std::swap(velocityCurrent, velocityNext);

    /*
    // Enforce no-slip (should this be before pressure subtraction?)

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);

    //glActiveTexture(GL_TEXTURE0 + 2);
    //glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent); // bind advected quantity to pos = 2
    
    applyOuterSlabOperation(boundaryVelocity, velocity.textureNext);
    std::swap(velocity.textureCurrent, velocity.textureNext);
    */
    // ////

    std::swap(levelSetCurrent, levelSetNext); // consider moving up to ***
    glDisable(GL_SCISSOR_TEST);
    // Tidy up
    glViewport(0,0,screenWidth,screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0 + 0);
    
    glEnable(GL_BLEND);
}


// Renders levelSetCurrent by marching through the volume
void FluidRenderer::renderFluid(){
    // Coordinates of entry/exit points of camera ray through the cube are rendered as RGB values to texture
    raycastingPosShader.useProgram();
    glUniformMatrix4fv(raycastingPosUniforms.viewTrans, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));

    // Draw front of cube (cull back faces) in RGB to texture
    glBindFramebuffer(GL_FRAMEBUFFER, frontCube.FBO);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
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
    glBindTexture(GL_TEXTURE_3D, levelSetCurrent.texture);
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

void FluidRenderer::handleEvents(SDL_Event const& event){
   // if (event.type == SDL_MOUSEBUTTONDOWN){/*std::swap(levelSet.textureCurrent, levelSet.textureNext);*/}
   // else if (event.type == SDL_MOUSEBUTTONUP){}
    //else if(event.type == SDL_MOUSEMOTION){};
   switch(event.type){
        case SDL_KEYDOWN:
            switch(event.key.keysym.scancode){
                case SDL_SCANCODE_UP:
                    gravityRotatingPos = true;
                break;
                case SDL_SCANCODE_DOWN:
                    gravityRotatingNeg = true;
                break;
                case SDL_SCANCODE_G:
                    resetGravity = true;
                    break;
                default:
                break;
            }
            break;
        case SDL_KEYUP:
            switch(event.key.keysym.scancode){
                case SDL_SCANCODE_UP:
                    gravityRotatingPos = false;
                break;

                case SDL_SCANCODE_DOWN:
                    gravityRotatingNeg = false;
                break;
                default:
                break;
            }
            break;
        default:
        break;
    }
};

FluidRenderer::Drawable::Drawable(std::vector<float> const& verts, unsigned int vertexDimension) : 
    vertices{verts}
{
    setUpBuffers(vertexDimension);
}

FluidRenderer::Drawable::~Drawable(){
    releaseBuffers();
}

// Generates VAO and VBO for Drawable object and copies vertex/UV data into VBO
// vertDim = dimension of vertex data (used for calculating stride)
void FluidRenderer::Drawable::setUpBuffers(unsigned int vertDim){
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

void FluidRenderer::Drawable::releaseBuffers(){
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void FluidRenderer::Drawable::draw(GLint drawingMode){
    glBindVertexArray(VAO);
    glDrawArrays(drawingMode, 0, vertices.size());
    glBindVertexArray(0);
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

FluidRenderer::SlabOperation::SlabOperation(const std::string vertexShaderPath, const std::string fragmentShaderPath) :
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
    
    
};

// ***Issue: ideally these would be a member of slab op, but then they don't have access to quad!
void FluidRenderer::applySlabOp(SlabOperation slabOp, SQ quantity, unsigned int frameTime, int layerFrom, int layerTo){
    slabOp.shader.useProgram();
    glUniform1f(slabOp.uniformTimeStep, (float)frameTime);
    for (int zSlice = layerFrom; zSlice < layerTo; ++zSlice){
        glBindFramebuffer(GL_FRAMEBUFFER, quantity.slabFBOs[zSlice]);
        glUniform1i(slabOp.uniformZSlice, zSlice);
        quad.draw(GL_TRIANGLES);
    }
}

void FluidRenderer::applyInnerSlabOp(innerSlabOp slabOp, SQ quantity, unsigned int frameTime){
    glScissor(1,1,gridSize-2,gridSize-2);
    applySlabOp(slabOp, quantity, frameTime, 1, gridSize-1);
}

void FluidRenderer::applyOuterSlabOp(outerSlabOp slabOp, SQ quantity, unsigned int frameTime){
    glScissor(0,0,gridSize,gridSize);
    applySlabOp(slabOp, quantity, frameTime, 0, gridSize);
}
