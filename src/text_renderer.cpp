#include "../include/text_renderer.hpp"

TextRenderer::TextRenderer() : 
    shader(".//shaders//text.vert", ".//shaders//text.frag"),
    texture{".//res//lucida_typewriter_font_v2.tga"}
{
    setUpBuffers();
    shader.useProgram();
    uniformModelTrans = shader.getUniformLocation("model");

    uniformProjTrans = shader.getUniformLocation("projection");

    // Set orthogonal projection matrix
    glm::mat4 projection = glm::ortho(0.0f, 640.0f,  480.0f, 0.0f, -1.0f, 1.0f); // needs updating for screenSize
    glUniformMatrix4fv(uniformProjTrans, 1, GL_FALSE, glm::value_ptr(projection));

    setPosition(100.0f, 320.0f, 240.0f);

    uniformTextureCoordOffset = shader.getUniformLocation("textureCoordOffset");
}

TextRenderer::~TextRenderer(){
    releaseBuffers();
}

void TextRenderer::setUpBuffers(){
    // Generate buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    // Bind VAO
    glBindVertexArray(VAO); 

    // Bind VBO and copy vertex data into VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));

    // Bind EBO and copy element data into EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size()*sizeof(GLuint), elements.data(), GL_STATIC_DRAW);

    // Unbind VAO
    glBindVertexArray(0);
}

void TextRenderer::releaseBuffers(){
    // Delete buffers
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

// Limited to uppercase letters, numbers and common symbols - trivial to extend to other ASCII characters
void TextRenderer::setChar(char toDraw){
    toDraw = toupper(toDraw);
    if (toDraw < ' ' || toDraw > '_'){
        glUniform2f(uniformTextureCoordOffset, 0.0f, 0.0f); // Draw space if out of range
    }
    else{
        int xOffset = (toDraw - ' ') % 8;
        int yOffset = (toDraw - ' ') / 8;
        glUniform2f(uniformTextureCoordOffset, xOffset*0.125f, yOffset*0.125f);
    }
}

void TextRenderer::setPosition(float scale, float xPos, float yPos){
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::scale(trans, glm::vec3(scale, scale, 1)); 
    trans = glm::translate(trans, glm::vec3(xPos, yPos, 0.0f));
    glUniformMatrix4fv(uniformModelTrans, 1, GL_FALSE, glm::value_ptr(trans));
}

void TextRenderer::drawString(std::string toDraw, float scale, float xPos, float yPos){
    glActiveTexture(GL_TEXTURE0 + 0);
    texture.bind();
    shader.useProgram();
    glBindVertexArray(VAO);
    setPosition(scale, xPos, yPos);
    for (char& ch : toDraw){
        setChar(ch);
        glDrawElements(drawingMode, elements.size(), GL_UNSIGNED_INT, 0);
        xPos += 1;//scale;
        setPosition(scale, xPos, yPos);
    }
    glBindVertexArray(0);
    texture.unbind();
}

void TextRenderer::drawStringCentred(std::string toDraw, float scale, float xPos, float yPos){
    drawString(toDraw, scale, xPos-0.5f*toDraw.length()*scale, yPos);
}