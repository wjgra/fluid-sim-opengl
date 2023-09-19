#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D velocityTexture;
uniform sampler3D levelSetTexture;
uniform sampler3D quantityTexture;

uniform float timeStep; // in microseconds
uniform float zSlice;

const float step = 1.0f/gridSize;

void main(){
    levelSetTexture;velocityTexture;timeStep;
    FragColor = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step));

}