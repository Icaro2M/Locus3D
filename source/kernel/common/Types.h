#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace locus::kernel {

using Int8 = std::int8_t;
using Int16 = std::int16_t;
using Int32 = std::int32_t;
using Int64 = std::int64_t;

using UInt8 = std::uint8_t;
using UInt16 = std::uint16_t;
using UInt32 = std::uint32_t;
using UInt64 = std::uint64_t;

using Size = std::size_t;
using Index = std::uint32_t;
using Count = std::uint32_t;
using Real = float;

constexpr Index InvalidIndex = std::numeric_limits<Index>::max();
constexpr Count InvalidCount = std::numeric_limits<Count>::max();
constexpr Real DefaultEpsilon = 1.0e-5f;

enum class Axis {
    X,
    Y,
    Z
};

enum class Orientation {
    Positive,
    Negative
};

enum class WindingOrder {
    Clockwise,
    CounterClockwise
};

}
