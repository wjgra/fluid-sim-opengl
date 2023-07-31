#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;
//in vec3 FragPos;

uniform sampler2D frontTexture;
uniform sampler2D backTexture;

uniform sampler3D levelSetTexture;

const int gridSize = 32;
const vec4 sampleColour = vec4(0.227f, 0.621f, 0.777f, 0.8f) * vec4(1.0f, 1.0f, 1.0f, 1.5f/gridSize); // Vivid pale blue, with alpha factor

void main()
{
    //FragColor = texture(frontTexture, TextureCoord);

    vec4 frontPos = texture(frontTexture, TextureCoord);
    vec4 backPos = texture(backTexture, TextureCoord);
    vec3 dir = (texture(backTexture, TextureCoord) - frontPos).xyz;

    //if (frontPos.a <= 0.0f)
       // discard;
    // Currently this reduces performance

    
    // Draw zero planes - can use this technique to draw 'box' if desired
    if (frontPos.x < 0.01f && frontPos.x > 0.0f){
        FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
        return;
    }
    else if( frontPos.y < 0.01f && frontPos.y > 0.0f){
        FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
        return;
    }
    else if( frontPos.z < 0.01f && frontPos.z > 0.0f){
        FragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
        return;
    }
    // End zero planes
    

    float len = length(dir);
    dir /= len;
    
    vec3 marchingPoint = frontPos.xyz;


    vec4 finalColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    float step = 1.0f/(gridSize * 2.0f); // Half voxel size

    bool reachedSurface = false;
    vec3 surfaceNormal;
    float dX = 2 * step;

    for (float marchingDistance = 0.0f; marchingDistance < len ; marchingDistance += step, marchingPoint += step * dir){
        if (finalColour.w > 0.99)
            break;
        
        float sample = float(texture(levelSetTexture, marchingPoint));

        if (sample < 0.0f){
            
            if (!reachedSurface){ // Consider separate loops
                reachedSurface  = true;
                if (false && marchingPoint.x <= dX){
                    //
                }
                else
                {
                    // Central differences
                    float levelSetCentre = sample;
                    float levelSetPosX = float(texture(levelSetTexture, marchingPoint + vec3(dX, 0.0f, 0.0f)));
                    float levelSetNegX = float(texture(levelSetTexture, marchingPoint + vec3(-dX, 0.0f, 0.0f)));
                    float levelSetPosY = float(texture(levelSetTexture, marchingPoint + vec3(0.0f, dX, 0.0f)));
                    float levelSetNegY = float(texture(levelSetTexture, marchingPoint + vec3(0.0f, -dX, 0.0f)));
                    float levelSetPosZ = float(texture(levelSetTexture, marchingPoint + vec3(0.0f, 0.0f, dX)));
                    float levelSetNegZ = float(texture(levelSetTexture, marchingPoint + vec3(0.0f, 0.0f, -dX)));

                    surfaceNormal = vec3(levelSetPosX - levelSetNegX, levelSetPosY - levelSetNegY, levelSetPosZ - levelSetNegZ) / (2 * dX);
                    surfaceNormal = normalize(surfaceNormal);
                }

            }
            finalColour.xyz += sampleColour.xyz * sampleColour.w * (1.0f - finalColour.w);
            finalColour.w += sampleColour.w * (1.0f - finalColour.w);
        }
        else{
            continue;
        }
        
        
    }
    // Normals need to be transformed...
    vec3 lightColour = vec3(1.0f, 1.0f, 1.0f);
    float ambientStrength = 0.8f;
    vec3 lightDir = normalize(vec3(0.0f, -1.0f, 1.0f));

    vec3 diffuseColour = max(dot(surfaceNormal, lightDir), 0.0f) * lightColour;
    vec3 ambientColour = lightColour * ambientStrength;

    FragColor = vec4(/*diffuseColour + */ambientColour, 1.0f) * finalColour;



}