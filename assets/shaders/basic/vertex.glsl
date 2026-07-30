#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec4 a_Color;

uniform mat4 u_Model;
uniform mat4 u_MVP;

out vec3 v_Normal;
out vec4 v_Color;

void main()
{
    mat3 normalMatrix = mat3(transpose(inverse(u_Model)));

    v_Normal = normalize(normalMatrix * a_Normal);
    v_Color = a_Color;

    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
