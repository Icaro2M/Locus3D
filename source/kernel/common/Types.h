/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace locus::kernel {

/**
 * @brief Signed 8-bit integer type used by the kernel API.
 */
using Int8 = std::int8_t;

/**
 * @brief Signed 16-bit integer type used by the kernel API.
 */
using Int16 = std::int16_t;

/**
 * @brief Signed 32-bit integer type used by the kernel API.
 */
using Int32 = std::int32_t;

/**
 * @brief Signed 64-bit integer type used by the kernel API.
 */
using Int64 = std::int64_t;

/**
 * @brief Unsigned 8-bit integer type used by the kernel API.
 */
using UInt8 = std::uint8_t;

/**
 * @brief Unsigned 16-bit integer type used by the kernel API.
 */
using UInt16 = std::uint16_t;

/**
 * @brief Unsigned 32-bit integer type used by the kernel API.
 */
using UInt32 = std::uint32_t;

/**
 * @brief Unsigned 64-bit integer type used by the kernel API.
 */
using UInt64 = std::uint64_t;

/**
 * @brief Native memory size type used for container sizes.
 */
using Size = std::size_t;

/**
 * @brief Compact unsigned index type used for kernel element references.
 */
using Index = std::uint32_t;

/**
 * @brief Compact unsigned count type used for kernel element totals.
 */
using Count = std::uint32_t;

/**
 * @brief Floating-point scalar type used by geometric calculations.
 */
using Real = float;

/**
 * @brief Sentinel value used to represent an invalid index.
 */
constexpr Index InvalidIndex = std::numeric_limits<Index>::max();

/**
 * @brief Sentinel value used to represent an invalid count.
 */
constexpr Count InvalidCount = std::numeric_limits<Count>::max();

/**
 * @brief Default tolerance used for floating-point comparisons.
 */
constexpr Real DefaultEpsilon = 1.0e-5f;

/**
 * @brief Identifies one of the three Cartesian axes.
 */
enum class Axis {
    /**
     * @brief X axis.
     */
    X,

    /**
     * @brief Y axis.
     */
    Y,

    /**
     * @brief Z axis.
     */
    Z
};

/**
 * @brief Describes a positive or negative direction along an axis.
 */
enum class Orientation {
    /**
     * @brief Positive axis direction.
     */
    Positive,

    /**
     * @brief Negative axis direction.
     */
    Negative
};

/**
 * @brief Describes the vertex order used by polygon boundaries.
 */
enum class WindingOrder {
    /**
     * @brief Clockwise winding order.
     */
    Clockwise,

    /**
     * @brief Counter-clockwise winding order.
     */
    CounterClockwise
};

}
