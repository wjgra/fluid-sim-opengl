#include "../include/fluid_renderer.hpp"

FluidRenderer::FluidRenderer(unsigned int width, unsigned int height) :
    renderFluidShader(".//shaders//fluid.vert", ".//shaders//fluid.frag"),
    raycastingPosShader(".//shaders//raycasting_pos.vert", ".//shaders//raycasting_pos.frag"),
    //integrateFluidShader(".//shaders//integrate_fluid.vert", ".//shaders//integrate_fluid.frag"),
    screenWidth{width},
    screenHeight{height},
    frontCube{width, height},
    backCube{width, height}
{
    setDrawableUniformValues();

    setUpFluidData();
    //setUpSlices();

    renderFluidShader.useProgram();
    // Bind level set to texture 2
    levelSet.uniformCurrent = renderFluidShader.getUniformLocation("levelSetTexture");
    glUniform1i(levelSet.uniformCurrent, 2);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, levelSet.textureCurrent);
    
    /*
    // Bind velocity field to texture 3
    velocity.uniformCurrent = renderFluidShader.getUniformLocation("velocityTexture");
    glUniform1i(velocity.uniformCurrent, 3);
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);
    
    // Bind pressure filed to texture 4
    pressure.uniformCurrent = renderFluidShader.getUniformLocation("pressureTexture");
    glUniform1i(pressure.uniformCurrent, 4);
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_3D, pressure.textureCurrent);
    */

    // NEXT VALUES /////////////////////

    // level set
    /*
    uniformNextLevelSetTexture = renderFluidShader.getUniformLocation("nextLevelSetTexture");
   

    glUniform1i(uniformNextLevelSetTexture, 5);
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_3D, nextLevelSetTexture);
    
    // velocity field
    uniformNextVelocityTexture = renderFluidShader.getUniformLocation("nextVelocityTexture");


    glUniform1i(uniformNextVelocityTexture, 6);
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_3D, nextVelocityTexture);
    
    // pressure field
    uniformNextPressureTexture = renderFluidShader.getUniformLocation("nextPressureTexture");
 

    glUniform1i(uniformNextPressureTexture, 7);
    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_3D, nextPressureTexture);
    */

    glActiveTexture(GL_TEXTURE0 + 0);

    /*
    // Integration shader
    integrationShader.useProgram();
    uniformModelTransIntegrate = integrationShader.getUniformLocation("model");
   

    glUniformMatrix4fv(uniformModelTransIntegrate, 1, GL_FALSE, glm::value_ptr(trans));

    uniformProjTransIntegrate = integrationShader.getUniformLocation("projection");

    glUniformMatrix4fv(uniformProjTransIntegrate, 1, GL_FALSE, glm::value_ptr(projection));
    */


};

// Generates VAO and VBO for Drawable object and copies vertex/UV data into VBO
// @param vertDim = dimension of vertex data (used for calculating stride)
void FluidRenderer::Drawable::setUpBuffers(unsigned int vertDim){
    if (vertDim != 2 && vertDim != 3){
        throw std::string("Failed to set up buffer object. Vertex dimension must be 2 or 3.\n");
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
    //integrateFluid(frameTime);

    horizRot += frameTime * horizRotSpeed;
    raycastingPosShader.useProgram();
    /*glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0, 0, 0.0f));
    model = glm::rotate(model, glm::radians(horizRot), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, vertRot, glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(scale,scale, 1));
    glUniformMatrix4fv(raycastingPosUniforms.modelTrans, 1, GL_FALSE, glm::value_ptr(model));*/

    // Temp rotation
    camera.pos = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(horizRot), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0.0f, 2.0f, -3.0f, 1.0f));
    camera.updateMatrix();

   //glm::mat4 view = glm::lookAt(glm::vec3(2.0f*cos(glm::radians(horizRot)), 2.0f, -2.0f*sin(glm::radians(horizRot))), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(2.0f*sin(glm::radians(horizRot)), 2.0f, 2.0f*cos(glm::radians(horizRot))));
    glUniformMatrix4fv(raycastingPosUniforms.viewTrans, 1, GL_FALSE, glm::value_ptr(camera.cameraMat));

    // Draw front to texture
    glBindFramebuffer(GL_FRAMEBUFFER, frontCube.FBO);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glDisable(GL_CULL_FACE);
    // No depth buffer, so have to cull back faces
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);//raycastingPosShader.useProgram(); // do I need to reactivate this?
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
    quad.draw(GL_TRIANGLES);
    /*glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLE_STRIP, quadElements.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);*/

    // unbind
    glActiveTexture(GL_TEXTURE0 + 0);
    frontCube.texture.unbind();
    glActiveTexture(GL_TEXTURE0 + 1);
    backCube.texture.unbind();

    glActiveTexture(GL_TEXTURE0 + 0);
    

};

void FluidRenderer::handleEvents(SDL_Event const& event){
    if (event.type == SDL_MOUSEBUTTONDOWN){}
    else if (event.type == SDL_MOUSEBUTTONUP){}
    else if(event.type == SDL_MOUSEMOTION){};
    
    /*
    if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_SPACE){
        //horizRotSpeed = 0;
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);    
    }
    else if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_SPACE){
        //horizRotSpeed = glm::radians(1500.0f)*1e-6;
        glDisable(GL_CULL_FACE);
    }
    */
};


