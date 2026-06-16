/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <limits>

namespace locus::kernel
{
    /**
     * @brief Unsigned storage type used by kernel object identifiers.
     */
    using IdValue = std::uint32_t;

    /**
     * @brief Sentinel value used to represent an invalid identifier.
     */
    constexpr IdValue InvalidIdValue = std::numeric_limits<IdValue>::max();

    /**
     * @brief Lightweight value type for stable references to kernel objects.
     *
     * The identifier stores only the numeric value. Ownership, lifetime, and
     * lookup rules are defined by the containers that create each ID.
     */
    struct Id
    {
        /**
         * @brief Raw identifier value.
         */
        IdValue value = InvalidIdValue;

        /**
         * @brief Creates an invalid identifier.
         */
        constexpr Id() = default;

        /**
         * @brief Creates an identifier from a raw numeric value.
         *
         * @param value Raw identifier value.
         */
        constexpr explicit Id(IdValue value)
            : value(value)
        {
        }

        /**
         * @brief Checks whether this identifier references a valid object.
         *
         * @return True when the stored value is not the invalid sentinel.
         */
        [[nodiscard]] constexpr bool is_valid() const
        {
            return value != InvalidIdValue;
        }

        /**
         * @brief Checks whether this identifier is invalid.
         *
         * @return True when the stored value is the invalid sentinel.
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
        friend constexpr bool operator==(Id lhs, Id rhs)
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
        friend constexpr bool operator!=(Id lhs, Id rhs)
        {
            return !(lhs == rhs);
        }
    };
}
