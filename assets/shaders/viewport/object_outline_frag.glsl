#version 450 core

uniform sampler2D u_MaskTexture;
uniform vec2 u_TextureSize;
uniform float u_HoveredWidthPixels;
uniform float u_SelectedWidthPixels;
uniform vec4 u_HoveredColor;
uniform vec4 u_SelectedColor;

in vec2 v_TexCoord;

out vec4 FragColor;

struct MaskSample
{
    uint id;
    uint category;
};

MaskSample read_mask(ivec2 pixel)
{
    ivec2 clampedPixel = clamp(
        pixel,
        ivec2(0),
        ivec2(u_TextureSize) - ivec2(1));
    vec4 value = texelFetch(u_MaskTexture, clampedPixel, 0);
    uvec4 bytes = uvec4(round(clamp(value, 0.0, 1.0) * 255.0));

    MaskSample maskValue;
    maskValue.id = bytes.r | (bytes.g << 8u) | (bytes.b << 16u);
    maskValue.category = bytes.a;
    return maskValue;
}

bool different_region(MaskSample a, MaskSample b)
{
    return a.id != b.id || a.category != b.category;
}

void main()
{
    if (u_TextureSize.x <= 0.0 || u_TextureSize.y <= 0.0) {
        discard;
    }

    ivec2 pixel = ivec2(clamp(v_TexCoord, vec2(0.0), vec2(0.999999)) * u_TextureSize);
    MaskSample center = read_mask(pixel);
    float maxWidth = max(u_HoveredWidthPixels, u_SelectedWidthPixels);
    int radius = int(ceil(max(maxWidth, 0.0)));

    if (radius <= 0) {
        discard;
    }

    uint category = 0u;
    bool edge = false;

    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x == 0 && y == 0) {
                continue;
            }

            float distancePixels = length(vec2(x, y));
            if (distancePixels > maxWidth) {
                continue;
            }

            MaskSample neighbor = read_mask(pixel + ivec2(x, y));
            if (!different_region(center, neighbor)) {
                continue;
            }

            uint candidate = max(center.category, neighbor.category);
            if (candidate == 0u) {
                continue;
            }

            float width = candidate == 2u
                ? u_SelectedWidthPixels
                : u_HoveredWidthPixels;

            if (distancePixels <= width) {
                edge = true;
                category = max(category, candidate);
            }
        }
    }

    if (!edge || category == 0u) {
        discard;
    }

    FragColor = category == 2u ? u_SelectedColor : u_HoveredColor;
}
