#include "../include/fluid_renderer.hpp"
/*
FluidRenderer::Drawable FluidRenderer::quad = FluidRenderer::Drawable({
        1.0f, 0.0f,   1.0f, 0.0f,   // SE
        1.0f, 1.0f,   1.0f, 1.0f,   // NE
        0.0f, 0.0f,   0.0f, 0.0f,   // SW
        1.0f, 1.0f,   1.0f, 1.0f,   // NE
        0.0f, 0.0f,   0.0f, 0.0f,   // SW
        0.0f, 1.0f,   0.0f, 1.0f    // NW
    }, 2u);*/


FluidRenderer::FluidRenderer(unsigned int width, unsigned int height) :
    screenWidth{width},
    screenHeight{height},
    frontCube{width, height},
    backCube{width, height},
    backgroundPlaneShader{".//shaders//background_plane.vert", ".//shaders//background_plane.frag"},
    raycastingPosShader(".//shaders//raycasting_pos.vert", ".//shaders//raycasting_pos.frag"),
    renderFluidShader(".//shaders//fluid.vert", ".//shaders//fluid.frag"),
    integrateFluidShader(".//shaders//integrate_fluid.vert", ".//shaders//integrate_fluid.frag"),//to delete
    advectLevelSetShader(".//shaders//advect_levelset.vert", ".//shaders//advect_levelset.frag"),// to delete
    advection(".//shaders//integrate_fluid.vert", ".//shaders//integrate_fluid.frag"), //  to rename file
    diffusion(".//shaders//diffuse_quantity.vert", ".//shaders//diffuse_quantity.frag"),
    forceApplication(".//shaders//apply_force_to_quantity.vert", ".//shaders//apply_force_to_quantity.frag"),
    passThrough(".//shaders//diffuse_quantity.vert", ".//shaders//pass_through.frag"),
    boundaryVelocity(".//shaders//diffuse_quantity.vert", ".//shaders//boundary_velocity.frag"),
    boundaryLS(".//shaders//diffuse_quantity.vert", ".//shaders//boundary_levelset.frag"),
    boundaryPressure(".//shaders//diffuse_quantity.vert", ".//shaders//boundary_pressure.frag"),
    pressurePoisson(".//shaders//diffuse_quantity.vert", ".//shaders//pressure_poisson.frag"),///////
    divergence(".//shaders//diffuse_quantity.vert", ".//shaders//divergence.frag"),
    removeDivergence(".//shaders//diffuse_quantity.vert", ".//shaders//remove_divergence.frag")
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
    velocity.uniformIntegrateShader = integrateFluidShader.getUniformLocation("velocityTexture");
    glUniform1i(velocity.uniformIntegrateShader, 0);
    levelSet.uniformIntegrateShader = integrateFluidShader.getUniformLocation("levelSetTexture");
    glUniform1i(levelSet.uniformIntegrateShader, 1);


    // TS/slice
    uniformSliceIF = integrateFluidShader.getUniformLocation("zSlice");
    uniformTimeStepIF = integrateFluidShader.getUniformLocation("timeStep");


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

    levelSet.uniformAdvectLSShader = advectLevelSetShader.getUniformLocation("velocityTexture");
    glUniform1i(levelSet.uniformAdvectLSShader, 0);
    //levelSet.uniformAdvectLSShader;
    
    levelSet.uniformIntegrateShader = advectLevelSetShader.getUniformLocation("levelSetTexture");
    glUniform1i(levelSet.uniformIntegrateShader, 1);
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

    // MOVE BELOW OUT OF FRAME - calculateCubeVectors() ?

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
    std::vector<float> tempSetData(4*gridSize*gridSize*gridSize, 0.0f);
    
    for (int i = 0; i < gridSize ; ++i){
        for (int j = 0; j < 4*gridSize*gridSize; j = j+4){
           tempSetData[4*i*gridSize*gridSize + j] = float(gridSize/2 - i); // Int division intentional
        }
    }

    // Improved level set data (0 around fluid) - still need to add border I suppose
    for (int k = 0; k < gridSize; ++k){
        for (int j = 0 ; j < gridSize; ++j){
            for (int i = 0; i < gridSize; ++i){
                //location of (i,j,k) in texture data is 4 * gridSize * gridSize * k + 4 * gridSize * j + 4 * i
                // bottom layer
                if ( j == 0){
                    tempSetData[4 * gridSize * gridSize * k + 4 * gridSize * j + 4 * i] = 0; // Top of water surface
                }

                // top 'half' decrements to 0 in j=16 layer
                else if (j >= 16){
                    tempSetData[4 * gridSize * gridSize * k + 4 * gridSize * j + 4 * i] = j - 16;
                }
            
                // bottom 'half': j in [1,15]
                else{
                    // Manhattan distance to edge
                    
                    int tempDist = std::min(i, gridSize-1-i);
                    tempDist = std::min(tempDist, std::min(k, gridSize-1-k));
                    tempDist = std::min(tempDist, j);
                    tempDist = std::min(tempDist, 16-j);

                    tempSetData[4 * gridSize * gridSize * k + 4 * gridSize * j + 4 * i] = -tempDist;


                    if (i == 0 || k == 0 || i == gridSize-1 || k == gridSize-1){//edges
                        tempSetData[4 * gridSize * gridSize * k + 4 * gridSize * j + 4 * i] = 0;
                    }
                }
            }
        }
    }
    
    // Yet further improved level set data (inc border, allowing for BCs to be used)

    for (int k = 0; k < gridSize; ++k){
        for (int j = 0 ; j < gridSize; ++j){
            for (int i = 0; i < gridSize; ++i){
                //location of (i,j,k) in texture data is 4 * gridSize * gridSize * k + 4 * gridSize * j + 4 * i
                // bottom layer is just outside fluid
                if ( j == 0){
                    tempSetData[4 * gridSize * gridSize * k + 4 * gridSize * j + 4 * i] = 1;
                }

                // top 'half' decrements to 1 in j=gridSize/2 + 1 layer
                else if (j > gridSize/2){
                    tempSetData[4 * gridSize * gridSize * k + 4 * gridSize * j + 4 * i] = j - gridSize/2;
                }
            
                // bottom 'half': j in [1,gridSize/2] 1s around outside, then 0s, then -ve
                else{
                    
                    if (i == 0 || k == 0 || i == gridSize-1 || k == gridSize-1){//edges
                        tempSetData[4 * gridSize * gridSize * k + 4 * gridSize * j + 4 * i] = 1;
                    }
                    else{// Manhattan distance to edge

                    int tempDist = std::min(i-1, gridSize-2-i);
                    tempDist = std::min(tempDist, std::min(k-1, gridSize-2-k));
                    tempDist = std::min(tempDist, j-1);
                    tempDist = std::min(tempDist, gridSize/2-j);

                    tempSetData[4 * gridSize * gridSize * k + 4 * gridSize * j + 4 * i] = -tempDist;


                    }
                }
            }
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
    /*
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    float tempLS[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, tempLS);*/
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, gridSize, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, tempSetData.data());
    glBindTexture(GL_TEXTURE_3D, 0);


    // Next LevelSet
    //std::vector<float> tempSetData2(4*gridSize*gridSize*gridSize, 0.0f);
    /*
    for (int i = 0; i < gridSize ; ++i){
        for (int j = 0; j < 4*gridSize*gridSize; j = j+4){
           tempSetData2[4*i*gridSize*gridSize + j] = float(i - gridSize/2); // Int division intentional

        }
    }*/

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
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, tempLS);*/
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, gridSize, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, tempSetData.data());
    glBindTexture(GL_TEXTURE_3D, 0);

    // Velocity  - initially parallel to (1,1,1)

    //float fluidVel = 1.0f;
    std::vector<float> tempVelocityData(4*gridSize*gridSize*gridSize, 0.0f); // Ignore a component

    for (int i = 0; i < gridSize ; ++i){
        for (int j = 0; j < 4*gridSize*gridSize; j = j+4){
           tempVelocityData[4*i*gridSize*gridSize + j+2/*z*/] = 0.0f;//-1e-7; // Int division intentional

        }
    }



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
    float tempVel[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glTexParameterfv(GL_TEXTURE_3D, GL_TEXTURE_BORDER_COLOR, tempVel);*/
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, gridSize, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, tempVelocityData.data());
    glBindTexture(GL_TEXTURE_3D, 0);

    
    // Pressure - initially zero 
    std::vector<float> tempPressureData(4*gridSize*gridSize*gridSize, 0.0f);
    

    /*
    for (int i = 0; i < gridSize ; ++i){
        for (int j = 0; j < gridSize*gridSize; ++j){
            // Each i is a z-slice?
            tempPressureData[i*gridSize*gridSize + j] = (i / gridSize) * gValue * rho;
        }
    }*/

    glGenTextures(1, &pressure.textureCurrent);
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_3D, pressure.textureCurrent);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, gridSize, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, tempPressureData.data());
    glBindTexture(GL_TEXTURE_3D, 0);


    glGenTextures(1, &pressure.textureNext);
    glActiveTexture(GL_TEXTURE0 + 4);
    glBindTexture(GL_TEXTURE_3D, pressure.textureNext);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, gridSize, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, tempPressureData.data());
    glBindTexture(GL_TEXTURE_3D, 0);

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


    // Temp velocity for use in Jacobi iteration
    glGenTextures(1, &textureVelocityTemp);
    glBindTexture(GL_TEXTURE_3D, textureVelocityTemp);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, gridSize, gridSize, gridSize, 0, GL_RGBA, GL_FLOAT, tempVelocityData.data());
    glBindTexture(GL_TEXTURE_3D, 0);  
};

