#version 330 core
out vec4 FragColor;

in vec2 pos;

//const float gridSize = 10.0f;

float chessBoard(vec2 coord, float cellSize){
    return (( int(floor(coord.x / cellSize)) + 
    int(floor(coord.y / cellSize))
    ) % 2);
}

void main()
{
    float spotLight = min(1.0f, 1.5 - 5.0f * length(pos));
    FragColor = spotLight * vec4((vec3(chessBoard(pos + vec2(0.5f, 0.5f), 0.02f)/2.0f + 0.2f) + vec3(0.0f, 0.07f, 0.0f)), 1);
}