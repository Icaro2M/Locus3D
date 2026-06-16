/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Id.h"

namespace locus::kernel::geometry
{
    /**
     * @brief Type-safe handle referencing a vertex element.
     */
    struct VertexHandle
    {
        /**
         * @brief Underlying kernel identifier.
         */
        Id id{};

        /**
         * @brief Creates an invalid vertex handle.
         */
        constexpr VertexHandle() = default;

        /**
         * @brief Creates a vertex handle from a raw identifier value.
         *
         * @param value Raw identifier value.
         */
        constexpr explicit VertexHandle(IdValue value)
            : id(value)
        {
        }

        /**
         * @brief Checks whether this handle references a valid vertex.
         *
         * @return True when the underlying identifier is valid.
         */
        [[nodiscard]] constexpr bool is_valid() const
        {
            return id.is_valid();
        }

        /**
         * @brief Checks whether this handle is invalid.
         *
         * @return True when the underlying identifier is invalid.
         */
        [[nodiscard]] constexpr bool is_invalid() const
        {
            return id.is_invalid();
        }

        /**
         * @brief Compares two vertex handles for equality.
         *
         * @param lhs Left-hand vertex handle.
         * @param rhs Right-hand vertex handle.
         * @return True when both handles reference the same identifier.
         */
        friend constexpr bool operator==(VertexHandle lhs, VertexHandle rhs)
        {
            return lhs.id == rhs.id;
        }

        /**
         * @brief Compares two vertex handles for inequality.
         *
         * @param lhs Left-hand vertex handle.
         * @param rhs Right-hand vertex handle.
         * @return True when the handles reference different identifiers.
         */
        friend constexpr bool operator!=(VertexHandle lhs, VertexHandle rhs)
        {
            return !(lhs == rhs);
        }
    };

    /**
     * @brief Type-safe handle referencing an edge element.
     */
    struct EdgeHandle
    {
        /**
         * @brief Underlying kernel identifier.
         */
        Id id{};

        /**
         * @brief Creates an invalid edge handle.
         */
        constexpr EdgeHandle() = default;

        /**
         * @brief Creates an edge handle from a raw identifier value.
         *
         * @param value Raw identifier value.
         */
        constexpr explicit EdgeHandle(IdValue value)
            : id(value)
        {
        }

        /**
         * @brief Checks whether this handle references a valid edge.
         *
         * @return True when the underlying identifier is valid.
         */
        [[nodiscard]] constexpr bool is_valid() const
        {
            return id.is_valid();
        }

        /**
         * @brief Checks whether this handle is invalid.
         *
         * @return True when the underlying identifier is invalid.
         */
        [[nodiscard]] constexpr bool is_invalid() const
        {
            return id.is_invalid();
        }

        /**
         * @brief Compares two edge handles for equality.
         *
         * @param lhs Left-hand edge handle.
         * @param rhs Right-hand edge handle.
         * @return True when both handles reference the same identifier.
         */
        friend constexpr bool operator==(EdgeHandle lhs, EdgeHandle rhs)
        {
            return lhs.id == rhs.id;
        }

        /**
         * @brief Compares two edge handles for inequality.
         *
         * @param lhs Left-hand edge handle.
         * @param rhs Right-hand edge handle.
         * @return True when the handles reference different identifiers.
         */
        friend constexpr bool operator!=(EdgeHandle lhs, EdgeHandle rhs)
        {
            return !(lhs == rhs);
        }
    };

    /**
     * @brief Type-safe handle referencing a loop element.
     */
    struct LoopHandle
    {
        /**
         * @brief Underlying kernel identifier.
         */
        Id id{};

        /**
         * @brief Creates an invalid loop handle.
         */
        constexpr LoopHandle() = default;

        /**
         * @brief Creates a loop handle from a raw identifier value.
         *
         * @param value Raw identifier value.
         */
        constexpr explicit LoopHandle(IdValue value)
            : id(value)
        {
        }

        /**
         * @brief Checks whether this handle references a valid loop.
         *
         * @return True when the underlying identifier is valid.
         */
        [[nodiscard]] constexpr bool is_valid() const
        {
            return id.is_valid();
        }

        /**
         * @brief Checks whether this handle is invalid.
         *
         * @return True when the underlying identifier is invalid.
         */
        [[nodiscard]] constexpr bool is_invalid() const
        {
            return id.is_invalid();
        }

        /**
         * @brief Compares two loop handles for equality.
         *
         * @param lhs Left-hand loop handle.
         * @param rhs Right-hand loop handle.
         * @return True when both handles reference the same identifier.
         */
        friend constexpr bool operator==(LoopHandle lhs, LoopHandle rhs)
        {
            return lhs.id == rhs.id;
        }

        /**
         * @brief Compares two loop handles for inequality.
         *
         * @param lhs Left-hand loop handle.
         * @param rhs Right-hand loop handle.
         * @return True when the handles reference different identifiers.
         */
        friend constexpr bool operator!=(LoopHandle lhs, LoopHandle rhs)
        {
            return !(lhs == rhs);
        }
    };

    /**
     * @brief Type-safe handle referencing a face element.
     */
    struct FaceHandle
    {
        /**
         * @brief Underlying kernel identifier.
         */
        Id id{};

        /**
         * @brief Creates an invalid face handle.
         */
        constexpr FaceHandle() = default;

        /**
         * @brief Creates a face handle from a raw identifier value.
         *
         * @param value Raw identifier value.
         */
        constexpr explicit FaceHandle(IdValue value)
            : id(value)
        {
        }

        /**
         * @brief Checks whether this handle references a valid face.
         *
         * @return True when the underlying identifier is valid.
         */
        [[nodiscard]] constexpr bool is_valid() const
        {
            return id.is_valid();
        }

        /**
         * @brief Checks whether this handle is invalid.
         *
         * @return True when the underlying identifier is invalid.
         */
        [[nodiscard]] constexpr bool is_invalid() const
        {
            return id.is_invalid();
        }

        /**
         * @brief Compares two face handles for equality.
         *
         * @param lhs Left-hand face handle.
         * @param rhs Right-hand face handle.
         * @return True when both handles reference the same identifier.
         */
        friend constexpr bool operator==(FaceHandle lhs, FaceHandle rhs)
        {
            return lhs.id == rhs.id;
        }

        /**
         * @brief Compares two face handles for inequality.
         *
         * @param lhs Left-hand face handle.
         * @param rhs Right-hand face handle.
         * @return True when the handles reference different identifiers.
         */
        friend constexpr bool operator!=(FaceHandle lhs, FaceHandle rhs)
        {
            return !(lhs == rhs);
        }
    };
}
