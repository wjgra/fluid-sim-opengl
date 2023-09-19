#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

uniform sampler3D quantityTexture;

uniform float timeStep; // in microseconds
uniform float zSlice;

const int gridSize = 32;
const float step = 1.0f/gridSize;

void main(){
    timeStep;
    FragColor = texture(quantityTexture, vec3(TextureCoord, zSlice * step + 0.5f * step));

}