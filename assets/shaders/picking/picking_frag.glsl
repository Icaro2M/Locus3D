/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */
 
 #version 450 core

uniform vec4 u_PickingColor;

out vec4 FragColor;

void main()
{
    FragColor = u_PickingColor;
}