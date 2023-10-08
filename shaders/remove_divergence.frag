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
    // Error O(h^2) grad approximation
    /* float quantityPosX = texture(pressureTexture, lookUpCoords + vec3(step, 0.0f, 0.0f)).x;
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
            0.0f) / (2 * step); */

    // Error O(h^4) grad approximation
    // This was an effort (largely in vain) to reduce the odd-even mode decoupling caused by
    // using central differences to solve N-S. Whilst the oscillation is still present, this
    // makes the solution slightly more stable on the boundary, so seems worthwhile to keep.
    float quantityPosX = texture(pressureTexture, lookUpCoords + vec3(step, 0.0f, 0.0f)).x;
    float quantityNegX = texture(pressureTexture, lookUpCoords + vec3(-step, 0.0f, 0.0f)).x;
    float quantityPosY = texture(pressureTexture, lookUpCoords + vec3(0.0f, step, 0.0f)).x;
    float quantityNegY = texture(pressureTexture, lookUpCoords + vec3(0.0f, -step, 0.0f)).x;
    float quantityPosZ = texture(pressureTexture, lookUpCoords + vec3(0.0f, 0.0f, step)).x;
    float quantityNegZ = texture(pressureTexture, lookUpCoords + vec3(0.0f, 0.0f, -step)).x;
    float quantityPosX2 = texture(pressureTexture, lookUpCoords + 2 * vec3(step, 0.0f, 0.0f)).x;
    float quantityNegX2 = texture(pressureTexture, lookUpCoords + 2 * vec3(-step, 0.0f, 0.0f)).x;
    float quantityPosY2 = texture(pressureTexture, lookUpCoords + 2 * vec3(0.0f, step, 0.0f)).x;
    float quantityNegY2 = texture(pressureTexture, lookUpCoords + 2 * vec3(0.0f, -step, 0.0f)).x;
    float quantityPosZ2 = texture(pressureTexture, lookUpCoords + 2 * vec3(0.0f, 0.0f, step)).x;
    float quantityNegZ2 = texture(pressureTexture, lookUpCoords + 2 * vec3(0.0f, 0.0f, -step)).x;

    FragColor = texture(velocityTexture, lookUpCoords)
        - vec4(
            8 * (quantityPosX - quantityNegX) - (quantityPosX2 - quantityNegX2),
            8 * (quantityPosY - quantityNegY) - (quantityPosY2 - quantityNegY2),
            8 * (quantityPosZ - quantityNegZ) - (quantityPosZ2 - quantityNegZ2),
            0.0f) / (12 * step);
}