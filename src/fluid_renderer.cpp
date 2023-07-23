#include "../include/fluid_renderer.hpp"

FluidRenderer::FluidRenderer(unsigned int width, unsigned int height) :
    fluidShader(".//shaders//fluid.vert", ".//shaders//fluid.frag"),
    raycastingPosShader(".//shaders//raycasting_pos.vert", ".//shaders//raycasting_pos.frag"),
    //integrationShader(".//shaders//integrate_fluid.vert", ".//shaders//integrate_fluid.frag"),
    screenWidth{width},
    screenHeight{height},
    frontCube{width, height},
    backCube{width, height}
{
    setUpBuffers();
    setUpFluidData();
    //setUpSlices();

    raycastingPosShader.useProgram();
    uniformModelTransRaycast = raycastingPosShader.getUniformLocation("model");
    if (uniformModelTransRaycast < 0)
       throw "Failed to get location of uniform \'model\'";

    //temp
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::translate(trans, glm::vec3(0, 0, 0.0f));
    trans = glm::rotate(trans, glm::radians(horizRot), glm::vec3(1.0f, 1.0f, 0.0f));
    trans = glm::scale(trans, glm::vec3(scale,scale, 1));
    glUniformMatrix4fv(uniformModelTransRaycast, 1, GL_FALSE, glm::value_ptr(trans));

    uniformProjTransRaycast = raycastingPosShader.getUniformLocation("projection");
    if (uniformProjTransRaycast < 0)
        throw "Failed to get location of uniform \'projection\'";

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width/(float)height, 0.1f, 100.0f);
    glUniformMatrix4fv(uniformProjTransRaycast, 1, GL_FALSE, glm::value_ptr(projection));

    uniformViewTransRaycast = raycastingPosShader.getUniformLocation("view");
    if (uniformViewTransRaycast < 0)
       throw "Failed to get location of uniform \'view\'";

    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    glUniformMatrix4fv(uniformViewTransRaycast, 1, GL_FALSE, glm::value_ptr(view));


    // Temp uniforms - can we make these common?

    fluidShader.useProgram();
    uniformModelTrans = fluidShader.getUniformLocation("model");
    if (uniformModelTrans < 0)
       throw "Failed to get location of uniform \'model\'";

    
    trans = glm::mat4(1.0f);
    trans = glm::scale(trans, glm::vec3(screenWidth, screenHeight, 1)); 
    trans = glm::translate(trans, glm::vec3(0, 0, 0.0f));
    glUniformMatrix4fv(uniformModelTrans, 1, GL_FALSE, glm::value_ptr(trans));

    //glUniformMatrix4fv(uniformModelTransRaycast, 1, GL_FALSE, glm::value_ptr(trans));
    projection = glm::ortho(0.0f, (float)screenWidth,  (float)screenHeight, 0.0f, -1.0f, 1.0f);
    uniformProjTrans = fluidShader.getUniformLocation("projection");
    if (uniformProjTrans < 0)
        throw "Failed to get location of uniform \'projection\'";

    glUniformMatrix4fv(uniformProjTrans, 1, GL_FALSE, glm::value_ptr(projection));

    //Temp
    setUpFramebuffer(&FBOFront, &frontCube);
    setUpFramebuffer(&FBOBack, &backCube);

    uniformFrontTexture =  fluidShader.getUniformLocation("frontTexture");
    if (uniformFrontTexture < 0 )
        throw "Failed to get location of uniform \'frontTexture\'";
    uniformBackTexture = fluidShader.getUniformLocation("backTexture");
    if (uniformBackTexture < 0 )
        throw "Failed to get location of uniform \'backTexture\'";
    //fluidShader.useProgram();
    glUniform1i(uniformFrontTexture, 0);
    glUniform1i(uniformBackTexture, 1);


    // level set
    uniformLevelSetTexture = fluidShader.getUniformLocation("levelSetTexture");
    if (uniformLevelSetTexture < 0)
        throw "Failed to get location of uniform \'levelSetTexture\'";

    glUniform1i(uniformLevelSetTexture, 2);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, levelSetTexture);
    
    // velocity field
    uniformVelocityTexture = fluidShader.getUniformLocation("velocityTexture");
    if (uniformVelocityTexture < 0)
        throw "Failed to get location of uniform \'velocityTexture\'";

    glUniform1i(uniformVelocityTexture, 3);
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_3D, velocityTexture);
    
    // pressure field
    uniformPressureTexture = fluidShader.getUniformLocation("pressureTexture");
    if (uniformPressureTexture < 0)
        throw "Failed to get location of uniform \'pressureTexture\'";

    glUniform1i(uniformPressureTexture, 4);
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_3D, pressureTexture);
    

    // NEXT VALUES

    // level set

    uniformNextLevelSetTexture = fluidShader.getUniformLocation("nextLevelSetTexture");
    if (uniformNextLevelSetTexture < 0)
        throw "Failed to get location of uniform \'nextLevelSetTexture\'";

    glUniform1i(uniformNextLevelSetTexture, 5);
    glActiveTexture(GL_TEXTURE0 + 5);
    glBindTexture(GL_TEXTURE_3D, nextLevelSetTexture);
    
    // velocity field
    uniformNextVelocityTexture = fluidShader.getUniformLocation("nextVelocityTexture");
    if (uniformNextVelocityTexture < 0)
        throw "Failed to get location of uniform \'nextVelocityTexture\'";

    glUniform1i(uniformNextVelocityTexture, 6);
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_3D, nextVelocityTexture);
    
    // pressure field
    uniformNextPressureTexture = fluidShader.getUniformLocation("nextPressureTexture");
    if (uniformNextPressureTexture < 0)
        throw "Failed to get location of uniform \'nextPressureTexture\'";

    glUniform1i(uniformNextPressureTexture, 7);
    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_3D, nextPressureTexture);


    glActiveTexture(GL_TEXTURE0 + 0);

    /*
    // Integration shader
    integrationShader.useProgram();
    uniformModelTransIntegrate = integrationShader.getUniformLocation("model");
    if (uniformModelTransIntegrate < 0)
       throw "Failed to get location of uniform \'model\'";

    glUniformMatrix4fv(uniformModelTransIntegrate, 1, GL_FALSE, glm::value_ptr(trans));

    uniformProjTransIntegrate = integrationShader.getUniformLocation("projection");
    if (uniformProjTransIntegrate < 0)
        throw "Failed to get location of uniform \'projection\'";

    glUniformMatrix4fv(uniformProjTransIntegrate, 1, GL_FALSE, glm::value_ptr(projection));
    */


};

