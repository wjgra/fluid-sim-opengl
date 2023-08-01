#include "../include/fluid_renderer.hpp"

FluidRenderer::FluidRenderer(unsigned int width, unsigned int height) :
    screenWidth{width},
    screenHeight{height},
    frontCube{width, height},
    backCube{width, height},
    backgroundPlaneShader{".//shaders//background_plane.vert", ".//shaders//background_plane.frag"},
    raycastingPosShader(".//shaders//raycasting_pos.vert", ".//shaders//raycasting_pos.frag"),
    renderFluidShader(".//shaders//fluid.vert", ".//shaders//fluid.frag"),
    integrateFluidShader(".//shaders//integrate_fluid.vert", ".//shaders//integrate_fluid.frag"),
    advectLevelSetShader(".//shaders//advect_levelset.vert", ".//shaders//advect_levelset.frag")
{
    setDrawableUniformValues();

    setUpFluidData();
    
    // Integration shader
    integrateFluidShader.useProgram();

    integrateFluidUniforms.modelTrans = integrateFluidShader.getUniformLocation("model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(gridSize, gridSize, 1));
    //model = glm::scale(model, glm::vec3(screenWidth, screenHeight, 1));
    model = glm::translate(model, glm::vec3(0.0f,0.0f,0.0f));  // should be (1,1,0) , but without any scaling!
    glUniformMatrix4fv(integrateFluidUniforms.modelTrans, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    glm::mat4 projection = glm::ortho(0.0f, (float)gridSize,  0.0f, (float)gridSize, -1.0f, 1.0f);
    integrateFluidUniforms.projTrans = integrateFluidShader.getUniformLocation("projection");
    glUniformMatrix4fv(integrateFluidUniforms.projTrans, 1, GL_FALSE, glm::value_ptr(projection));

    uniformTimeStep = integrateFluidShader.getUniformLocation("timeStep");
    uniformSlice = integrateFluidShader.getUniformLocation("zSlice");


    // Set texture indices / get locations (VEL ADV SHADER)
    velocity.uniformCurrent = integrateFluidShader.getUniformLocation("velocityTexture");
    glUniform1i(velocity.uniformCurrent, 0);
    glActiveTexture(GL_TEXTURE0 + 0);
    //glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);



    // LS ADV SHADER
    advectLevelSetShader.useProgram();

    // Proj/model
    advectLSUniforms.modelTrans = advectLevelSetShader.getUniformLocation("model");
    model = glm::translate(model, glm::vec3(0.0f,0.0f,0.0f));
    glUniformMatrix4fv(advectLSUniforms.modelTrans, 1, GL_FALSE, glm::value_ptr(model));
    advectLSUniforms.projTrans = advectLevelSetShader.getUniformLocation("projection");
    glUniformMatrix4fv(advectLSUniforms.projTrans, 1, GL_FALSE, glm::value_ptr(projection));

    // TS/slice
    uniformSliceLS = advectLevelSetShader.getUniformLocation("zSlice");
    uniformTimeStepLS = advectLevelSetShader.getUniformLocation("timeStep");

    uniformLSVelocity = advectLevelSetShader.getUniformLocation("velocityTexture");
    glUniform1i(uniformLSVelocity, 0);
    
    levelSet.uniformCurrent = advectLevelSetShader.getUniformLocation("levelSetTexture");
    glUniform1i(levelSet.uniformCurrent, 1);
    // Textures
    
  
    glActiveTexture(GL_TEXTURE0 + 0);
};

// Generates VAO and VBO for Drawable object and copies vertex/UV data into VBO
// @param vertDim = dimension of vertex data (used for calculating stride)
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

void FluidRenderer::frame(unsigned int frameTime){
    horizRot += frameTime * horizRotSpeed;
    
    camera.pos = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(horizRot), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0.0f, 2.0f, 3.0f, 1.0f));
    camera.updateMatrix();
    
    glDisable(GL_CULL_FACE);
    backgroundPlaneShader.useProgram();
    glUniformMatrix4fv(backgroundPlaneUniforms.viewTrans, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));
    backgroundPlane.draw(GL_TRIANGLES);

    integrateFluid(frameTime);


    // Temp rotation
    raycastingPosShader.useProgram();
   //glm::mat4 view = glm::lookAt(glm::vec3(2.0f*cos(glm::radians(horizRot)), 2.0f, -2.0f*sin(glm::radians(horizRot))), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(2.0f*sin(glm::radians(horizRot)), 2.0f, 2.0f*cos(glm::radians(horizRot))));
    glUniformMatrix4fv(raycastingPosUniforms.viewTrans, 1, GL_FALSE, glm::value_ptr(camera.viewMatrix));

    // Draw front to texture
    glBindFramebuffer(GL_FRAMEBUFFER, frontCube.FBO);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glDisable(GL_CULL_FACE);
    // No depth buffer, so have to cull back faces
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);//raycastingPosShader.useProgram(); // do I need to reactivate this? no...
    cube.draw(GL_TRIANGLES);

    glBindFramebuffer(GL_FRAMEBUFFER, backCube.FBO);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    cube.draw(GL_TRIANGLES);;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_CULL_FACE); 
    
    renderFluidShader.useProgram();

    glActiveTexture(GL_TEXTURE0 + 0);
    frontCube.texture.bind();
    glActiveTexture(GL_TEXTURE0 + 1);
    backCube.texture.bind();
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, levelSet.textureCurrent);

    quad.draw(GL_TRIANGLES);

    // unbind
    glActiveTexture(GL_TEXTURE0 + 0);
    frontCube.texture.unbind();
    glActiveTexture(GL_TEXTURE0 + 1);
    backCube.texture.unbind();
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, 0);

    glActiveTexture(GL_TEXTURE0 + 0);
    
    //integrateFluid(frameTime);
};

