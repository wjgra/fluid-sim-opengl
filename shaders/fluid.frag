#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

uniform sampler2D frontTexture;
uniform sampler2D backTexture;

uniform sampler3D levelSetTexture;

void main()
{
    //FragColor = texture(frontTexture, TextureCoord);
    
    vec4 frontPos = texture(frontTexture, TextureCoord);
    vec4 backPos = texture(backTexture, TextureCoord);
    vec3 dir = (texture(backTexture, TextureCoord) - frontPos).xyz;

    //if (frontPos.a <= 0.0f)
       // discard;
    // Currently this reduces performance

    float len = length(dir);
    
    dir /= len;
    
    //FragColor = vec4(dir, 1.0f); return;

    vec3 marchingPoint = frontPos.xyz;

    int gridSize = 32;

    vec4 finalColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    float step = 1.0f/(gridSize * 2.0f); // Half voxel size
    for (float marchingDistance = 0.0f; marchingDistance < len ; marchingDistance += step, marchingPoint += step * dir){
        vec4 sampleColour = texture(levelSetTexture, marchingPoint) * vec4(1.0f, 1.0f, 1.0f, 3.0f/gridSize);
        
        finalColour.xyz += sampleColour.xyz * sampleColour.w * (1.0f - finalColour.w);
        finalColour.w += sampleColour.w * (1.0f - finalColour.w);
        if (finalColour.w > 0.99)
            break;

        /*if (frontPos.z < 0.5f && backPos.z > 0.5f){
            // finalColour = vec4(1.0f, 0.0f, 0.0f, 1.0f);
            if (true ){
                //finalColour = vec4(1.0f, 0.0f, 0.0f, 1.0f);
                finalColour = vec4(dir, 1.0f);
                break;
            }
        }*/
        
    }

    //if (len < 0.1f)
        //finalColour = vec4(1.0f, 0.0f, 0.0f, 1.0f);

    FragColor = finalColour;//vec4(len, len, len, 1.0f) * samp;



}