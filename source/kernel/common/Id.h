#pragma once

#include <cstdint>
#include <limits>

namespace locus::kernel
{
    using IdValue = std::uint32_t;

    constexpr IdValue InvalidIdValue = std::numeric_limits<IdValue>::max();

    struct Id
    {
        IdValue value = InvalidIdValue;

        constexpr Id() = default;

        constexpr explicit Id(IdValue value)
            : value(value)
        {
        }

        [[nodiscard]] constexpr bool isValid() const
        {
            return value != InvalidIdValue;
        }

        [[nodiscard]] constexpr bool isInvalid() const
        {
            return !isValid();
        }

        friend constexpr bool operator==(Id lhs, Id rhs)
        {
            return lhs.value == rhs.value;
        }

        friend constexpr bool operator!=(Id lhs, Id rhs)
        {
            return !(lhs == rhs);
        }
    };
}