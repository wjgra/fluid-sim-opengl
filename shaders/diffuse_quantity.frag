#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D velocityTexture;
uniform sampler3D levelSetTexture;
uniform sampler3D quantityTexture;

uniform float timeStep; // in microseconds
uniform int zSlice;

const float step = 1.0f/gridSize;

const float viscosity = 0.1f;
const int numJacobiIterations = 50;

vec3 diffuseQuantity(){
    // Perform one jacobi iteration
    vec3 prevQuantity = texture(velocityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step)).xyz;
    vec3 tempQuantity;// = prevQuantity;

    float beta = viscosity * timeStep * step * step;

    float alpha = 1.0f/(1.0f + 6*beta);

    
    //for (int i = 0 ; i < numJacobiIterations ; ++i){

        vec3 quantityPosX = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(step, 0.0f, 0.0f)).xyz;
        vec3 quantityNegX = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(-step, 0.0f, 0.0f)).xyz;
        vec3 quantityPosY = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, step, 0.0f)).xyz;
        vec3 quantityNegY = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, -step, 0.0f)).xyz;
        vec3 quantityPosZ = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, 0.0f, step)).xyz;
        vec3 quantityNegZ = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step) + vec3(0.0f, 0.0f, -step)).xyz;

        tempQuantity = alpha * (prevQuantity + beta * (quantityNegX + quantityPosX + quantityNegY + quantityPosY + quantityNegZ + quantityPosZ));

    //}
    return tempQuantity;
}

void main(){
    levelSetTexture;
    FragColor = vec4(diffuseQuantity(), 0.0f);

}