FluidRenderer::~FluidRenderer(){
    releaseBuffers();
};

void FluidRenderer::setUpBuffers(){
    // Generate buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind VAO
    glBindVertexArray(VAO); 

    // Bind VBO and copy vertex data into VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Verts
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    // UVs
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));

    // Bind EBO and copy element data into EBO
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size()*sizeof(GLuint), elements.data(), GL_STATIC_DRAW);

    // Unbind VAO
    glBindVertexArray(0);


    // Quads
      // Generate buffers
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glGenBuffers(1, &quadEBO);

    // Bind VAO
    glBindVertexArray(quadVAO); 

    // Bind VBO and copy vertex data into VBO
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    // Verts
    glBufferData(GL_ARRAY_BUFFER, quadVertices.size()*sizeof(float), quadVertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    // UVs
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));

    // Bind EBO and copy element data into EBO
     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEBO);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, quadElements.size()*sizeof(GLuint), quadElements.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);


};

void FluidRenderer::releaseBuffers(){
    // Delete buffers
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    // Quads
     glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteBuffers(1, &quadEBO);
};

void FluidRenderer::draw(){
    glBindVertexArray(VAO);
    raycastingPosShader.useProgram();
    //glDrawElements(drawingMode, elements.size(), GL_UNSIGNED_INT, 0);
    glDrawArrays(drawingMode, 0, 36);
    glBindVertexArray(0);
};

