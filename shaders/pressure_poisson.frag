#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

uniform sampler3D pressureTexture; // pressure
uniform sampler3D levelSetTexture; // level set
uniform sampler3D divergenceTexture; // div(velocity)

uniform float timeStep; // in microseconds
uniform float zSlice;

const int gridSize = 32;
const float step = 1.0f/gridSize;

vec3 lookUpCoords = vec3(TextureCoord, zSlice * step + 0.5f * step);

float solvePoisson(){

    float quantityPosX = texture(pressureTexture, lookUpCoords + vec3(step, 0.0f, 0.0f)).x;
    float quantityNegX = texture(pressureTexture, lookUpCoords + vec3(-step, 0.0f, 0.0f)).x;
    float quantityPosY = texture(pressureTexture, lookUpCoords + vec3(0.0f, step, 0.0f)).x;
    float quantityNegY = texture(pressureTexture, lookUpCoords + vec3(0.0f, -step, 0.0f)).x;
    float quantityPosZ = texture(pressureTexture, lookUpCoords + vec3(0.0f, 0.0f, step)).x;
    float quantityNegZ = texture(pressureTexture, lookUpCoords + vec3(0.0f, 0.0f, -step)).x;

    return (quantityNegX + quantityPosX + quantityNegY + quantityPosY + quantityNegZ + quantityPosZ - (step * step) * texture(divergenceTexture, lookUpCoords).x)/6.0f;
}

void main(){
    timeStep;
    if (texture(levelSetTexture, lookUpCoords ).x > 0)
    {
        FragColor = vec4(0.0f, 0.0f, 0.0f, 0.0f); // No pressure outside fluid
    }
    else{
        FragColor = vec4(solvePoisson(), 0.0f, 0.0f, 0.0f);
    }
    /* // Relaxation
    float beta = 0.6f;
    FragColor.x *= beta;
    FragColor.x += (1.0f - beta) * texture(pressureTexture, lookUpCoords).x; */
}