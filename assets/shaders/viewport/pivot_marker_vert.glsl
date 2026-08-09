#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in float a_HalfExtentPixels;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in float a_CenterRadiusPixels;
layout(location = 4) in float a_GapPixels;
layout(location = 5) in float a_ArmLengthPixels;
layout(location = 6) in float a_ArmThicknessPixels;

uniform mat4 u_ViewProjection;
uniform vec2 u_ViewportSize;

out vec2 v_Pixel;
out vec4 v_Color;
out float v_CenterRadiusPixels;
out float v_GapPixels;
out float v_ArmLengthPixels;
out float v_ArmThicknessPixels;

void main()
{
    vec4 centerClip = u_ViewProjection * vec4(a_Position, 1.0);

    if (centerClip.w <= 0.0001 ||
        u_ViewportSize.x <= 0.0 ||
        u_ViewportSize.y <= 0.0 ||
        a_HalfExtentPixels <= 0.0) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        v_Pixel = vec2(0.0);
        v_Color = vec4(0.0);
        v_CenterRadiusPixels = 0.0;
        v_GapPixels = 0.0;
        v_ArmLengthPixels = 0.0;
        v_ArmThicknessPixels = 0.0;
        return;
    }

    vec2 corner = vec2(
        (gl_VertexID == 0 || gl_VertexID == 2) ? -1.0 : 1.0,
        (gl_VertexID < 2) ? -1.0 : 1.0);
    vec2 pixelToNdc = vec2(2.0 / u_ViewportSize.x, 2.0 / u_ViewportSize.y);
    vec2 offsetPixels = corner * a_HalfExtentPixels;

    vec4 clip = centerClip;
    clip.xy += offsetPixels * pixelToNdc * centerClip.w;

    gl_Position = clip;
    v_Pixel = offsetPixels;
    v_Color = a_Color;
    v_CenterRadiusPixels = a_CenterRadiusPixels;
    v_GapPixels = a_GapPixels;
    v_ArmLengthPixels = a_ArmLengthPixels;
    v_ArmThicknessPixels = a_ArmThicknessPixels;
}
