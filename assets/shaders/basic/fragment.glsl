#version 450 core

in vec3 v_Normal;
in vec4 v_Color;

uniform vec4 u_BaseColor;
uniform int u_UseVertexColor;
uniform int u_FaceOrientationEnabled;
uniform vec4 u_FrontFaceOrientationColor;
uniform vec4 u_BackFaceOrientationColor;

out vec4 FragColor;

void main()
{
    if (u_FaceOrientationEnabled != 0)
    {
        vec4 orientationColor = gl_FrontFacing
            ? u_FrontFaceOrientationColor
            : u_BackFaceOrientationColor;

        FragColor = vec4(orientationColor.rgb, orientationColor.a);
        return;
    }

    vec3 normal = normalize(v_Normal);

    vec3 lightA = normalize(vec3(0.6, 0.8, 0.5));
    vec3 lightB = normalize(vec3(-0.5, -0.2, 0.7));

    float ambient = 0.38;
    float diffuseA = max(dot(normal, lightA), 0.0);
    float diffuseB = max(dot(normal, lightB), 0.0) * 0.25;
    float rim = pow(1.0 - abs(normal.z), 2.0) * 0.12;

    float lighting = ambient + diffuseA * 0.55 + diffuseB + rim;

    vec3 clay = vec3(0.84, 0.84, 0.82);
    vec4 sourceColor = u_UseVertexColor != 0 ? v_Color : u_BaseColor;
    vec3 baseColor = mix(clay, sourceColor.rgb, 0.18);
    vec3 color = baseColor * lighting;

    FragColor = vec4(color, sourceColor.a);
}
