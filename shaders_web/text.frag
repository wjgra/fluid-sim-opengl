#version 300 es
precision highp float;
out vec4 FragColor;

in vec2 TextureCoord;

uniform sampler2D textTexture;

void main()
{
    FragColor = texture(textTexture, TextureCoord);
}