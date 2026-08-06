#version 330 core

in vec2 v_PixelPosition;

uniform vec2 u_RectMin;
uniform vec2 u_RectMax;
uniform float u_BorderThicknessPixels;
uniform vec4 u_FillColor;
uniform vec4 u_BorderColor;

out vec4 FragColor;

void main()
{
    vec2 distanceToEdge = min(
        v_PixelPosition - u_RectMin,
        u_RectMax - v_PixelPosition);

    bool border = distanceToEdge.x <= u_BorderThicknessPixels ||
        distanceToEdge.y <= u_BorderThicknessPixels;

    FragColor = border ? u_BorderColor : u_FillColor;
}
