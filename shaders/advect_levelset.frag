#version 330 core
out vec4 FragColor;
//out float temp;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D levelSetTexture;
uniform sampler3D velocityTexture;

uniform float timeStep = 0.0f; // in microseconds
uniform int zSlice;

const float step = 1.0f/gridSize;

float advectLevelSet(){
    vec3 vel = vec3(texture(velocityTexture, vec3(TextureCoord, zSlice * step)));
    return float(texture(levelSetTexture, vec3(TextureCoord, zSlice * step) - vel * timeStep * step).x);
}

void main(){

    //newLevelSet = -1.0f;
    float temp = advectLevelSet();
    FragColor = vec4( temp, 0.0f, 0.0f, 0.0f); // how to output as r32f? does output get clamped?
    // vec4(1.0f, 0.0f, 0.0f, 1.0f);
}