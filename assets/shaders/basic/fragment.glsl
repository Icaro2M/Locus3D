#version 450 core

in vec3 vNormal;

out vec4 FragColor;

void main()
{
    vec3 normal = normalize(vNormal);

    vec3 lightA = normalize(vec3(0.6, 0.8, 0.5));
    vec3 lightB = normalize(vec3(-0.5, -0.2, 0.7));

    float ambient = 0.38;
    float diffuseA = max(dot(normal, lightA), 0.0);
    float diffuseB = max(dot(normal, lightB), 0.0) * 0.25;

    float lighting = ambient + diffuseA * 0.55 + diffuseB;

    vec3 baseColor = vec3(0.84, 0.84, 0.82);
    vec3 color = baseColor * lighting;

    FragColor = vec4(color, 1.0);
}