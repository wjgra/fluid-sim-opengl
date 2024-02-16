#include "drawable.hpp"

Drawable::Drawable(std::vector<float> const& verts, unsigned int vertexDimension) : m_vertices{verts}{
    try{
        setUpBuffers(vertexDimension);
        m_successfullyInitialised = true;
    }
    catch(std::exception const& e){
        std::cerr << "[ERROR]: " << e.what() << "\n";
        m_successfullyInitialised = false;
    }
}

Drawable::~Drawable(){
    releaseBuffers();
}

void Drawable::bindVAO() const{
    glBindVertexArray(m_VAO);
}

void Drawable::unbindVAO(){
    glBindVertexArray(0);
}

void Drawable::draw(GLint drawingMode) const{
    glDrawArrays(drawingMode, 0, m_vertices.size());
}

bool Drawable::successfullyInitialised() const{
    return m_successfullyInitialised;
}

void Drawable::setUpBuffers(unsigned int vertexDimension){
    if (vertexDimension < 2 || vertexDimension > 4){
        throw std::runtime_error("Failed to set up buffer object. Vertex dimension must be 2, 3 or 4.\n");
    }
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    bindVAO(); 

    // Bind VBO and copy vertex data into VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    unsigned int stride = vertexDimension + 2; // Length of vert and UV data    
    // Verts
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size()*sizeof(float), m_vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, vertexDimension, GL_FLOAT, GL_FALSE, stride * sizeof(float), static_cast<void*>(0));
    // UVs
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(vertexDimension*sizeof(float)));

    unbindVAO();
}

void Drawable::releaseBuffers(){
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
}