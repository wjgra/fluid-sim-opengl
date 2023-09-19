#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D velocityTexture;
uniform sampler3D levelSetTexture;
uniform sampler3D quantityTexture; // vel

uniform float timeStep; // in microseconds
uniform float zSlice;

const float step = 1.0f/gridSize;

void main(){
    levelSetTexture;velocityTexture;timeStep;

    float quantityPosX = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(step, 0.0f, 0.0f)).x;
    float quantityNegX = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(-step, 0.0f, 0.0f)).x;
    float quantityPosY = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, step, 0.0f)).y;
    float quantityNegY = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, -step, 0.0f)).y;
    float quantityPosZ = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, 0.0f, step)).z;
    float quantityNegZ = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, 0.0f, -step)).z;

    FragColor = vec4((quantityPosX - quantityNegX + quantityPosY - quantityNegY + quantityPosZ - quantityNegZ)/(2 * step), 0.0f, 0.0f, 0.0f);

    //float temp = texture(quantityTexture, vec3(0.4f, 0.3f, 0.4f)).y;

    //if (quantityNegY  != 0.0f ){
      //  FragColor.x = 1e-6;
    //}
}
