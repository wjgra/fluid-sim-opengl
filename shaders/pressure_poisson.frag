#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D velocityTexture; // pressure
uniform sampler3D levelSetTexture; // level set
uniform sampler3D quantityTexture; // div(velocity)

uniform float timeStep; // in microseconds
uniform float zSlice;

const float step = 1.0f/gridSize;

float solvePoisson(){

    float quantityPosX = texture(velocityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(step, 0.0f, 0.0f)).x;
    float quantityNegX = texture(velocityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(-step, 0.0f, 0.0f)).x;
    float quantityPosY = texture(velocityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, step, 0.0f)).x;
    float quantityNegY = texture(velocityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, -step, 0.0f)).x;
    float quantityPosZ = texture(velocityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, 0.0f, step)).x;
    float quantityNegZ = texture(velocityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, 0.0f, -step)).x;

    return (quantityNegX + quantityPosX + quantityNegY + quantityPosY + quantityNegZ + quantityPosZ - step * step * texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step)))/6.0f;
}


void main(){
    timeStep;
    if (texture(levelSetTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) ).x > 0)
    {
        FragColor = vec4(0.0f, 0.0f, 0.0f, 0.0f); // No pressure outside fluid
    }
    else{
        FragColor = vec4(solvePoisson(), 0.0f, 0.0f, 0.0f);
        //if (texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step)).x == 0.0f){



            //FragColor = vec4(1e-8, 0.0f, 0.0f, 0.0f);
            //FragColor = vec4((0.5-TextureCoord.y) * 1e-9, 0.0f, 0.0f, 0.0f);
        //}  
    }

    //if (texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step)).x != 0.0f) FragColor = vec4(1e-8, 0.0f, 0.0f, 0.0f);
}