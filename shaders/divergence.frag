#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

uniform sampler3D velocityTexture;

uniform float timeStep; // in microseconds
uniform float zSlice;

const int gridSize = 32;
const float step = 1.0f/gridSize;

vec3 lookUpCoords = vec3(TextureCoord, zSlice * step + 0.5f * step);

void main(){
   timeStep;

    float quantityPosX = texture(velocityTexture, lookUpCoords + vec3(step, 0.0f, 0.0f)).x;
    float quantityNegX = texture(velocityTexture, lookUpCoords + vec3(-step, 0.0f, 0.0f)).x;
    float quantityPosY = texture(velocityTexture, lookUpCoords + vec3(0.0f, step, 0.0f)).y;
    float quantityNegY = texture(velocityTexture, lookUpCoords + vec3(0.0f, -step, 0.0f)).y;
    float quantityPosZ = texture(velocityTexture, lookUpCoords + vec3(0.0f, 0.0f, step)).z;
    float quantityNegZ = texture(velocityTexture, lookUpCoords + vec3(0.0f, 0.0f, -step)).z;

    FragColor = vec4((quantityPosX - quantityNegX + quantityPosY - quantityNegY + quantityPosZ - quantityNegZ)/(2 * step), 0.0f, 0.0f, 0.0f);
}
