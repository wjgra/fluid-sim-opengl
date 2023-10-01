#version 330 core
out vec4 FragColor;

in vec2 TextureCoord;
//in vec3 FragPos;

uniform sampler2D frontTexture;
uniform sampler2D backTexture;

uniform sampler3D levelSetTexture;
uniform sampler1D splineTexture;
uniform samplerCube skyBoxTexture;

const int gridSize = 32;
const float planeSize = 10.0f;
const float cubeScale = 1.5f;
const vec4 sampleColour = vec4(0.227f, 0.621f, 0.777f, 0.8f) * vec4(1.0f, 1.0f, 1.0f, 1.5f/gridSize); // Vivid pale blue, with alpha factor

const vec4 skyColour = vec4(0.0f, 0.0f, 0.0f, 1.0f);

const float step = 1.0f/(gridSize * 2.0f); // Half voxel size

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
    return spotLight * vec4((vec3(chessBoard(pos + vec2(0.5f, 0.5f), 0.02f)/2.0f + 0.2f) + vec3(0.0f, 0.07f, 0.0f)), 1.0f);// + /*vec4(0.5f, 0.5f, 0.5f, 1.0f)*/ skyColour * (1-spotLight) ;

}
////////////


vec3 normalAtPoint(vec3 marchingPoint, float sample /*so not sampled redundantly*/){
    vec3 splineCoords = gridSize * marchingPoint; // Do we need to subtract 0.5?
    vec4 ghX = texture(splineTexture, splineCoords.x);
    vec4 ghY = texture(splineTexture, splineCoords.y);
    vec4 ghZ = texture(splineTexture, splineCoords.z);
    const float dX = step;
    vec3 e_x = vec3(dX, 0.0f, 0.0f);
    vec3 e_y = vec3(0.0f, dX, 0.0f);
    vec3 e_z = vec3(0.0f, 0.0f, dX);



    // 
    //const float dX = step;
    // Central differences
    float levelSetCentre = sample;
    float levelSetPosX = float(texture(levelSetTexture, marchingPoint + e_x).x);
    float levelSetNegX = float(texture(levelSetTexture, marchingPoint - e_x).x);
    float levelSetPosY = float(texture(levelSetTexture, marchingPoint + e_y).x);
    float levelSetNegY = float(texture(levelSetTexture, marchingPoint - e_y).x);
    float levelSetPosZ = float(texture(levelSetTexture, marchingPoint + e_z).x);
    float levelSetNegZ = float(texture(levelSetTexture, marchingPoint - e_z).x);

    vec3 surfaceNormal = vec3(levelSetPosX - levelSetNegX, levelSetPosY - levelSetNegY, levelSetPosZ - levelSetNegZ) / (2 * dX);
    return normalize(surfaceNormal);
}

// Maps a coord to the step below it
vec3 floorStep(vec3 x){
    return x - mod(x, step);
}

