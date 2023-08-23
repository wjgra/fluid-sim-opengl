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

vec3 applyGravity(){
    float levelSetValue = texture(levelSetTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step)).x;
    vec3 vel = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step)).xyz;
    if (levelSetValue <= 0){
        //vel += vec3(-1e-13 * timeStep, 0.0f, 0.0f);
        vel += vec3(0.0f, -1e-13 * timeStep, 0.0f);
    }
    else{
        //vel = vec3(0.0f, 0.0f, 0.0f); // Not really the place for it...
    }
    if (TextureCoord.x < 0.3f){
        //vel += vec3(1e-13 * timeStep, 0.0f, 0.0f);
    }
    return vel;
}

void main(){
    velocityTexture;
    FragColor = vec4(applyGravity(), 0.0f);

}