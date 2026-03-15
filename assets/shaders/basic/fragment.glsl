#version 450 core

in vec3 vNormal;
out vec4 FragColor;

void main()
{
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));

    float ambient = 0.2;
    float diffuse = max(dot(normal, lightDir), 0.0);

    vec3 baseColor = vec3(0.9,0.9,0.9);
    vec3 color = (ambient + diffuse) * baseColor;

    FragColor = vec4(color, 1.0);
}
