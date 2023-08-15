#version 330 core
//out float newLevelSet;
out vec4 FragColor;
//out float temp;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D velocityTexture;
uniform sampler3D levelSetTexture;

uniform float timeStep ; // in microseconds
uniform int zSlice ; // need to ensure slices are at cell centres (with 0.5 offset in other calcs - make texcoord to/from texel coord functions)

const float step = 1.0f/float(gridSize);

float advectLevelSet(){

    float thisPoint = texture(levelSetTexture, vec3(TextureCoord, float(zSlice+0.5f)*step)).x;

    vec4 vel2 = texture(velocityTexture, vec3(TextureCoord, float(zSlice+0.5f)*step)) ;/// vec3(1.0f, 1.0f, step);
    
    vec3 vel = vel2.xyz;//vel = vec3(0,1,0);
    //vel /= -1e7;
    

    return float(texture(levelSetTexture, vec3(TextureCoord, float(zSlice+0.5f)*step) - vel * timeStep).x);
}

void main(){
    float temp = advectLevelSet();
    FragColor = vec4(temp, 0.0f, 0.0f, 0.0f); return;

}