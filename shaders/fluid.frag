#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;
//in vec3 FragPos;

uniform sampler2D frontTexture;
uniform sampler2D backTexture;

uniform sampler3D levelSetTexture;

const int gridSize = 32;
const float planeSize = 10.0f;
const float cubeScale = 1.5f;
const vec4 sampleColour = vec4(0.227f, 0.621f, 0.777f, 0.8f) * vec4(1.0f, 1.0f, 1.0f, 1.5f/gridSize); // Vivid pale blue, with alpha factor

const vec4 skyColour = vec4(0.0f, 0.0f, 0.0f, 1.0f);

////////////////
//floor colour functions
float chessBoard(vec2 coord, float cellSize){
    return (( int(floor(coord.x / cellSize)) + 
    int(floor(coord.y / cellSize))
    ) % 2);
}

vec4 getFloorColor(vec3 floorPos){
    vec2 pos = vec2(floorPos.z, -floorPos.x);
    float spotLight = min(1.0f, 1.5 - 5.0f * length(pos));
    return spotLight * vec4((vec3(chessBoard(pos + vec2(0.5f, 0.5f), 0.02f)/2.0f + 0.2f) + vec3(0.0f, 0.07f, 0.0f)), 1.0f) + /*vec4(0.5f, 0.5f, 0.5f, 1.0f)*/ skyColour * (1-spotLight) ;

}
////////////

void main()
{
    //FragColor = texture(frontTexture, TextureCoord);

    vec4 frontPos = texture(frontTexture, TextureCoord);
    vec4 backPos = texture(backTexture, TextureCoord);
    vec3 dir = (texture(backTexture, TextureCoord) - frontPos).xyz;

    //if (frontPos.a <= 0.0f)
       //discard;
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
    
    vec3 marchingPoint = frontPos.xyz ;


    vec4 finalColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    float step = 1.0f/(gridSize * 2.0f); // Half voxel size

    //marchingPoint += dir * step;

    bool reachedSurface = false;
    vec3 surfaceNormal;
    float dX = step;

    vec3 surfacePoint;

    float marchingDistance;

    for (marchingDistance = 0.0f ; marchingDistance < len ; marchingDistance += step, marchingPoint += step * dir){
        if (finalColour.w > 0.99f)
            break;
        
        float sample = float(texture(levelSetTexture, marchingPoint).x);

        if (sample < 0.0f){
            
            if (!reachedSurface){
                //FragColor = vec4(-sample / 1.0f, 0.0f, 0.0f, 1.0f); return;
                reachedSurface  = true;
                surfacePoint = marchingPoint;
                if (false && marchingPoint.z >= 1.0f-step){
                    surfaceNormal = vec3(0.0f, 0.0f, 1.0f);
                }
                else
                {
                    // Central differences
                    float levelSetCentre = sample;
                    float levelSetPosX = float(texture(levelSetTexture, marchingPoint + vec3(dX, 0.0f, 0.0f)).x);
                    float levelSetNegX = float(texture(levelSetTexture, marchingPoint + vec3(-dX, 0.0f, 0.0f)).x);
                    float levelSetPosY = float(texture(levelSetTexture, marchingPoint + vec3(0.0f, dX, 0.0f)).x);
                    float levelSetNegY = float(texture(levelSetTexture, marchingPoint + vec3(0.0f, -dX, 0.0f)).x);
                    float levelSetPosZ = float(texture(levelSetTexture, marchingPoint + vec3(0.0f, 0.0f, dX)).x);
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

    // Go back half a step to refine exit point - consider doing this for all entry/exit points
    marchingDistance -= 0.5f * step; marchingPoint -= 0.5f * step * dir;
    float sample = float(texture(levelSetTexture, marchingPoint).x);
    if (sample < 0.0f){
        reachedSurface = true;
        finalColour.xyz += 0.5f * sampleColour.xyz * sampleColour.w * (1.0f - finalColour.w);
        finalColour.w += 0.5f * sampleColour.w * (1.0f - finalColour.w);

        // Central differences
        float levelSetCentre = sample;
        float levelSetPosX = float(texture(levelSetTexture, marchingPoint + vec3(dX, 0.0f, 0.0f)).x);
        float levelSetNegX = float(texture(levelSetTexture, marchingPoint + vec3(-dX, 0.0f, 0.0f)).x);
        float levelSetPosY = float(texture(levelSetTexture, marchingPoint + vec3(0.0f, dX, 0.0f)).x);
        float levelSetNegY = float(texture(levelSetTexture, marchingPoint + vec3(0.0f, -dX, 0.0f)).x);
        float levelSetPosZ = float(texture(levelSetTexture, marchingPoint + vec3(0.0f, 0.0f, dX)).x);
        float levelSetNegZ = float(texture(levelSetTexture, marchingPoint + vec3(0.0f, 0.0f, -dX)));

        surfaceNormal = vec3(levelSetPosX - levelSetNegX, levelSetPosY - levelSetNegY, levelSetPosZ - levelSetNegZ) / (2 * dX);
        surfaceNormal = normalize(surfaceNormal);
    }


    if (reachedSurface){
        //finalColour.w += 1.0f;//0.1f;
    }

    // Normals need to be transformed... ?? or do they??
    vec3 lightColour = vec3(1.0f, 1.0f, 1.0f);
    float ambientStrength = 0.5f;
    vec3 lightDir = normalize(vec3(1.0f, 2.0f, 1.0f));

    vec3 diffuseColour = max(dot(surfaceNormal, lightDir), 0.0f) * lightColour * 2.0f;
    vec3 ambientColour = lightColour * ambientStrength;

    vec3 refDir = reflect(dir,surfaceNormal);
    //float lambda = 1.5f * gridSize * (surfacePoint.y*10.0f - 0.5f)/refDir.y;

    

    //vec3 floorPos = 1.5f * gridSize * surfacePoint*10.0f + lambda * refDir;
    
    vec4 reflectColour;// = getFloorColor(floorPos);
    if (refDir.y >= 0.0f){//} || dot(vec3(dir.x, 0.0f, dir.z), refDir) >= 0.0f){
        // Sky reflection
        reflectColour = skyColour;//vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else{
        // Plane reflection
        float lambda = - surfacePoint.y * cubeScale / refDir.y;
        vec3 floorPos = (surfacePoint - 0.5f) * cubeScale + lambda * refDir; 

        reflectColour = getFloorColor(floorPos / ( planeSize));// = vec4(0.0f, 0.0f, 1.0f, 1.0f);
    }
    reflectColour.w = float(reachedSurface);

    FragColor =  mix(vec4( diffuseColour + ambientColour, 1.0f )* finalColour, reflectColour, 0.2f);

    //FragColor = vec4 ( abs(surfaceNormal), 1.0f);
}
