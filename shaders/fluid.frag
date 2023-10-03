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
    return (( int(floor(coord.x / cellSize)) + int(floor(coord.y / cellSize)) ) % 2);
}

vec4 getFloorColor(vec3 floorPos){
    vec2 pos = vec2(floorPos.x, -floorPos.z);
    float spotLight = min(1.0f, 1.5 - 5.0f * length(pos));
    spotLight = max(0, spotLight);
    return spotLight * vec4((vec3(chessBoard(pos + vec2(0.5f, 0.5f), 0.02f)/2.0f + 0.2f) + vec3(0.0f, 0.07f, 0.0f)), 1.0f);// + /*vec4(0.5f, 0.5f, 0.5f, 1.0f)*/ skyColour * (1-spotLight) ;
}
////////////

// Maps a coord to the step below it
vec3 floorStep(vec3 x){

    return x - mod(x, 2 * step) + step; // step = 1/(2 * gS)
}

vec3 normalAtPoint(vec3 marchingPoint, float sample /*so not sampled redundantly*/){
    vec3 splineCoords = marchingPoint * gridSize; // in [0,1]

    // we need to map spline coords to between floor(marching point) and '' + 1

    vec4 ghX = texture(splineTexture, splineCoords.x);
    vec4 ghY = texture(splineTexture, splineCoords.y);
    vec4 ghZ = texture(splineTexture, splineCoords.z);
    const float dX = 2 * step;
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


    // Tricubic
    /* vec3 pt = floor(splineCoords);
    vec3 surfaceNormal = ghZ.x * (ghY.x * (ghX.x * texture(levelSetTexture, pt + vec3(ghX.z, ghY.z, ghZ.z)).xyz + 
                                     ghX.y * texture(levelSetTexture, pt + vec3(ghX.w, ghY.z, ghZ.z)).xyz) +
                             ghY.y * (ghX.x * texture(levelSetTexture, pt + vec3(ghX.z, ghY.w, ghZ.z)).xyz  +
                                      ghX.y * texture(levelSetTexture, pt + vec3(ghX.w, ghY.w, ghZ.z)).xyz)) +
                    ghZ.y * (ghY.x * (ghX.x * texture(levelSetTexture, pt + vec3(ghX.z, ghY.z, ghZ.w)).xyz +
                                      ghX.y * texture(levelSetTexture, pt + vec3(ghX.w, ghY.z, ghZ.w)).xyz) +
                             ghY.y * (ghX.x * texture(levelSetTexture, pt + vec3(ghX.z, ghY.w, ghZ.w)).xyz +
                                      ghX.y * texture(levelSetTexture, pt + vec3(ghX.w, ghY.w, ghZ.w)).xyz)); */


    return normalize(surfaceNormal);
}

vec4 rayColour(vec3 startPoint, vec3 dir){
    vec4 outColour;
    if (dir.y >= 0.0f){ // Ray pointing up
        outColour =  texture(skyBoxTexture, dir);
    }
    else{ // Ray pointing down
        float lambda = (-startPoint.y + 2 * step) * cubeScale;
        lambda /= dir.y;
        vec3 floorPos = (startPoint - vec3(0.5f, 0.5f, 0.5f)) * cubeScale + lambda * dir; 
        outColour = getFloorColor(floorPos / ( planeSize));
        // Blend bg plane with skybox
        outColour.xyz = outColour.w * outColour.xyz + (1 - outColour.w) * texture(skyBoxTexture, dir).xyz;
    }
    return outColour;
}

void main()
{
    // Consider using subtractive blending instead
    vec4 frontPos = texture(frontTexture, TextureCoord);
    vec4 backPos = texture(backTexture, TextureCoord);
    vec3 dir = (backPos - frontPos).xyz;
    
     // Draw zero planes - can use this technique to draw 'box' if desired
    /* if (frontPos.x < 0.01f && frontPos.x > 0.0f){
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
    } */
    // End zero planes

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

    vec3 lightColour = vec3(1.0f, 1.0f, 1.0f);
    float ambientStrength = 0.5f;
    vec3 lightDir = normalize(vec3(1.0f, 2.0f, 1.0f));

    vec3 diffuseColour = max(dot(surfaceNormal, lightDir), 0.0f) * lightColour * 2.0f;
    vec3 ambientColour = lightColour * ambientStrength;

    // FragColor = vec4(diffuseColour + ambientColour, 1.0f) * finalColour;
    
    // Reflection
    vec3 reflectDir = reflect(dir,surfaceNormal);
    vec4 reflectColour = rayColour(surfacePoint, reflectDir);
    reflectColour.w = float(reachedSurface); 


    // Refraction
    
    vec4 refractColour;
    vec3 tempRefractDir = refract(refractDir, -exitNormal, refIndex);
    tempRefractDir = normalize(tempRefractDir);

    if (false && exited && exitPoint.y <= 2 * step + 1e-4){ // Issue : handle frustrated TIR?
       // FragColor = vec4(1.0, 0.0f, 0.0f, 1.0f); return;
    }
    else{
        if (false){// Issue : handle TIR?
            refractDir = reflect(refractDir, -exitNormal);
        }
        else{
            refractDir = tempRefractDir;
        } 
        refractColour = rayColour(exitPoint, refractDir);
    }
   
    refractColour.w = float(reachedSurface);



    FragColor =  mix(vec4( diffuseColour + ambientColour, 1.0f )/* * finalColour */, mix(refractColour, reflectColour, 0.5), 1.0f);

    //FragColor.xyz = abs(surfaceNormal);
    FragColor.a = reachedSurface ? 1.0f : 0.0f;
}


// To do: multiple refraction