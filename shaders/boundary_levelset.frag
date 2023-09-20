#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D levelSetTexture;

uniform float zSlice;

const float step = 1.0f/gridSize;

vec4 sampleLS(vec3 coordOffset){
    return texture(levelSetTexture, coordOffset + vec3(TextureCoord, float(zSlice + 0.5f) * step));
}

void main(){

    vec3 offset = vec3(0.0f, 0.0f, 0.0f);

    if (TextureCoord.x<step){
        offset = vec3(step, 0.0f, 0.0f);
    }
    else if (TextureCoord.x>1-step){
        offset = vec3(-step, 0.0f, 0.0f);
    }
    else if (TextureCoord.y<step){
        offset = vec3( 0.0f, step,0.0f);
    }
    else if (TextureCoord.y>1-step){
        offset = vec3(0.0f, -step,0.0f);
    }
    else if (zSlice == 0){
        offset = vec3(0.0f, 0.0f, step);
    }
    else if (zSlice == gridSize-1){
        offset = vec3(0.0f, 0.0f, -step);
    }
    else{
        FragColor = sampleLS(offset); return;
    }

    FragColor = sampleLS(offset);
    if (FragColor.x <= 0){
        FragColor.x *= -1.0f;
    }
    else{
        //FragColor.x += 1.0f;
    }


    //discard;
}