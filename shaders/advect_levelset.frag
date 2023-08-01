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
    //return -1.0f;
    /*if (TextureCoord.x > 0.5){
        return -1.0f;
    }
    else{
        return 1.0f;
    }*/
    // coords are wrong!!!
    // We are looking up in an area clamped to the border texture
    // all coords for texture lookup are in range (0,1)
    // x:
    // y:
    // z:  (zSlice)/32
    float thisPoint = texture(levelSetTexture, vec3(TextureCoord, float(zSlice+0.5f)*step)).x;
    //thisPoint = texture(levelSetTexture, vec3(0.25, 1.05, 0.55)).x;
    /*
    if (thisPoint >= 0.0f){
        return 1.0f;
    }
    else{
        return -1.0f;
    }*/

    //return thisPoint;

    //int slice = 22;
    vec4 vel2 = texture(velocityTexture, vec3(TextureCoord, float(zSlice+0.5f)*step)) ;/// vec3(1.0f, 1.0f, step);
    /*
    if (vel2.x > 0.0f){
        return -1.0f;
    }
    else{
        return 1.0f;
    }*/
    vec3 vel = vel2.xyz;vel = vec3(0,1,0);
    vel /= -1e7;
    
    // Disappears for z>=0 - not anymore!
    // why does it still disappear with velocity == 0? Also, why does A component of frag color make a difference?
    //vel = vec3(1e10,0,0);

   // vel = vec3(0,0,-1.0f);
    //vel = vec3(0,0,-1e-7);
    //vel/= 10000000;
    /*if (vel.z > 0){
        return -1.0f;
    }
    else{
        return 1.0f;
    }*/
    //return float(texture(levelSetTexture, vec3(TextureCoord, zSlice)).x);
    return float(texture(levelSetTexture, vec3(TextureCoord, float(zSlice+0.5f)*step) - vel * timeStep).x);
}

void main(){
    
    //newLevelSet = -10.0f; return;
    float temp = advectLevelSet();
    FragColor = vec4(temp, 0.0f, 0.0f, 0.0f); return;

    /*
    if (temp < 0){
        FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f); 
    }
    else{
        FragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f); 
    }*/
    //newLevelSet = temp;//vec4( temp, 0.0f, 0.0f, 0.0f); // how to output as r32f? does output get clamped?
    // vec4(1.0f, 0.0f, 0.0f, 1.0f);
}