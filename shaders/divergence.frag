#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D velocityTexture;
uniform sampler3D levelSetTexture;
uniform sampler3D quantityTexture;

uniform float timeStep; // in microseconds
uniform int zSlice;

const float step = 1.0f/gridSize;

void main(){
    levelSetTexture;velocityTexture;timeStep;

    float quantityPosX = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(step, 0.0f, 0.0f)).x;
    float quantityNegX = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(-step, 0.0f, 0.0f)).x;
    float quantityPosY = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, step, 0.0f)).x;
    float quantityNegY = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, -step, 0.0f)).x;
    float quantityPosZ = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, 0.0f, step)).x;
    float quantityNegZ = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, 0.0f, -step)).x;

    FragColor = vec4((quantityPosX - quantityNegX + quantityPosY - quantityNegY + quantityPosZ - quantityNegZ)/(2 * step), 0.0f, 0.0f, 0.0f);
}