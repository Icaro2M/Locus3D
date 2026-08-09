#version 450 core

in vec2 v_Pixel;
in vec4 v_Color;
in float v_CenterRadiusPixels;
in float v_GapPixels;
in float v_ArmLengthPixels;
in float v_ArmThicknessPixels;

out vec4 FragColor;

float rect_alpha(vec2 p, vec2 halfSize)
{
    vec2 d = abs(p) - halfSize;
    float outside = length(max(d, vec2(0.0)));
    float inside = min(max(d.x, d.y), 0.0);
    float signedDistance = outside + inside;
    return 1.0 - smoothstep(-0.5, 0.5, signedDistance);
}

void main()
{
    float centerDistance = length(v_Pixel);
    float centerAlpha = 1.0 - smoothstep(
        v_CenterRadiusPixels - 0.75,
        v_CenterRadiusPixels + 0.75,
        centerDistance);

    float armCenter = v_CenterRadiusPixels + v_GapPixels + v_ArmLengthPixels * 0.5;
    vec2 horizontalHalf = vec2(v_ArmLengthPixels * 0.5, v_ArmThicknessPixels * 0.5);
    vec2 verticalHalf = vec2(v_ArmThicknessPixels * 0.5, v_ArmLengthPixels * 0.5);

    float rightAlpha = rect_alpha(v_Pixel - vec2(armCenter, 0.0), horizontalHalf);
    float leftAlpha = rect_alpha(v_Pixel + vec2(armCenter, 0.0), horizontalHalf);
    float topAlpha = rect_alpha(v_Pixel - vec2(0.0, armCenter), verticalHalf);
    float bottomAlpha = rect_alpha(v_Pixel + vec2(0.0, armCenter), verticalHalf);

    float alpha = max(centerAlpha, max(max(leftAlpha, rightAlpha), max(topAlpha, bottomAlpha)));
    if (alpha <= 0.0) {
        discard;
    }

    FragColor = vec4(v_Color.rgb, v_Color.a * alpha);
}