void FluidRenderer::setUpSlices(){
    // Gen textures and bind to FBOs
    
    //glGenFramebuffers(1, &FBOPressureSlice);
    
}

void FluidRenderer::integrateFluid(unsigned int frameTime){
    

    glDisable(GL_BLEND);
    glViewport(0,0,gridSize, gridSize); // change to slab-op later 
    glEnable(GL_SCISSOR_TEST);
    

    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, levelSet.textureCurrent);

    // Velocity BC
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent); // bind advected quantity to pos = 2
    
    applyOuterSlabOperation(boundaryVelocity, velocity.textureNext);
    std::swap(velocity.textureCurrent, velocity.textureNext);

    // Advect Velocity
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, levelSet.textureCurrent);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);
    applyInnerSlabOperation(advection, frameTime, velocity.textureNext);

    //Level set BC

    glBindTexture(GL_TEXTURE_3D, levelSet.textureCurrent); // Bind to pos 2
    
    applyOuterSlabOperation(boundaryLS, levelSet.textureNext);
    std::swap(levelSet.textureCurrent, levelSet.textureNext);

     
    // Advect Level Set using old velocity (but with corrected BC)
    glBindTexture(GL_TEXTURE_3D, levelSet.textureCurrent);
    applyInnerSlabOperation(advection, frameTime, levelSet.textureNext);

    
    // Put advected velocity in current
    std::swap(velocity.textureCurrent, velocity.textureNext);

    // Re-bind velocity    
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);

    // Re-bind velocity as quantity to be altered in pos = 2
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);

    // pass through current velocity to temp velocity, which is used as 0th iteration
    applyInnerSlabOperation(passThrough, frameTime, textureVelocityTemp);
    // then bind temp velocity to pos = 2 
    //glBindTexture(GL_TEXTURE_3D, textureVelocityTemp);

    // Diffuse velocity
    for (int i = 0; i < numJacobiIterations; ++i){
        glBindTexture(GL_TEXTURE_3D, textureVelocityTemp);
        // Render into next velocity (kth iterate is in temp, k+1th in next)
        applyInnerSlabOperation(diffusion, frameTime, velocity.textureNext);
        // swap next and temp velocity, then iterate 
        std::swap(velocity.textureNext, textureVelocityTemp);
    }
        
    std::swap(velocity.textureCurrent, textureVelocityTemp); // Swap final iteration into current velocity

    // *** force is currently based on old level set - move swap to here and rebind LS to base it on new LS
    // Apply force to velocity
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent); // pos = 2
    applyInnerSlabOperation(forceApplication, frameTime, velocity.textureNext);
    std::swap(velocity.textureCurrent, velocity.textureNext);

    // *Remove divergence from velocity*

    // Compute div of currentVelocity (store in textureVelocityTemp)
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);
    applyInnerSlabOperation(divergence, frameTime, textureVelocityTemp);

    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, pressure.textureCurrent);

    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_3D, pressure.textureNext);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, textureVelocityTemp); // div(velocity)

    // Solve Poisson eqn 
    for (int i = 0; i < numJacobiIterationsPressure; ++i){
        // Pressure BC
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_3D, pressure.textureCurrent);

        //glActiveTexture(GL_TEXTURE0 + 1);
        //glBindTexture(GL_TEXTURE_3D, pressure.textureNext);
        
        applyOuterSlabOperation(boundaryPressure, pressure.textureNext);
        std::swap(pressure.textureCurrent, pressure.textureNext);

        // Iteration (kth iteration in current, k+1th in next)
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_3D, pressure.textureCurrent);
         glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_3D, textureVelocityTemp);

        //glActiveTexture(GL_TEXTURE0 + 1);
        //glBindTexture(GL_TEXTURE_3D, pressure.textureNext);
        // above re-binding could be avoided by relabelling in shader...
        applyInnerSlabOperation(pressurePoisson, frameTime, pressure.textureNext);

        std::swap(pressure.textureCurrent, pressure.textureNext);

    }

    // Subtract grad(pressure) from currentVelocity
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, pressure.textureCurrent);

    applyInnerSlabOperation(removeDivergence, frameTime, velocity.textureNext);
    std::swap(velocity.textureCurrent, velocity.textureNext);


    // Enforce no-slip (should this be before pressure subtraction?)

    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent);

    //glActiveTexture(GL_TEXTURE0 + 2);
    //glBindTexture(GL_TEXTURE_3D, velocity.textureCurrent); // bind advected quantity to pos = 2
    
    applyOuterSlabOperation(boundaryVelocity, velocity.textureNext);
    std::swap(velocity.textureCurrent, velocity.textureNext);

    // ////

    std::swap(levelSet.textureCurrent, levelSet.textureNext); // consider moving up to ***
    glDisable(GL_SCISSOR_TEST);
    // Tidy up
    glViewport(0,0,screenWidth,screenHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glActiveTexture(GL_TEXTURE0 + 0);
    
    glEnable(GL_BLEND);
}


