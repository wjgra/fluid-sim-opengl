#include "../include/text_renderer.hpp"

TextRenderer::TextRenderer(unsigned int w, unsigned int h) : 
    m_shader(".//shaders//text.vert", ".//shaders//text.frag"),
    m_texture{".//res//lucida_typewriter_font_v2.tga"}
{   
    try{
        setUpBuffers();
        m_shader.useProgram();
        m_uniformModelTrans = m_shader.getUniformLocation("model");
        if (m_uniformModelTrans < 0)
            throw std::runtime_error("Failed to get get location of uniform \'model\'");

        m_uniformProjTrans = m_shader.getUniformLocation("projection");
        if (m_uniformProjTrans < 0)
            throw std::runtime_error("Failed to get get location of uniform \'projection\'");

        // Set orthogonal projection matrix
        glm::mat4 projection = glm::ortho(0.0f, (float)w,  (float)h, 0.0f, -1.0f, 1.0f);
        glUniformMatrix4fv(m_uniformProjTrans, 1, GL_FALSE, glm::value_ptr(projection));

        setPosition(100.0f, 320.0f, 240.0f);

        m_uniformTextureCoordOffset = m_shader.getUniformLocation("textureCoordOffset");
        if (m_uniformTextureCoordOffset < 0)
            throw std::runtime_error("Failed to get get location of uniform \'textureCoordOffset\'");
        m_successfullyInitialised = true;
    }
    catch (std::exception const& e){
        std::cerr << "[ERROR]: " << e.what() << "\n";
        m_successfullyInitialised = false;
    }
}

TextRenderer::~TextRenderer(){
    releaseBuffers();
}

void TextRenderer::setUpBuffers(){
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO); 

    // Copy vertex data into VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, m_quadVertexData.size()*sizeof(float), m_quadVertexData.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));

    // Copy element data into EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_quadElementData.size()*sizeof(GLuint), m_quadElementData.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void TextRenderer::releaseBuffers(){
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
}

// Limited to uppercase letters, numbers and common symbols
void TextRenderer::setChar(char toDraw) const{
    toDraw = toupper(toDraw);
    if (toDraw < ' ' || toDraw > '_'){
        glUniform2f(m_uniformTextureCoordOffset, 0.0f, 0.0f); // Draw space if out of range
    }
    else{
        int xOffset = (toDraw - ' ') % 8;
        int yOffset = (toDraw - ' ') / 8;
        glUniform2f(m_uniformTextureCoordOffset, xOffset*0.125f, yOffset*0.125f);
    }
}

bool TextRenderer::successfullyInitialised() const{
    return m_successfullyInitialised;
}

void TextRenderer::setPosition(float scale, float xPos, float yPos) const{
    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::scale(trans, glm::vec3(scale, scale, 1)); 
    trans = glm::translate(trans, glm::vec3(xPos, yPos, 0.0f));
    glUniformMatrix4fv(m_uniformModelTrans, 1, GL_FALSE, glm::value_ptr(trans));
}

void TextRenderer::drawString(std::string const& toDraw, float scale, float xPos, float yPos) const{
    m_texture.bind();
    m_shader.useProgram();
    glBindVertexArray(m_VAO);
    setPosition(scale, xPos, yPos);
    for (auto& ch : toDraw){
        setChar(ch);
        glDrawElements(GL_TRIANGLE_STRIP, m_quadElementData.size(), GL_UNSIGNED_INT, 0);
        xPos += 1;;
        setPosition(scale, xPos, yPos);
    }
    glBindVertexArray(0);
    m_texture.unbind();
}

void TextRenderer::drawStringCentred(std::string const& toDraw, float scale, float xPos, float yPos) const{
    drawString(toDraw, scale, xPos-0.5f*toDraw.length()*scale, yPos);
}