void main()
{
    // Consider using subtractive blending instead
    vec4 frontPos = texture(frontTexture, TextureCoord);
    vec4 backPos = texture(backTexture, TextureCoord);
    vec3 dir = (backPos - frontPos).xyz;
    
    /* // Draw zero planes - can use this technique to draw 'box' if desired
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
    
    if( frontPos.y < 0.505f && frontPos.y > 0.495f){ // Halfway line
        FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
        return;
    } */

    

    float len = length(dir);
    dir /= len;
    
    if (len< 0) discard;

    vec3 marchingPoint = frontPos.xyz ;

    vec4 finalColour = vec4(0.0f, 0.0f, 0.0f, 0.0f);
    

    bool reachedSurface = false;
    bool exited = false;
    vec3 surfaceNormal;
    vec3 exitNormal;

    vec3 exitPoint;
    vec3 surfacePoint;
    float marchingDistance;

    vec3 refractDir;

    vec3 tempDir = dir;

    const float refIndex = 1.33;

    for (marchingDistance = 0.0f ; marchingDistance < 2.0f * len ; marchingDistance += step, marchingPoint += step * tempDir){
        /* if (finalColour.w > 0.99f)
            break; */
        
        float sample = float(texture(levelSetTexture, marchingPoint).x);

        if (sample < 0){
            if (!reachedSurface){
                // FragColor = vec4(-1.0f/sample, 0.0f, 0.0f, 1.0f); return;
                reachedSurface  = true;
                // Hitpoint refinement
                const int numberOfRefinements = 6;
                for (int i = 1 ; i <= numberOfRefinements; ++i){
                    if (sample < 0){
                        marchingPoint -= pow(0.5f, i) * step * tempDir;
                    }
                    else{
                        marchingPoint += pow(0.5f, i) * step * tempDir;
                    }
                    sample = float(texture(levelSetTexture, marchingPoint).x);
                }
                // Normals
                surfacePoint = marchingPoint;
                surfaceNormal = normalAtPoint(marchingPoint, sample);
                refractDir = refract(dir,surfaceNormal, 1.0f/refIndex);
                refractDir = normalize(refractDir);
                tempDir = refractDir; 
            }
            //finalColour.xyz += sampleColour.xyz * sampleColour.w * (1.0f - finalColour.w);
            //finalColour.w += sampleColour.w * (1.0f - finalColour.w);
        }
        else{
            if (reachedSurface){
                exited = true;
                // Hitpoint refinement
                const int numberOfRefinements = 6;
                for (int i = 1 ; i <= numberOfRefinements; ++i){
                    if (sample > 0){
                        marchingPoint -= pow(0.5f, i) * step * tempDir;
                    }
                    else{
                        marchingPoint += pow(0.5f, i) * step * tempDir;
                    }
                    sample = float(texture(levelSetTexture, marchingPoint).x);
                }                
                exitPoint = marchingPoint;
                exitNormal = normalAtPoint(marchingPoint, sample);
                break;
            }
        }
        
        
    }

    // Issue: Add hitpoint refinement

    vec3 lightColour = vec3(1.0f, 1.0f, 1.0f);
    float ambientStrength = 0.5f;
    vec3 lightDir = normalize(vec3(1.0f, 2.0f, 1.0f));

    vec3 diffuseColour = max(dot(surfaceNormal, lightDir), 0.0f) * lightColour * 2.0f;
    vec3 ambientColour = lightColour * ambientStrength;

    // FragColor = vec4(diffuseColour + ambientColour, 1.0f) * finalColour;
    
    
    vec3 refDir = reflect(dir,surfaceNormal);
    
    vec4 reflectColour;
    if (refDir.y >= 0.0f){
        // Sky reflection
        reflectColour =  texture(skyBoxTexture, refDir);//skyColour;//vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else{
        // Plane reflection
        float lambda = - surfacePoint.y * cubeScale / refDir.y;
        vec3 floorPos = (surfacePoint - 0.5f) * cubeScale + lambda * refDir; 
        reflectColour = getFloorColor(floorPos / ( planeSize));// = vec4(0.0f, 0.0f, 1.0f, 1.0f);

        // Blend with bg
        reflectColour.xyz = reflectColour.w * reflectColour.xyz + (1 - reflectColour.w) * texture(skyBoxTexture, refDir).xyz;
    }
    reflectColour.w = float(reachedSurface);

    // REFRACTION
    //vec3 refractDir = refract(dir,surfaceNormal, 1/1.33); 
    vec3 tempRefractDir = refract(refractDir, -exitNormal, refIndex);
    tempRefractDir = normalize(tempRefractDir);
    //if (exited && refractDir.x == 0) {FragColor.r = 1.0f; return;};

    if (false && tempRefractDir == vec3(0.0f)){//TIR
        refractDir = reflect(refractDir, -exitNormal);
        //if (exited){FragColor.r = -(exitNormal.y); return;}
    }
    else{
        refractDir = tempRefractDir;
    }

    vec4 refractColour;
    if (refractDir.y >= 0.0f){
        //refractColour = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        refractColour = texture(skyBoxTexture, refractDir);
        //refractColour = vec4(1.0f, 0.0f, 0.0f, 0.0f);
    }
    else{
        // Plane reflection
        float lambda = - exitPoint.y * cubeScale / refractDir.y;
        vec3 floorPos = (exitPoint - 0.5f) * cubeScale + lambda * refractDir; 
        refractColour = getFloorColor(floorPos / ( planeSize));// = vec4(0.0f, 0.0f, 1.0f, 1.0f);

        refractColour.xyz = refractColour.w * refractColour.xyz + (1 - refractColour.w) * texture(skyBoxTexture, refractDir).xyz;
    }
    refractColour.w = float(reachedSurface);



    FragColor =  mix(vec4( diffuseColour + ambientColour, 1.0f )/* * finalColour */, mix(refractColour, reflectColour, 0.0f), 1.0f);

    //FragColor.xyz = abs(surfaceNormal);
    FragColor.a = reachedSurface ? 1.0f : 0.0f;

    //FragColor.r = refractDir.y ==0 ? 1.0f : 0.0f;

    // FragColor.xyz = abs(surfaceNormal);

    //float temp = texture(levelSetTexture, vec3(0.029f, 0.4f, 0.1f)).x;
    //FragColor = temp > 0 ? vec4(1.0f, 0.0f, 0.0f, 0.5f) : vec4(0.0f, 1.0f, 0.0f, 0.5f);
}


// To do: multiple refraction