/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */
 
 #version 450 core

in vec3 v_WorldPosition;
in vec3 v_LocalPosition;

uniform float u_MinorSpacing;
uniform float u_MajorSpacing;
uniform float u_FadeStart;
uniform float u_FadeEnd;
uniform float u_LineStrength;
uniform float u_MajorLineStrength;

uniform vec4 u_MinorColor;
uniform vec4 u_MajorColor;

out vec4 FragColor;

float grid_line(vec2 position, float spacing, float thickness)
{
    vec2 grid = abs(fract(position / spacing - 0.5) - 0.5) / fwidth(position / spacing);
    float line = min(grid.x, grid.y);

    return 1.0 - min(line / thickness, 1.0);
}

void main()
{
    vec2 worldXZ = v_WorldPosition.xz;

    float minorLine = grid_line(worldXZ, u_MinorSpacing, 1.0);
    float majorLine = grid_line(worldXZ, u_MajorSpacing, 1.35);

    float distanceFromCameraPlane = length(v_LocalPosition.xz);
    float fade = 1.0 - smoothstep(u_FadeStart, u_FadeEnd, distanceFromCameraPlane);

    vec3 color = u_MinorColor.rgb * minorLine * u_LineStrength;
    color = max(color, u_MajorColor.rgb * majorLine * u_MajorLineStrength);

    float visibility = max(
        minorLine * u_LineStrength,
        majorLine * u_MajorLineStrength
    );

    visibility *= fade;

    if (visibility <= 0.005)
    {
        discard;
    }

    FragColor = vec4(color * fade, 1.0);
}