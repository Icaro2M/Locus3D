#version 330 core

layout(location = 0) in vec2 a_Position;

uniform vec2 u_RectMin;
uniform vec2 u_RectMax;
uniform vec2 u_ViewportSize;

out vec2 v_PixelPosition;

void main()
{
    vec2 pixelPosition = mix(u_RectMin, u_RectMax, a_Position);
    v_PixelPosition = pixelPosition;

    vec2 ndc = vec2(
        (pixelPosition.x / u_ViewportSize.x) * 2.0 - 1.0,
        (pixelPosition.y / u_ViewportSize.y) * 2.0 - 1.0);

    gl_Position = vec4(ndc, 0.0, 1.0);
}
