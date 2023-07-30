#version 330 core
layout (location = 0) in vec2 position;
layout (location = 1 ) in vec2 textureCoord;

uniform mat4 model;
uniform mat4 projection;

out vec2 TextureCoord;
//out vec3 FragPos;


void main()
{
    gl_Position = projection * model * vec4(position, 0.0f, 1.0);
    TextureCoord = textureCoord;
}