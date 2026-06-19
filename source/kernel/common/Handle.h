/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Id.h"

namespace locus::kernel {

    /**
     * @brief Type-safe lightweight handle used to reference kernel objects.
     *
     * @tparam Tag Unique tag type that distinguishes this handle from other handle
     * categories while keeping the same storage representation.
     */
    template <typename Tag>
    struct Handle {
        /**
         * @brief Underlying kernel identifier.
         */
        Id id{};

        /**
         * @brief Creates an invalid handle.
         */
        constexpr Handle() = default;

        /**
         * @brief Creates a handle from a raw identifier value.
         *
         * @param value Raw identifier value.
         */
        constexpr explicit Handle(IdValue value)
            : id(value)
        {
        }

        /**
         * @brief Checks whether this handle references a valid identifier.
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
         * @brief Compares two handles for equality.
         *
         * @param lhs Left-hand handle.
         * @param rhs Right-hand handle.
         * @return True when both handles store the same identifier.
         */
        friend constexpr bool operator==(Handle lhs, Handle rhs)
        {
            return lhs.id == rhs.id;
        }

        /**
         * @brief Compares two handles for inequality.
         *
         * @param lhs Left-hand handle.
         * @param rhs Right-hand handle.
         * @return True when the handles store different identifiers.
         */
        friend constexpr bool operator!=(Handle lhs, Handle rhs)
        {
            return !(lhs == rhs);
        }
    };

}