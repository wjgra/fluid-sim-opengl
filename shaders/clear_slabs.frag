#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D velocityTexture; // velocity
uniform sampler3D levelSetTexture;
uniform sampler3D quantityTexture; // poisson'd pressure

uniform float timeStep; // in microseconds
uniform int zSlice;

const float step = 1.0f/gridSize;

void main(){
    levelSetTexture;timeStep;velocityTexture; quantityTexture;zSlice;

    FragColor = vec4 (0.0f, 0.0f, 0.0f, 0.0f);
}