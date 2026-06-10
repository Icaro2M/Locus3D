#version 450 core

uniform vec4 u_PickingColor;

out vec4 FragColor;

void main()
{
    FragColor = u_PickingColor;
}