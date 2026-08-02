#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in float a_RadiusPixels;
layout(location = 2) in vec4 a_FillColor;
layout(location = 3) in vec4 a_BorderColor;
layout(location = 4) in float a_BorderWidthPixels;

uniform mat4 u_ViewProjection;
uniform vec2 u_ViewportSize;

out vec2 v_Local;
out float v_RadiusPixels;
out float v_BorderWidthPixels;
out vec4 v_FillColor;
out vec4 v_BorderColor;

void main()
{
    vec4 centerClip = u_ViewProjection * vec4(a_Position, 1.0);

    if (centerClip.w <= 0.0001 ||
        u_ViewportSize.x <= 0.0 ||
        u_ViewportSize.y <= 0.0 ||
        a_RadiusPixels <= 0.0) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        v_Local = vec2(0.0);
        v_RadiusPixels = 0.0;
        v_BorderWidthPixels = 0.0;
        v_FillColor = vec4(0.0);
        v_BorderColor = vec4(0.0);
        return;
    }

    vec2 corner = vec2(
        (gl_VertexID == 0 || gl_VertexID == 2) ? -1.0 : 1.0,
        (gl_VertexID < 2) ? -1.0 : 1.0);
    vec2 pixelToNdc = vec2(2.0 / u_ViewportSize.x, 2.0 / u_ViewportSize.y);
    vec2 offsetNdc = corner * a_RadiusPixels * pixelToNdc;

    vec4 clip = centerClip;
    clip.xy += offsetNdc * centerClip.w;

    gl_Position = clip;
    v_Local = corner;
    v_RadiusPixels = a_RadiusPixels;
    v_BorderWidthPixels = clamp(a_BorderWidthPixels, 0.0, a_RadiusPixels);
    v_FillColor = a_FillColor;
    v_BorderColor = a_BorderColor;
}
