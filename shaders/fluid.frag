#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;

uniform sampler2D frontTexture;
uniform sampler2D backTexture;

uniform sampler3D levelSetTexture;
//uniform sampler3D velocityTexture;
//uniform sampler3D pressureTexture;

//uniform sampler3D nextLevelSetTexture;
//uniform sampler3D nextVelocityTexture;
//uniform sampler3D nextPressureTexture;

const int gridSize = 32;
const vec4 sampleColour = vec4(0.227f, 0.621f, 0.777f, 0.4f) * vec4(1.0f, 1.0f, 1.0f, 1.5f/gridSize); // Vivid pale blue, with alpha factor

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
    

    vec3 marchingPoint = frontPos.xyz;


    vec4 finalColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    float step = 1.0f/(gridSize * 2.0f); // Half voxel size

    for (float marchingDistance = 0.0f; marchingDistance < len ; marchingDistance += step, marchingPoint += step * dir){
        if (finalColour.w > 0.99)
            break;
        
        float sample = float(texture(levelSetTexture, marchingPoint));

        if (sample <= 0.0f){
            finalColour.xyz += sampleColour.xyz * sampleColour.w * (1.0f - finalColour.w);
            finalColour.w += sampleColour.w * (1.0f - finalColour.w);
        }
        else{
            continue;
        }
        
        
    }

    FragColor = finalColour;



}