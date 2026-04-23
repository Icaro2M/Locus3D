#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aStrength;

out vec3 vColor;
out float vStrength;
out vec3 vViewPosition;
out vec3 vWorldPosition; 

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;

void main()
{
    vec4 worldPosition = u_Model * vec4(aPos, 1.0);
    vec4 viewPosition  = u_View * worldPosition;

    gl_Position    = u_Projection * viewPosition;
    vColor         = aColor;
    vStrength      = aStrength;
    vViewPosition  = viewPosition.xyz;
    vWorldPosition = worldPosition.xyz; 
}