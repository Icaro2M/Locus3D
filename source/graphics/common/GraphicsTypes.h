/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <string>

namespace locus::graphics
{
    /**
     * @brief Unsigned 8-bit integer.
     */
    using u8 = std::uint8_t;

    /**
     * @brief Unsigned 16-bit integer.
     */
    using u16 = std::uint16_t;

    /**
     * @brief Unsigned 32-bit integer.
     */
    using u32 = std::uint32_t;

    /**
     * @brief Unsigned 64-bit integer.
     */
    using u64 = std::uint64_t;

    /**
     * @brief Signed 8-bit integer.
     */
    using i8 = std::int8_t;

    /**
     * @brief Signed 16-bit integer.
     */
    using i16 = std::int16_t;

    /**
     * @brief Signed 32-bit integer.
     */
    using i32 = std::int32_t;

    /**
     * @brief Signed 64-bit integer.
     */
    using i64 = std::int64_t;

    /**
     * @brief 32-bit floating-point value.
     */
    using f32 = float;

    /**
     * @brief 64-bit floating-point value.
     */
    using f64 = double;

    /**
     * @brief Opaque identifier for graphics resources.
     */
    using GraphicsResourceId = u32;

    /**
     * @brief Sentinel value for invalid graphics resources.
     */
    constexpr GraphicsResourceId InvalidGraphicsResourceId = 0;

    /**
     * @brief Supported graphics backends.
     */
    enum class GraphicsApi
    {
        None,
        OpenGL
    };

    /**
     * @brief Primitive assembly modes used by draw calls.
     */
    enum class PrimitiveTopology
    {
        Points,
        Lines,
        LineStrip,
        Triangles,
        TriangleStrip
    };

    /**
     * @brief Supported index buffer element types.
     */
    enum class IndexType
    {
        UInt16,
        UInt32
    };

    /**
     * @brief Expected update frequency for buffer storage.
     */
    enum class BufferUsage
    {
        Static,
        Dynamic,
        Stream
    };

    /**
     * @brief GPU buffer binding categories.
     */
    enum class BufferType
    {
        Vertex,
        Index,
        Uniform
    };

    /**
     * @brief Texture storage formats supported by the graphics layer.
     */
    enum class TextureFormat
    {
        Unknown,
        R8,
        RGB8,
        RGBA8,
        Depth24Stencil8
    };

    /**
     * @brief Programmable shader stages supported by the graphics layer.
     */
    enum class ShaderStage
    {
        Vertex,
        Fragment,
        Geometry
    };

    /**
     * @brief Polygon rasterization modes.
     */
    enum class PolygonMode
    {
        Fill,
        Line,
        Point
    };

    /**
     * @brief Face culling modes.
     */
    enum class CullMode
    {
        None,
        Front,
        Back,
        FrontAndBack
    };

    /**
     * @brief Depth test modes.
     */
    enum class DepthMode
    {
        Disabled,
        Less,
        LessOrEqual
    };

    /**
     * @brief Rectangular viewport region in pixels.
     */
    struct ViewportRect
    {
        /**
         * @brief Left viewport coordinate.
         */
        i32 x = 0;

        /**
         * @brief Bottom viewport coordinate.
         */
        i32 y = 0;

        /**
         * @brief Viewport width in pixels.
         */
        i32 width = 1;

        /**
         * @brief Viewport height in pixels.
         */
        i32 height = 1;
    };

    /**
     * @brief Linear RGBA color with float channels.
     */
    struct ColorRGBA
    {
        /**
         * @brief Red channel.
         */
        f32 r = 1.0f;

        /**
         * @brief Green channel.
         */
        f32 g = 1.0f;

        /**
         * @brief Blue channel.
         */
        f32 b = 1.0f;

        /**
         * @brief Alpha channel.
         */
        f32 a = 1.0f;
    };

    /**
     * @brief Clear values used for color, depth, and stencil attachments.
     */
    struct ClearState
    {
        /**
         * @brief Color clear value.
         */
        ColorRGBA color{};

        /**
         * @brief Depth clear value.
         */
        f32 depth = 1.0f;

        /**
         * @brief Stencil clear value.
         */
        i32 stencil = 0;
    };

}
