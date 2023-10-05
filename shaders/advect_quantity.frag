#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

uniform sampler3D velocityTexture;
uniform sampler3D quantityTexture; // Quantity to be advected

uniform float timeStep; // in microseconds
uniform float zSlice;

const int gridSize = 32;
const float step = 1.0f/gridSize;

vec3 lookUpCoords = vec3(TextureCoord, zSlice * step + 0.5f * step);

vec4 advectQuantity(){
    vec3 vel = texture(velocityTexture, lookUpCoords).xyz;
    return texture(quantityTexture, lookUpCoords - vel * timeStep);
}

void main(){
    //float sample = texture(quantityTexture, lookUpCoords).x;
    FragColor = advectQuantity();

    const float beta = 0.1f;
    float phiInf = lookUpCoords.y * gridSize - (0.5f * gridSize);
    if (phiInf < 0){
        FragColor.x *= (1.0f - beta);
        FragColor.x += beta * phiInf;
    }
}