#version 450 core

in vec3 vColor;
in float vStrength;
in vec3 vViewPosition;

out vec4 FragColor;

uniform float u_FadeStart;
uniform float u_FadeEnd;
uniform float u_MinAlpha;
uniform float u_MaxAlpha;

void main()
{
    float depth = length(vViewPosition.xz);
    float fade  = 1.0 - smoothstep(u_FadeStart, u_FadeEnd, depth);
    float alpha = mix(u_MinAlpha, u_MaxAlpha, fade) * vStrength;

    FragColor = vec4(vColor, alpha);
}