#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

const int gridSize = 32;

uniform sampler3D quantityTexture;

uniform int zSlice;

const float step = 1.0f/gridSize;

vec4 sampleQuantity(vec3 coordOffset){
    return texture(quantityTexture, coordOffset + vec3(TextureCoord, float(zSlice + 0.5f) * step));
}

void main(){
       //discard;
    //FragColor = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step)); return;
    //FragColor = vec4(-1.0f,0.0f, 0.0f, 0.0f);return;
    /*if (TextureCoord.x<step || TextureCoord.x>1-step || TextureCoord.y<step || TextureCoord.y>1-step){
        FragColor = vec4(1.0f,0.0f, 0.0f, 0.0f); // 1+inner
    }
    else{
        FragColor = texture(quantityTexture, vec3(TextureCoord, float(zSlice + 0.5f) * step));
    }
    */
    vec3 offset = vec3(0.0f, 0.0f, 0.0f);

    if (TextureCoord.x<step){
        offset = vec3(step, 0.0f, 0.0f);
        //FragColor = sampleQuantity(offset)+1;
    }
    else if (TextureCoord.x>1-step){
        offset = vec3(-step, 0.0f, 0.0f);
        //FragColor = sampleQuantity(offset)+1;
    }
    else if (TextureCoord.y<step){
        offset = vec3( 0.0f, step,0.0f);
        //FragColor = sampleQuantity(offset)+1;
    }
    else if (TextureCoord.y>1-step){
        offset = vec3(0.0f, -step,0.0f);
        //FragColor = sampleQuantity(offset)+1;
    }
    else if (zSlice == 0){
        offset = vec3(0.0f, 0.0f, step);
        //FragColor = sampleQuantity(offset)+1;
    }
    else if (zSlice == gridSize-1){
        offset = vec3(0.0f, 0.0f, -step);
        //FragColor = sampleQuantity(offset)+1;
    }
    else{
        FragColor = sampleQuantity(offset); return;
    }

    FragColor = sampleQuantity(offset);
    if (FragColor.x <= 0){
        FragColor.x *= -1.0f;
    }
    else{
        //FragColor.x += 1.0f;
    }


    //discard;
}