void FluidRenderer::setDrawableUniformValues(){
    // Get uniform locations and set values for raycastingPosShader
    raycastingPosShader.useProgram();
    
    // Model matrix
    raycastingPosUniforms.modelTrans = raycastingPosShader.getUniformLocation("model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(scale,scale, 1));
    glUniformMatrix4fv(raycastingPosUniforms.modelTrans, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    raycastingPosUniforms.projTrans = raycastingPosShader.getUniformLocation("projection");
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth/(float)screenHeight, 0.1f, 100.0f);
    glUniformMatrix4fv(raycastingPosUniforms.projTrans, 1, GL_FALSE, glm::value_ptr(projection));

    // View matrix
    raycastingPosUniforms.viewTrans = raycastingPosShader.getUniformLocation("view");
    camera.updateMatrix();
    glm::mat4 view = camera.cameraMat;//::lookAt(glm::vec3(0.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 2.0f, 2.0f));
    glUniformMatrix4fv(raycastingPosUniforms.viewTrans, 1, GL_FALSE, glm::value_ptr(view));


    // Get uniform locations and set values for renderFluidShader - note symmetry with above (can we condense?)
    renderFluidShader.useProgram();

    // Model matrix
    renderFluidUniforms.modelTrans = renderFluidShader.getUniformLocation("model");
    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(screenWidth, screenHeight, 1)); 
    model = glm::translate(model, glm::vec3(0, 0, 0.0f));
    glUniformMatrix4fv(renderFluidUniforms.modelTrans, 1, GL_FALSE, glm::value_ptr(model));

    // Projection matrix
    projection = glm::ortho(0.0f, (float)screenWidth,  (float)screenHeight, 0.0f, -1.0f, 1.0f);
    renderFluidUniforms.projTrans = renderFluidShader.getUniformLocation("projection");
    glUniformMatrix4fv(renderFluidUniforms.projTrans, 1, GL_FALSE, glm::value_ptr(projection));

    // No view matrix required in orthogonal projection

    // Set up uniforms for 'cube vectors'
    frontCube.uniformTexture =  renderFluidShader.getUniformLocation("frontTexture");
    backCube.uniformTexture = renderFluidShader.getUniformLocation("backTexture");
    renderFluidShader.useProgram();
    glUniform1i(frontCube.uniformTexture, 0);
    glUniform1i(backCube.uniformTexture, 1);
}

// Generate 3D textures and set initial values for level set, velocity and pressure
void FluidRenderer::setUpFluidData(){    
    // Level set - initial surface at z = 0.5f
    std::vector<float> tempSetData(gridSize*gridSize*gridSize);
    
    for (int i = 0; i < gridSize ; ++i){
        for (int j = 0; j < gridSize*gridSize; ++j){
           tempSetData[i*gridSize*gridSize + j] = float(gridSize/2 - i); // Int division intentional
        }
    }

    glGenTextures(1, &levelSet.textureCurrent);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, levelSet.textureCurrent);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, tempSetData.data());
    glBindTexture(GL_TEXTURE_3D, 0);

    // Velocity  - initially zero

    std::vector<float> tempVelocityData(3*gridSize*gridSize*gridSize, 0.0f);

    glGenTextures(1, &velocity.textureCurrent);
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, gridSize, gridSize, gridSize, 0, GL_RGB, GL_FLOAT, tempVelocityData.data());
    glBindTexture(GL_TEXTURE_3D, 0);


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
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, tempPressureData.data());
    glBindTexture(GL_TEXTURE_3D, 0);


    // NEXT VALUES
    /*
    // level set
    glGenTextures(1, &nextLevelSetTexture);
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_3D, nextLevelSetTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, tempSetData.data());
    glBindTexture(GL_TEXTURE_3D, 0);

    // Velocity 

    glGenTextures(1, &nextVelocityTexture);
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_3D, nextVelocityTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, gridSize, gridSize, gridSize, 0, GL_RGB, GL_FLOAT, tempVelocityData.data());
    glBindTexture(GL_TEXTURE_3D, 0);    

    // Pressure

    glGenTextures(1, &nextPressureTexture);
    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_3D, nextPressureTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, tempPressureData.data());
    glBindTexture(GL_TEXTURE_3D, 0);
    */
};

/*
void FluidRenderer::setUpFramebuffer(GLuint* framebuffer, Texture* texture){
    glGenFramebuffers(1, framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->getLocation(), 0);

    // Check complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw "Failed to initialise framebuffer";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FluidRenderer::releaseFramebuffer(GLuint* framebuffer){
    glDeleteFramebuffers(1, framebuffer);
}
*/

/*
void FluidRenderer::setUpSlices(){
    // Gen textures and bind to FBOs
    glGenFramebuffers(1, &FBOVelocitySlice);
    glGenFramebuffers(1, &FBOPressureSlice);
    glGenFramebuffers(1, &FBOLevelSetSlice);
}

void FluidRenderer::integrateFluid(unsigned int frameTime){
    glBindFramebuffer(GL_FRAMEBUFFER, FBOVelocitySlice);
    // for each slice, render quad into slice of 3D texture, excluding outer pixels
    for (int zSlice = 0; zSlice < gridSize; ++zSlice){
        glFramebufferTexture3D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_3D, nextVelocityTexture, 0, zSlice);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            throw "Failed to initialise framebuffer";

        glViewport(1, 1, gridSize-1, gridSize-1);

        glBindVertexArray(quadVAO);
        glDrawElements(GL_TRIANGLE_STRIP, quadElements.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

    }
}*/