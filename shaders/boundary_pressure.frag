#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D pressureTexture;

uniform float zSlice;

const float step = 1.0f/gridSize;

vec4 sampleQuantity(vec3 coordOffset){
    return texture(pressureTexture, coordOffset + vec3(TextureCoord, float(zSlice + 0.5f) * step));
}
void main(){
    //discard;
    //FragColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);return;
    //FragColor = texture(pressureTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step));


    vec3 offset = vec3(0.0f, 0.0f, 0.0f);

    if (TextureCoord.x<step){
        offset = vec3(step, 0.0f, 0.0f);
        FragColor = sampleQuantity(offset);
    }
    else if (TextureCoord.x>1-step){
        offset = vec3(-step, 0.0f, 0.0f);
        FragColor = sampleQuantity(offset);
    }
    else if (TextureCoord.y<step){
        offset = vec3( 0.0f, step,0.0f);
        FragColor = sampleQuantity(offset);
    }
    else if (TextureCoord.y>1-step){
        offset = vec3(0.0f, -step,0.0f);
        FragColor = sampleQuantity(offset);
    }
    else if (zSlice == 0){
        offset = vec3(0.0f, 0.0f, step);
        FragColor = sampleQuantity(offset);
    }
    else if (zSlice == gridSize-1){
        offset = vec3(0.0f, 0.0f, -step);
        FragColor = sampleQuantity(offset);
    }
    else{
        FragColor = sampleQuantity(offset);
    }

}