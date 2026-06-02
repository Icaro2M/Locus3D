#pragma once

#include <cstdint>
#include <string>

namespace locus::graphics
{

    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using i8 = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using f32 = float;
    using f64 = double;

    using GraphicsResourceId = u32;

    constexpr GraphicsResourceId InvalidGraphicsResourceId = 0;

    enum class GraphicsApi
    {
        None,
        OpenGL
    };

    enum class PrimitiveTopology
    {
        Points,
        Lines,
        LineStrip,
        Triangles,
        TriangleStrip
    };

    enum class IndexType
    {
        UInt16,
        UInt32
    };

    enum class BufferUsage
    {
        Static,
        Dynamic,
        Stream
    };

    enum class BufferType
    {
        Vertex,
        Index,
        Uniform
    };

    enum class TextureFormat
    {
        Unknown,
        R8,
        RGB8,
        RGBA8,
        Depth24Stencil8
    };

    enum class ShaderStage
    {
        Vertex,
        Fragment,
        Geometry
    };

    enum class PolygonMode
    {
        Fill,
        Line,
        Point
    };

    enum class CullMode
    {
        None,
        Front,
        Back,
        FrontAndBack
    };

    enum class DepthMode
    {
        Disabled,
        Less,
        LessOrEqual
    };

    struct ViewportRect
    {
        i32 x = 0;
        i32 y = 0;
        i32 width = 1;
        i32 height = 1;
    };

    struct ColorRGBA
    {
        f32 r = 1.0f;
        f32 g = 1.0f;
        f32 b = 1.0f;
        f32 a = 1.0f;
    };

    struct ClearState
    {
        ColorRGBA color{};
        f32 depth = 1.0f;
        i32 stencil = 0;
    };

}