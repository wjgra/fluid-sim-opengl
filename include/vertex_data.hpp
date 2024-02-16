#ifndef _FLUID_VERTEX_DATA_HPP_
#define _FLUID_VERTEX_DATA_HPP_

#include "drawable.hpp"

float constexpr inline cubeVerts[] = {
    // Back face
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // SW
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // NE
    0.5f, -0.5f, -0.5f,  1.0f, 0.0f, // SE         
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // NE
    -0.5f, -0.5f, -0.5f,  0.0f, 0.0f, // SW
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // NW
    // Front face
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // SW
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // SE
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // NE
    0.5f,  0.5f,  0.5f,  1.0f, 1.0f, // NE
    -0.5f,  0.5f,  0.5f,  0.0f, 1.0f, // NW
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // SW
    // Left face
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // NE
    -0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // NW
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // SW
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // SW
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // SE
    -0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // NE
    // Right face
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // NW
    0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // SE
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // NE         
    0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // SE
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // NW
    0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // SW     
    // Bottom face
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // NE
    0.5f, -0.5f, -0.5f,  1.0f, 1.0f, // NW
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // SW
    0.5f, -0.5f,  0.5f,  1.0f, 0.0f, // SW
    -0.5f, -0.5f,  0.5f,  0.0f, 0.0f, // SE
    -0.5f, -0.5f, -0.5f,  0.0f, 1.0f, // NE
    // Top face
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // NW
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // SE
    0.5f,  0.5f, -0.5f,  1.0f, 1.0f, // NE     
    0.5f,  0.5f,  0.5f,  1.0f, 0.0f, // SE
    -0.5f,  0.5f, -0.5f,  0.0f, 1.0f, // NW
    -0.5f,  0.5f,  0.5f,  0.0f, 0.0f  // SW  
};
unsigned int constexpr inline cubeVertsSize = std::size(cubeVerts);

float constexpr inline quadVerts[] = {
    1.0f, 0.0f,   1.0f, 0.0f,   // SE
    1.0f, 1.0f,   1.0f, 1.0f,   // NE
    0.0f, 0.0f,   0.0f, 0.0f,   // SW
    1.0f, 1.0f,   1.0f, 1.0f,   // NE
    0.0f, 0.0f,   0.0f, 0.0f,   // SW
    0.0f, 1.0f,   0.0f, 1.0f    // NW
};
unsigned int constexpr inline quadVertsSize = std::size(quadVerts);

float constexpr inline backgroundPlaneVerts[] = {
    -0.5f, -0.5f,      0.0f, 0.0f,
    0.5f,  -0.5f,     1.0f, 0.0f,
    -0.5f,  0.5f,      0.0f, 1.0f,
    -0.5f, 0.5f,     0.0f, 1.0f,
    0.5f, -0.5f,      1.0f, 0.0f,
    0.5f, 0.5,     1.0f, 1.0f
};
unsigned int constexpr inline backgroundPlaneVertsSize = std::size(backgroundPlaneVerts);

#endif