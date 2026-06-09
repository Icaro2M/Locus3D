/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#version 450 core

layout (location = 0) in vec3 a_Position;

uniform mat4 u_Model;
uniform mat4 u_MVP;

out vec3 v_WorldPosition;
out vec3 v_LocalPosition;

void main()
{
    vec4 worldPosition = u_Model * vec4(a_Position, 1.0);

    v_WorldPosition = worldPosition.xyz;
    v_LocalPosition = a_Position;

    gl_Position = u_MVP * vec4(a_Position, 1.0);
}