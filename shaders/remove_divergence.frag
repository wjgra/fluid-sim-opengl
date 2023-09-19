#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

uniform sampler3D velocityTexture; // velocity
uniform sampler3D pressureTexture; // poisson'd pressure

uniform float timeStep; // in microseconds
uniform float zSlice;

const int gridSize = 32;
const float step = 1.0f/gridSize;

vec3 lookUpCoords = vec3(TextureCoord, zSlice * step + 0.5f * step);

void main(){
    timeStep;
    
    float quantityPosX = texture(pressureTexture, lookUpCoords + vec3(step, 0.0f, 0.0f)).x;
    float quantityNegX = texture(pressureTexture, lookUpCoords + vec3(-step, 0.0f, 0.0f)).x;
    float quantityPosY = texture(pressureTexture, lookUpCoords + vec3(0.0f, step, 0.0f)).x;
    float quantityNegY = texture(pressureTexture, lookUpCoords + vec3(0.0f, -step, 0.0f)).x;
    float quantityPosZ = texture(pressureTexture, lookUpCoords + vec3(0.0f, 0.0f, step)).x;
    float quantityNegZ = texture(pressureTexture, lookUpCoords + vec3(0.0f, 0.0f, -step)).x;

    FragColor = texture(velocityTexture, lookUpCoords)
        - vec4(
            (quantityPosX - quantityNegX),
            (quantityPosY - quantityNegY),
            (quantityPosZ - quantityNegZ),
            0.0f) / (2 * step);
}