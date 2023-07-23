#include "../include/fluid_renderer.hpp"

FluidRenderer::FluidRenderer(unsigned int width, unsigned int height) :
    fluidShader(".//shaders//fluid.vert", ".//shaders//fluid.frag"),
    raycastingPosShader(".//shaders//raycasting_pos.vert", ".//shaders//raycasting_pos.frag"),
    screenWidth{width},
    screenHeight{height},
    frontCube{width, height},
    backCube{width, height}    
{
    setUpBuffers();
    setUpLevelSet();

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
    glActiveTexture(GL_TEXTURE0 + 0);

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

void FluidRenderer::setUpLevelSet(){




    const int gridSize = 32;
    std::vector<float> tempSetData(4*gridSize*gridSize*gridSize, 1.0f);
    for (auto&& iter = tempSetData.begin(); iter != tempSetData.begin() + 2*gridSize*gridSize*gridSize;){
        *iter = 0.0f; ++iter;  //R
        *iter = 0.0f; ++iter;  //G
        *iter = 0.0f; ++iter;  //B
        *iter = 0.0f; ++iter;  //A
    }
    for (auto&& iter = tempSetData.begin() + 2*gridSize*gridSize*gridSize; iter != tempSetData.end();){
        *iter = 0.0f; ++iter;  //R
        *iter = 0.0f; ++iter;  //G
        *iter = 1.0f; ++iter;  //B
        *iter = 0.5f; ++iter;  //A
    }
    glGenTextures(1, &levelSetTexture);
    glBindTexture(GL_TEXTURE_3D, levelSetTexture);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, gridSize, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, tempSetData.data());
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