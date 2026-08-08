/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    /**
     * @brief Depth comparison function.
     */
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

    /**
     * @brief Blending factor used by color blending.
     */
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

    /**
     * @brief Face selection used by culling state.
     */
    enum class CullFace
    {
        Back,
        Front,
        FrontAndBack
    };

    /**
     * @brief Vertex winding considered to be front-facing.
     */
    enum class FrontFace
    {
        CounterClockwise,
        Clockwise
    };

    /**
     * @brief Polygon rasterization mode.
     */
    enum class RenderPolygonMode
    {
        Fill,
        Line,
        Point
    };

    /**
     * @brief Stateless helpers for configuring common OpenGL render state.
     */
    class RenderState
    {
    public:
        RenderState() = delete;

        /**
         * @brief Enables or disables depth testing.
         *
         * @param enabled True to enable depth testing.
         */
        static void set_depth_test(bool enabled);

        /**
         * @brief Enables or disables depth buffer writes.
         *
         * @param enabled True to allow depth writes.
         */
        static void set_depth_write(bool enabled);

        /**
         * @brief Sets the depth comparison function.
         *
         * @param func Depth comparison function.
         */
        static void set_depth_func(DepthFunc func);

        /**
         * @brief Enables or disables color blending.
         *
         * @param enabled True to enable blending.
         */
        static void set_blend(bool enabled);

        /**
         * @brief Sets source and destination blend factors.
         *
         * @param source Source color factor.
         * @param destination Destination color factor.
         */
        static void set_blend_func(BlendFactor source, BlendFactor destination);

        /**
         * @brief Enables or disables face culling.
         *
         * @param enabled True to enable culling.
         */
        static void set_cull_face(bool enabled);

        /**
         * @brief Sets which faces are culled.
         *
         * @param face Face selection.
         */
        static void set_cull_face_mode(CullFace face);

        /**
         * @brief Sets front-face winding.
         *
         * @param face Front-face winding mode.
         */
        static void set_front_face(FrontFace face);

        /**
         * @brief Sets polygon rasterization mode.
         *
         * @param mode Polygon mode.
         */
        static void set_polygon_mode(RenderPolygonMode mode);

        /**
         * @brief Enables or disables filled polygon depth offset.
         *
         * @param enabled True to enable polygon offset for filled polygons.
         */
        static void set_polygon_offset_fill(bool enabled);

        /**
         * @brief Sets the filled polygon depth offset parameters.
         *
         * @param factor Scale factor for the maximum depth slope.
         * @param units Constant depth offset units.
         */
        static void set_polygon_offset(float factor, float units);

        /**
         * @brief Sets line rasterization width.
         *
         * @param width Line width in pixels.
         */
        static void set_line_width(float width);

        /**
         * @brief Sets point rasterization size.
         *
         * @param size Point size in pixels.
         */
        static void set_point_size(float size);

        /**
         * @brief Sets the active viewport rectangle.
         *
         * @param x Viewport X coordinate.
         * @param y Viewport Y coordinate.
         * @param width Viewport width.
         * @param height Viewport height.
         */
        static void set_viewport(i32 x, i32 y, i32 width, i32 height);

        /**
         * @brief Enables or disables scissor testing.
         *
         * @param enabled True to enable scissor testing.
         */
        static void set_scissor_test(bool enabled);

        /**
         * @brief Sets the active scissor rectangle.
         *
         * @param x Scissor X coordinate.
         * @param y Scissor Y coordinate.
         * @param width Scissor width.
         * @param height Scissor height.
         */
        static void set_scissor(i32 x, i32 y, i32 width, i32 height);

        /**
         * @brief Enables or disables writes for each color channel.
         *
         * @param red True to write red.
         * @param green True to write green.
         * @param blue True to write blue.
         * @param alpha True to write alpha.
         */
        static void set_color_write(bool red, bool green, bool blue, bool alpha);

        /**
         * @brief Restores the default render state used by Locus3D.
         */
        static void reset_default();

    private:
        [[nodiscard]] static u32 gl_depth_func(DepthFunc func);
        [[nodiscard]] static u32 gl_blend_factor(BlendFactor factor);
        [[nodiscard]] static u32 gl_cull_face(CullFace face);
        [[nodiscard]] static u32 gl_front_face(FrontFace face);
        [[nodiscard]] static u32 gl_polygon_mode(RenderPolygonMode mode);
    };
}
