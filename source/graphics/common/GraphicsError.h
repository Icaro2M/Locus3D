#pragma once

#include <string>

namespace locus::graphics
{

    enum class GraphicsErrorCode
    {
        None,

        WindowCreationFailed,
        ContextCreationFailed,
        GraphicsApiUnavailable,
        GraphicsApiLoadFailed,

        InvalidOperation,
        InvalidArgument,
        UnsupportedFeature,

        BufferCreationFailed,
        BufferUploadFailed,

        VertexArrayCreationFailed,

        ShaderFileReadFailed,
        ShaderCompilationFailed,
        ShaderLinkFailed,

        TextureCreationFailed,
        TextureUploadFailed,

        FramebufferCreationFailed,
        FramebufferIncomplete,

        ResourceNotFound,
        ResourceAlreadyExists,

        Unknown
    };

    struct GraphicsError
    {
        GraphicsErrorCode code = GraphicsErrorCode::None;
        std::string message;

        [[nodiscard]] bool has_error() const
        {
            return code != GraphicsErrorCode::None;
        }

        [[nodiscard]] static GraphicsError none()
        {
            return {};
        }

        [[nodiscard]] static GraphicsError make(GraphicsErrorCode code, std::string message)
        {
            return GraphicsError{ code, std::move(message) };
        }
    };

}