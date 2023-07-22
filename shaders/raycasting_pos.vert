#version 330 core
layout (location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 cubePos;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    cubePos = position + vec3(0.5f, 0.5f, 0.5f);
}