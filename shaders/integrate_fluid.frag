#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D velocityTexture;

uniform float timeStep; // in microseconds
uniform int zSlice;

const float step = 1.0f/gridSize;

vec3 advectVelocity(){
    vec3 vel = vec3(texture(velocityTexture, vec3(TextureCoord, zSlice * step)));
    return vec3(texture(velocityTexture, vec3(TextureCoord, zSlice * step) - vel * timeStep * step));
}

void main(){
    //temp = advectVelocity();
    FragColor = vec4(advectVelocity(), 0.0f);
    // vec4(1.0f, 0.0f, 0.0f, 1.0f);
}