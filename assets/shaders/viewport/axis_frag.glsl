/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#version 450 core

in vec4 v_Color;

out vec4 FragColor;

uniform float u_VertexAlphaMultiplier;

void main()
{
    FragColor = vec4(
        v_Color.rgb,
        v_Color.a * u_VertexAlphaMultiplier);
}
