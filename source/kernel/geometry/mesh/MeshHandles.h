#pragma once

#include "kernel/common/Id.h"

namespace locus::kernel::geometry
{
    struct VertexHandle
    {
        Id id{};

        constexpr VertexHandle() = default;

        constexpr explicit VertexHandle(IdValue value)
            : id(value)
        {
        }

        [[nodiscard]] constexpr bool isValid() const
        {
            return id.isValid();
        }

        [[nodiscard]] constexpr bool isInvalid() const
        {
            return id.isInvalid();
        }

        friend constexpr bool operator==(VertexHandle lhs, VertexHandle rhs)
        {
            return lhs.id == rhs.id;
        }

        friend constexpr bool operator!=(VertexHandle lhs, VertexHandle rhs)
        {
            return !(lhs == rhs);
        }
    };

    struct EdgeHandle
    {
        Id id{};

        constexpr EdgeHandle() = default;

        constexpr explicit EdgeHandle(IdValue value)
            : id(value)
        {
        }

        [[nodiscard]] constexpr bool isValid() const
        {
            return id.isValid();
        }

        [[nodiscard]] constexpr bool isInvalid() const
        {
            return id.isInvalid();
        }

        friend constexpr bool operator==(EdgeHandle lhs, EdgeHandle rhs)
        {
            return lhs.id == rhs.id;
        }

        friend constexpr bool operator!=(EdgeHandle lhs, EdgeHandle rhs)
        {
            return !(lhs == rhs);
        }
    };

    struct LoopHandle
    {
        Id id{};

        constexpr LoopHandle() = default;

        constexpr explicit LoopHandle(IdValue value)
            : id(value)
        {
        }

        [[nodiscard]] constexpr bool isValid() const
        {
            return id.isValid();
        }

        [[nodiscard]] constexpr bool isInvalid() const
        {
            return id.isInvalid();
        }

        friend constexpr bool operator==(LoopHandle lhs, LoopHandle rhs)
        {
            return lhs.id == rhs.id;
        }

        friend constexpr bool operator!=(LoopHandle lhs, LoopHandle rhs)
        {
            return !(lhs == rhs);
        }
    };

    struct FaceHandle
    {
        Id id{};

        constexpr FaceHandle() = default;

        constexpr explicit FaceHandle(IdValue value)
            : id(value)
        {
        }

        [[nodiscard]] constexpr bool isValid() const
        {
            return id.isValid();
        }

        [[nodiscard]] constexpr bool isInvalid() const
        {
            return id.isInvalid();
        }

        friend constexpr bool operator==(FaceHandle lhs, FaceHandle rhs)
        {
            return lhs.id == rhs.id;
        }

        friend constexpr bool operator!=(FaceHandle lhs, FaceHandle rhs)
        {
            return !(lhs == rhs);
        }
    };
} 