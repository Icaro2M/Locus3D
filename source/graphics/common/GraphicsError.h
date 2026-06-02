/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

namespace locus::graphics
{
    /**
     * @brief Enumerates graphics subsystem error categories.
     */
    enum class GraphicsErrorCode
    {
        /**
         * @brief No error was reported.
         */
        None,

        /**
         * @brief A window could not be created.
         */
        WindowCreationFailed,

        /**
         * @brief A graphics context could not be created or initialized.
         */
        ContextCreationFailed,

        /**
         * @brief The requested graphics API or version is unavailable.
         */
        GraphicsApiUnavailable,

        /**
         * @brief Graphics API function loading failed.
         */
        GraphicsApiLoadFailed,

        /**
         * @brief The requested operation is invalid for the current state.
         */
        InvalidOperation,

        /**
         * @brief One or more input arguments are invalid.
         */
        InvalidArgument,

        /**
         * @brief The requested feature is not supported by the backend.
         */
        UnsupportedFeature,

        /**
         * @brief Buffer object creation failed.
         */
        BufferCreationFailed,

        /**
         * @brief Buffer data upload failed.
         */
        BufferUploadFailed,

        /**
         * @brief Vertex array object creation failed.
         */
        VertexArrayCreationFailed,

        /**
         * @brief Shader source file could not be read.
         */
        ShaderFileReadFailed,

        /**
         * @brief Shader source compilation failed.
         */
        ShaderCompilationFailed,

        /**
         * @brief Shader program linking failed.
         */
        ShaderLinkFailed,

        /**
         * @brief Texture object creation failed.
         */
        TextureCreationFailed,

        /**
         * @brief Texture data upload failed.
         */
        TextureUploadFailed,

        /**
         * @brief Framebuffer object creation failed.
         */
        FramebufferCreationFailed,

        /**
         * @brief Framebuffer completeness validation failed.
         */
        FramebufferIncomplete,

        /**
         * @brief A requested graphics resource was not found.
         */
        ResourceNotFound,

        /**
         * @brief A graphics resource already exists.
         */
        ResourceAlreadyExists,

        /**
         * @brief Fallback category for unspecified graphics errors.
         */
        Unknown
    };

    /**
     * @brief Stores an error code and human-readable diagnostic message.
     */
    struct GraphicsError
    {
        /**
         * @brief Error category.
         */
        GraphicsErrorCode code = GraphicsErrorCode::None;

        /**
         * @brief Human-readable diagnostic message.
         */
        std::string message;

        /**
         * @brief Checks whether this value represents an error.
         *
         * @return True when the error code is not None.
         */
        [[nodiscard]] bool has_error() const
        {
            return code != GraphicsErrorCode::None;
        }

        /**
         * @brief Creates an empty error value.
         *
         * @return Error value with code None.
         */
        [[nodiscard]] static GraphicsError none()
        {
            return {};
        }

        /**
         * @brief Creates an error value with a code and message.
         *
         * @param code Error category.
         * @param message Human-readable diagnostic message.
         * @return Populated error value.
         */
        [[nodiscard]] static GraphicsError make(GraphicsErrorCode code, std::string message)
        {
            return GraphicsError{ code, std::move(message) };
        }
    };

}
