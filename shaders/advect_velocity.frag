#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

uniform sampler3D velocityTexture;
uniform sampler3D quantityTexture; // Quantity to be advected

uniform float timeStep; // in microseconds
uniform float zSlice;

const int gridSize = 32;
const float step = 1.0f/gridSize;

vec3 lookUpCoords = vec3(TextureCoord, zSlice * step + 0.5f * step);

vec4 advectQuantity(){
    vec3 vel = texture(velocityTexture, lookUpCoords).xyz;
    return texture(quantityTexture, lookUpCoords - vel * timeStep);
}

void main(){
    FragColor = advectQuantity();
}