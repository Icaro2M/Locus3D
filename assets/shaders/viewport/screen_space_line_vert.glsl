#version 450 core

layout(location = 0) in vec3 a_Start;
layout(location = 1) in float a_WidthPixels;
layout(location = 2) in vec3 a_End;
layout(location = 3) in vec4 a_Color;

uniform mat4 u_ViewProjection;
uniform vec2 u_ViewportSize;

out vec4 v_Color;

vec4 clamp_endpoint(vec4 point, vec4 other)
{
    const float minW = 0.0001;

    if (point.w >= minW) {
        return point;
    }

    if (other.w < minW) {
        return vec4(2.0, 2.0, 2.0, 1.0);
    }

    float denom = other.w - point.w;
    if (abs(denom) <= minW) {
        return other;
    }

    float t = clamp((minW - point.w) / denom, 0.0, 1.0);
    return mix(point, other, t);
}

void main()
{
    vec4 startClip = u_ViewProjection * vec4(a_Start, 1.0);
    vec4 endClip = u_ViewProjection * vec4(a_End, 1.0);

    startClip = clamp_endpoint(startClip, endClip);
    endClip = clamp_endpoint(endClip, startClip);

    vec2 startNdc = startClip.xy / max(startClip.w, 0.0001);
    vec2 endNdc = endClip.xy / max(endClip.w, 0.0001);
    vec2 direction = endNdc - startNdc;

    if (dot(direction, direction) <= 0.00000001 ||
        u_ViewportSize.x <= 0.0 ||
        u_ViewportSize.y <= 0.0 ||
        a_WidthPixels <= 0.0) {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        v_Color = vec4(0.0);
        return;
    }

    vec2 tangent = normalize(direction);
    vec2 normal = vec2(-tangent.y, tangent.x);
    float side = (gl_VertexID == 0 || gl_VertexID == 2) ? -1.0 : 1.0;
    bool useStart = gl_VertexID < 2;

    vec4 clip = useStart ? startClip : endClip;
    vec2 pixelToNdc = vec2(2.0 / u_ViewportSize.x, 2.0 / u_ViewportSize.y);
    vec2 offsetNdc = normal * side * (a_WidthPixels * 0.5) * pixelToNdc;

    clip.xy += offsetNdc * clip.w;

    gl_Position = clip;
    v_Color = a_Color;
}
