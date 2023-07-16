#version 300 es
in vec2 position;
in vec2 textureCoord;

uniform mat4 model;
uniform mat4 projection;

uniform vec2 textureCoordOffset;

out vec2 TextureCoord;

void main()
{
    gl_Position = projection * model * vec4(position, 0.0f, 1.0);
    TextureCoord = textureCoord + textureCoordOffset;
}