/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <functional>
#include <limits>

namespace locus::editor {

    /**
     * @brief Raw storage type used by editor scene node identifiers.
     */
    using SceneNodeIdValue = std::uint64_t;

    /**
     * @brief Sentinel value used to represent an invalid scene node identifier.
     */
    constexpr SceneNodeIdValue InvalidSceneNodeIdValue =
        std::numeric_limits<SceneNodeIdValue>::max();

    /**
     * @brief Stable identifier used to reference editor scene nodes.
     */
    struct SceneNodeId {
        /**
         * @brief Raw identifier value.
         */
        SceneNodeIdValue value = InvalidSceneNodeIdValue;

        /**
         * @brief Creates an invalid node identifier.
         */
        constexpr SceneNodeId() = default;

        /**
         * @brief Creates a node identifier from a raw value.
         *
         * @param value Raw identifier value.
         */
        constexpr explicit SceneNodeId(SceneNodeIdValue value)
            : value(value)
        {
        }

        /**
         * @brief Checks whether this identifier references a valid node.
         *
         * @return True when the identifier is valid.
         */
        [[nodiscard]] constexpr bool is_valid() const
        {
            return value != InvalidSceneNodeIdValue;
        }

        /**
         * @brief Checks whether this identifier is invalid.
         *
         * @return True when the identifier is invalid.
         */
        [[nodiscard]] constexpr bool is_invalid() const
        {
            return !is_valid();
        }

        /**
         * @brief Compares two identifiers for equality.
         *
         * @param lhs Left-hand identifier.
         * @param rhs Right-hand identifier.
         * @return True when both identifiers store the same value.
         */
        friend constexpr bool operator==(SceneNodeId lhs, SceneNodeId rhs)
        {
            return lhs.value == rhs.value;
        }

        /**
         * @brief Compares two identifiers for inequality.
         *
         * @param lhs Left-hand identifier.
         * @param rhs Right-hand identifier.
         * @return True when the identifiers store different values.
         */
        friend constexpr bool operator!=(SceneNodeId lhs, SceneNodeId rhs)
        {
            return !(lhs == rhs);
        }
    };

}

namespace std {

    /**
     * @brief Hash functor for editor scene node identifiers.
     */
    template <>
    struct hash<locus::editor::SceneNodeId> {
        /**
         * @brief Hashes a scene node identifier.
         *
         * @param id Identifier to hash.
         * @return Hash value.
         */
        std::size_t operator()(locus::editor::SceneNodeId id) const noexcept
        {
            return std::hash<locus::editor::SceneNodeIdValue>{}(id.value);
        }
    };

}