#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D velocityTexture;

uniform float timeStep; // in microseconds
uniform int zSlice;

const float step = 1.0f/gridSize;

vec4 advectVelocity(){
    vec3 vel = texture(velocityTexture, vec3(TextureCoord, zSlice * step)).xyz;
    return texture(velocityTexture, vec3(TextureCoord, zSlice * step) - vel * timeStep * step);
}

void main(){
    //temp = advectVelocity();
    FragColor =advectVelocity();
    // vec4(1.0f, 0.0f, 0.0f, 1.0f);
}