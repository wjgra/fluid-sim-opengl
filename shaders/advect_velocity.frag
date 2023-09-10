#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D velocityTexture;
uniform sampler3D levelSetTexture; // treat as below - rename later
uniform sampler3D quantityTexture; // might be either of the above!

uniform float timeStep; // in microseconds
uniform int zSlice;

const float step = 1.0f/gridSize;

vec3 advectQuantity(){
    vec3 vel = texture(velocityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step)).xyz;
    return texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) - vel * timeStep).xyz;
}

void main(){
    //FragColor = vec4(0.0f, 0.0f, -1e-7, 0.0f); return;
    //vec3 temp = texture(levelSetTexture, vec3(TextureCoord, zSlice * step)).xyz;
    //temp = advectVelocity();
    levelSetTexture;
    FragColor = vec4(advectQuantity(), 0.0f);
    // FragColor = vec4(-1.0f, 0.0f, 0.0f, 0.0f);
    // vec4(1.0f, 0.0f, 0.0f, 1.0f);
}