void FluidRenderer::handleEvents(SDL_Event const& event){
    if (event.type == SDL_MOUSEBUTTONDOWN){/*std::swap(levelSet.textureCurrent, levelSet.textureNext);*/}
    else if (event.type == SDL_MOUSEBUTTONUP){}
    else if(event.type == SDL_MOUSEMOTION){};
};


void FluidRenderer::setDrawableUniformValues(){
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
    //model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
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
}

// Generate 3D textures and set initial values for level set, velocity and pressure
void FluidRenderer::setUpFluidData(){    
    glActiveTexture(GL_TEXTURE0 + 0);
    // Level set - initial surface at z = 0.5f
    std::vector<float> tempSetData(gridSize*gridSize*gridSize);
    
    for (int i = 0; i < gridSize ; ++i){
        for (int j = 0; j < gridSize*gridSize; ++j){
           tempSetData[i*gridSize*gridSize + j] = float(gridSize/2 - i); // Int division intentional
        }
    }

    glGenTextures(1, &levelSet.textureCurrent);
    //glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, levelSet.textureCurrent);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    /*glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    float tempLS = 0.0f;
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, &tempLS);*/
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, tempSetData.data());
    glBindTexture(GL_TEXTURE_3D, 0);


    // Next LevelSet
    std::vector<float> tempSetData2(gridSize*gridSize*gridSize);
    
    for (int i = 0; i < gridSize ; ++i){
        for (int j = 0; j < gridSize*gridSize; ++j){
           tempSetData2[i*gridSize*gridSize + j] = float(i - gridSize/2); // Int division intentional
        }
    }

    glGenTextures(1, &levelSet.textureNext);
    //glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_3D, levelSet.textureNext);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    /*glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, &tempLS);*/
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, tempSetData.data());
    glBindTexture(GL_TEXTURE_3D, 0);

    // Velocity  - initially parallel to (1,1,1)

    float fluidVel = 1.0f;
    std::vector<float> tempVelocityData(4*gridSize*gridSize*gridSize, float(fluidVel)); // Ignore a component

    glGenTextures(1, &velocity.textureCurrent);
    //glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    /*glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    float tempVel[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, tempVel);*/
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, gridSize, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, tempVelocityData.data());
    glBindTexture(GL_TEXTURE_3D, 0);

    /*
    // Pressure - initially just hydrostatic
    std::vector<float> tempPressureData(gridSize*gridSize*gridSize, 0.0f);
 
    //values!!!
    
    for (int i = 0; i < gridSize ; ++i){
        for (int j = 0; j < gridSize*gridSize; ++j){
            // Each i is a z-slice?
            tempPressureData[i*gridSize*gridSize + j] = (i / gridSize) * gValue * rho;
        }
    }

    glGenTextures(1, &pressure.textureCurrent);
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_3D, pressure.textureCurrent);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, tempPressureData.data());
    glBindTexture(GL_TEXTURE_3D, 0);*/

    // Next Velocity 

    //std::vector<float> tempVelocityData2(3*gridSize*gridSize*gridSize, float(0.2f * 1e-6));

    glGenTextures(1, &velocity.textureNext);
    //glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_3D, velocity.textureNext);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    /*glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, tempVel);*/
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, gridSize, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, tempVelocityData.data());
    glBindTexture(GL_TEXTURE_3D, 0);    
};

