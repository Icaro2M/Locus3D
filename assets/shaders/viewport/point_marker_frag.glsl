#version 450 core

in vec2 v_Local;
in float v_RadiusPixels;
in float v_BorderWidthPixels;
in vec4 v_FillColor;
in vec4 v_BorderColor;

out vec4 FragColor;

void main()
{
    float distancePixels = length(v_Local) * v_RadiusPixels;
    float signedDistance = v_RadiusPixels - distancePixels;
    float alpha = smoothstep(0.0, 1.0, signedDistance);

    if (alpha <= 0.0) {
        discard;
    }

    float borderMix = smoothstep(
        max(v_BorderWidthPixels - 1.0, 0.0),
        v_BorderWidthPixels,
        signedDistance);
    vec4 color = mix(v_BorderColor, v_FillColor, borderMix);
    FragColor = vec4(color.rgb, color.a * alpha);
}
