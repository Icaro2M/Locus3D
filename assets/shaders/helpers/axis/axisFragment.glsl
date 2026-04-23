#version 450 core

in vec3 vColor;
in float vStrength;
in vec3 vViewPosition;
in vec3 vWorldPosition;

out vec4 FragColor;

uniform float u_Alpha;
uniform float u_FadeStart;
uniform float u_FadeEnd;
uniform float u_MinAlpha;
uniform float u_CamX;
uniform float u_CamY;
uniform float u_CamZ;

void main()
{
    vec3 camPos = vec3(u_CamX, u_CamY, u_CamZ);
    float dist  = length(vWorldPosition - camPos);
    float fade  = 1.0 - smoothstep(u_FadeStart, u_FadeEnd, dist);
    float alpha = mix(u_MinAlpha, u_Alpha, fade) * vStrength;

    FragColor = vec4(vColor, alpha);
}