void FluidRenderer::setUpSlices(){
    // Gen textures and bind to FBOs
    
    //glGenFramebuffers(1, &FBOPressureSlice);
    
}

void FluidRenderer::integrateFluid(unsigned int frameTime){
    
    /*
    // Advect velocity
    glGenFramebuffers(1, &FBOVelocitySlice);
    glBindFramebuffer(GL_FRAMEBUFFER, FBOVelocitySlice);
    
    // for each slice, render quad into slice of 3D texture, excluding outer pixels
    for (int zSlice = 1; zSlice < gridSize - 1; ++zSlice){
        

        //glBindFramebuffer(GL_FRAMEBUFFER, FBOVelocitySlice);
        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, velocity.textureNext, 0, zSlice);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::string("Failed to initialise framebuffer\n");
        
        
        glViewport(1, 1, gridSize-1, gridSize-1);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        integrateFluidShader.useProgram();
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);
        glUniform1i(uniformSlice, zSlice);
        glUniform1f(uniformTimeStep, (float)frameTime);

        quad.draw(GL_TRIANGLES);
    }
    */

    //glGenFramebuffers(1, &FBOLevelSetSlice);
    //glBindFramebuffer(GL_FRAMEBUFFER, FBOLevelSetSlice);    
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0,0,gridSize, gridSize);
    
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, levelSet.textureCurrent);
    
    glActiveTexture(GL_TEXTURE0 + 0);
    //glActiveTexture(GL_TEXTURE0 + 2);
    //glBindTexture(GL_TEXTURE_3D, 0);
    advectLevelSetShader.useProgram();



    //quad.draw(GL_TRIANGLES);
    //glViewport(0,0,screenWidth,screenHeight);

    //int zSlice = 6;
    //glUniform1i(uniformSliceLS, zSlice);
    glUniform1f(uniformTimeStepLS, (float)frameTime);

    glGenFramebuffers(1, &FBOLevelSetSlice);
    glBindFramebuffer(GL_FRAMEBUFFER, FBOLevelSetSlice);
    /*    
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glBindFramebuffer(GL_FRAMEBUFFER, 0)
    glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, levelSet.textureNext, 0, zSlice);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::string("Failed to initialise framebuffer\n");
    
    //GLfloat temp = -1.0f;
    //glClearBufferfv(GL_COLOR, 0, &temp);
    quad.draw(GL_TRIANGLES);
    */
    //std::swap(velocity.textureCurrent, velocity.textureNext);

    //zSlice++;
    for (int zSlice = 0; zSlice < gridSize; ++zSlice){
        glUniform1i(uniformSliceLS, zSlice);
        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, levelSet.textureNext, 0, zSlice);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                throw std::string("Failed to initialise framebuffer\n");
        quad.draw(GL_TRIANGLES);
    }
    // Tidy up
    glViewport(0,0,screenWidth,screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    std::swap(levelSet.textureCurrent, levelSet.textureNext);

    /*for (int zSlice = 1; zSlice < gridSize - 1; ++zSlice){
        glUniform1i(uniformSliceLS, zSlice);
        glUniform1f(uniformTimeStepLS, (float)frameTime);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);*/
        //glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, levelSet.textureNext, 0, zSlice);
        /*if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw std::string("Failed to initialise framebuffer\n");
        */
        //glViewport(1, 1, gridSize-1, gridSize-1);
        
        // GLfloat temp = -1.0f;
        //glClearBufferfv(GL_COLOR, 0, &temp);
/*
        glDisable(GL_CULL_FACE); 

        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        quad.draw(GL_TRIANGLES);
    }
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, 0);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
    glActiveTexture(GL_TEXTURE0);

    std::swap(velocity.textureCurrent, velocity.textureNext);
    std::swap(levelSet.textureCurrent, levelSet.textureNext);*/
}