void FluidRenderer::frame(unsigned int frameTime){
    //integrateFluid(frameTime);

    horizRot += frameTime * rotSpeed;
    // std::cout << horizRot <<"\n";
    raycastingPosShader.useProgram();
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::translate(trans, glm::vec3(0, 0, 0.0f));
    trans = glm::rotate(trans, glm::radians(horizRot), glm::vec3(0.0f, 1.0f, 0.0f));
    trans = glm::rotate(trans, -1.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    trans = glm::scale(trans, glm::vec3(scale,scale, 1));
    glUniformMatrix4fv(uniformModelTransRaycast, 1, GL_FALSE, glm::value_ptr(trans));

    // Draw front to texture
    glBindFramebuffer(GL_FRAMEBUFFER, FBOFront);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // glDisable(GL_CULL_FACE);
    // No depth buffer, so have to cull back faces
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    draw();

    glBindFramebuffer(GL_FRAMEBUFFER, FBOBack);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    draw();


    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glDisable(GL_CULL_FACE); 
    
    fluidShader.useProgram();
    //glUniformMatrix4fv(uniformModelTrans, 1, GL_FALSE, glm::value_ptr(trans));
    glActiveTexture(GL_TEXTURE0 + 0);
    frontCube.bind();
    glActiveTexture(GL_TEXTURE0 + 1);
    backCube.bind();
    glBindVertexArray(quadVAO);
    glDrawElements(GL_TRIANGLE_STRIP, quadElements.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // unbind
    glActiveTexture(GL_TEXTURE0 + 0);
    frontCube.unbind();
    glActiveTexture(GL_TEXTURE0 + 1);
    backCube.unbind();

    glActiveTexture(GL_TEXTURE0 + 0);
    

};

void FluidRenderer::handleEvents(SDL_Event const& event){
    if (event.type == SDL_MOUSEBUTTONDOWN){}
    else if (event.type == SDL_MOUSEBUTTONUP){}
    else if(event.type == SDL_MOUSEMOTION){};
    
    /*
    if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_SPACE){
        //rotSpeed = 0;
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);    
    }
    else if (event.type == SDL_KEYUP && event.key.keysym.scancode == SDL_SCANCODE_SPACE){
        //rotSpeed = glm::radians(1500.0f)*1e-6;
        glDisable(GL_CULL_FACE);
    }
    */
};

void FluidRenderer::setUpFluidData(){
    // Level set - initial surface at z = 0.5f
    std::vector<float> tempSetData(gridSize*gridSize*gridSize);
    
    for (int i = 0; i < gridSize ; ++i){
        for (int j = 0; j < gridSize*gridSize; ++j){
             float temp= float(gridSize/2 - i);
             int temp2 = gridSize/2 - i;
           tempSetData[i*gridSize*gridSize + j] = float(gridSize/2 - i); // Int division intentional
        }
    }

    glGenTextures(1, &levelSetTexture);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, levelSetTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, gridSize, gridSize, gridSize, 0, GL_R32F, GL_FLOAT, tempSetData.data());
    glBindTexture(GL_TEXTURE_3D, 0);

    // Velocity  - initially zero

    std::vector<float> tempVelocityData(3*gridSize*gridSize*gridSize, 0.0f);

    glGenTextures(1, &velocityTexture);
    glActiveTexture(GL_TEXTURE0 + 3);
    glBindTexture(GL_TEXTURE_3D, velocityTexture);
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
    const float gValue = 9.81;
    const float rho = 997;
    for (int i = 0; i < gridSize ; ++i){
        for (int j = 0; j < gridSize*gridSize; ++j){
            // Each i is a z-slice?
            tempPressureData[i*gridSize*gridSize + j] = (i / gridSize) * gValue * rho;
        }
    }


    glGenTextures(1, &pressureTexture);
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_3D, pressureTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, gridSize, gridSize, gridSize, 0, GL_RED, GL_FLOAT, tempPressureData.data());
    glBindTexture(GL_TEXTURE_3D, 0);


    // NEXT VALUES
    
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
    
};


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



    
}