#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

uniform sampler3D velocityTexture;
uniform sampler3D levelSetTexture;

uniform float timeStep; // in microseconds
uniform float zSlice;

const int gridSize = 32;
const float step = 1.0f/gridSize;

vec3 lookUpCoords = vec3(TextureCoord, zSlice * step + 0.5f * step); // consider calculating in vertex shader

uniform vec3 gravityDirection = vec3(0.0, -1.0f, 0.0f);
const float gravityStrength = 4e-13;

vec4 applyGravity(){
    vec4 vel = texture(velocityTexture, lookUpCoords);
    if (texture(levelSetTexture, lookUpCoords).x <= 0.0f){
        vel += (gravityStrength * timeStep) * vec4(gravityDirection, 0.0f);
    }
    return vel;
}

void main(){
    FragColor = applyGravity();
}