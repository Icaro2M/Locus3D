#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    enum class DepthFunc
    {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always
    };

    enum class BlendFactor
    {
        Zero,
        One,
        SourceAlpha,
        OneMinusSourceAlpha,
        DestinationAlpha,
        OneMinusDestinationAlpha,
        SourceColor,
        OneMinusSourceColor,
        DestinationColor,
        OneMinusDestinationColor
    };

    enum class CullFace
    {
        Back,
        Front,
        FrontAndBack
    };

    enum class FrontFace
    {
        CounterClockwise,
        Clockwise
    };

    enum class RenderPolygonMode
    {
        Fill,
        Line,
        Point
    };

    class RenderState
    {
    public:
        RenderState() = delete;

        static void set_depth_test(bool enabled);
        static void set_depth_write(bool enabled);
        static void set_depth_func(DepthFunc func);

        static void set_blend(bool enabled);
        static void set_blend_func(BlendFactor source, BlendFactor destination);

        static void set_cull_face(bool enabled);
        static void set_cull_face_mode(CullFace face);
        static void set_front_face(FrontFace face);

        static void set_polygon_mode(RenderPolygonMode mode);
        static void set_line_width(float width);
        static void set_point_size(float size);

        static void set_viewport(i32 x, i32 y, i32 width, i32 height);

        static void set_scissor_test(bool enabled);
        static void set_scissor(i32 x, i32 y, i32 width, i32 height);

        static void set_color_write(bool red, bool green, bool blue, bool alpha);

        static void reset_default();

    private:
        [[nodiscard]] static u32 gl_depth_func(DepthFunc func);
        [[nodiscard]] static u32 gl_blend_factor(BlendFactor factor);
        [[nodiscard]] static u32 gl_cull_face(CullFace face);
        [[nodiscard]] static u32 gl_front_face(FrontFace face);
        [[nodiscard]] static u32 gl_polygon_mode(RenderPolygonMode mode);
    };
}