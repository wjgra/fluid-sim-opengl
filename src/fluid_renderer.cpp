#include "../include/fluid_renderer.hpp"

FluidRenderer::FluidRenderer(unsigned int width, unsigned int height) :
    fluidShader(".//shaders//fluid.vert", ".//shaders//fluid.frag"),
    screenWidth{width},
    screenHeight{height}    
{
    setUpBuffers();

    fluidShader.useProgram();
    uniformModelTrans = fluidShader.getUniformLocation("model");
    if (uniformModelTrans < 0)
       throw "Failed to get location of uniform \'model\'";

    //temp
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::translate(trans, glm::vec3(0, 0, 0.0f));
    trans = glm::rotate(trans, glm::radians(horizRot), glm::vec3(1.0f, 1.0f, 0.0f));
    trans = glm::scale(trans, glm::vec3(scale,scale, 1));
    glUniformMatrix4fv(uniformModelTrans, 1, GL_FALSE, glm::value_ptr(trans));

    uniformProjTrans = fluidShader.getUniformLocation("projection");
    if (uniformProjTrans < 0)
        throw "Failed to get location of uniform \'projection\'";

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width/(float)height, 0.1f, 100.0f);
    glUniformMatrix4fv(uniformProjTrans, 1, GL_FALSE, glm::value_ptr(projection));

    uniformViewTrans = fluidShader.getUniformLocation("view");
    if (uniformViewTrans < 0)
       throw "Failed to get location of uniform \'view\'";

    glm::mat4 view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    glUniformMatrix4fv(uniformViewTrans, 1, GL_FALSE, glm::value_ptr(view));

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
};

void FluidRenderer::releaseBuffers(){
    // Delete buffers
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
};

void FluidRenderer::draw(){
    glBindVertexArray(VAO);
    fluidShader.useProgram();
    //glDrawElements(drawingMode, elements.size(), GL_UNSIGNED_INT, 0);
    glDrawArrays(drawingMode, 0, 36);
    glBindVertexArray(0);
};

void FluidRenderer::frame(unsigned int frameTime){
    horizRot += frameTime * rotSpeed;
    // std::cout << horizRot <<"\n";
    fluidShader.useProgram();
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::translate(trans, glm::vec3(0, 0, 0.0f));
    trans = glm::rotate(trans, glm::radians(horizRot), glm::vec3(0.0f, 1.0f, 0.0f));
    trans = glm::scale(trans, glm::vec3(scale,scale, 1));
    glUniformMatrix4fv(uniformModelTrans, 1, GL_FALSE, glm::value_ptr(trans));
    draw();
};

void FluidRenderer::handleEvents(SDL_Event const& event){
    if (event.type == SDL_MOUSEBUTTONDOWN){}
    else if (event.type == SDL_MOUSEBUTTONUP){}
    else if(event.type == SDL_MOUSEMOTION){};
};