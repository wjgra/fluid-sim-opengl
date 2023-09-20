#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

uniform sampler3D quantityTexture;

uniform float timeStep; // in microseconds
uniform float zSlice;

const int gridSize = 32;
const float step = 1.0f/gridSize;

vec3 lookUpCoords = vec3(TextureCoord, zSlice * step + 0.5f * step);

const float viscosity = 1e-5;//1e-3; // Currently like honey! Needs to be lower for water

vec4 diffuseQuantity(){
    // Perform one jacobi iteration
    vec4 prevQuantity = texture(quantityTexture, lookUpCoords);

    vec4 quantityPosX = texture(quantityTexture, lookUpCoords + vec3(step, 0.0f, 0.0f));
    vec4 quantityNegX = texture(quantityTexture, lookUpCoords + vec3(-step, 0.0f, 0.0f));
    vec4 quantityPosY = texture(quantityTexture, lookUpCoords + vec3(0.0f, step, 0.0f));
    vec4 quantityNegY = texture(quantityTexture, lookUpCoords + vec3(0.0f, -step, 0.0f));
    vec4 quantityPosZ = texture(quantityTexture, lookUpCoords + vec3(0.0f, 0.0f, step));
    vec4 quantityNegZ = texture(quantityTexture, lookUpCoords + vec3(0.0f, 0.0f, -step));

    float beta = (viscosity * timeStep) * (step * step); // these are the same for all fragments - consider precalculating
    float alpha = 1.0f/(6.0f * beta + 1.0f);

    return alpha * prevQuantity + (alpha * beta) * (quantityNegX + quantityPosX + quantityNegY + quantityPosY + quantityNegZ + quantityPosZ);
}

void main(){
    FragColor = diffuseQuantity();
}