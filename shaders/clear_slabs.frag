#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

uniform float timeStep; // in microseconds
uniform float zSlice;

void main(){
    timeStep;zSlice;
    FragColor = vec4 (0.0f, 0.0f, 0.0f, 0.0f);
}