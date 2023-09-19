#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D velocityTexture; // velocity
uniform sampler3D levelSetTexture;
uniform sampler3D quantityTexture; // poisson'd pressure

uniform float timeStep; // in microseconds
uniform float zSlice;

const float step = 1.0f/gridSize;

void main(){
    levelSetTexture;timeStep;
    
    float quantityPosX = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(step, 0.0f, 0.0f)).x;
    float quantityNegX = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(-step, 0.0f, 0.0f)).x;
    float quantityPosY = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, step, 0.0f)).x;
    float quantityNegY = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, -step, 0.0f)).x;
    float quantityPosZ = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, 0.0f, step)).x;
    float quantityNegZ = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, 0.0f, -step)).x;

    FragColor = texture(velocityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step))
        - vec4(
            (quantityPosX - quantityNegX)/(2 * step),
            (quantityPosY - quantityNegY)/(2 * step),
            (quantityPosZ - quantityNegZ)/(2 * step),
            0.0f);

    //FragColor = vec4(1e-7, 1e-7, 0.0f, 0.0f);
    // Check if non-zero solution to Poisson eqn
    if (quantityPosY != 0.0f || quantityNegY != 0.0f || quantityNegX != 0.0f || quantityPosX != 0.0f || quantityNegZ != 0.0f || quantityPosZ != 0.0f){
        //FragColor += vec4(0.0f, 1e-5, 0.0f, 0.0f);
    }
}