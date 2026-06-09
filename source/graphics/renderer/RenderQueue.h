#pragma once

#include "graphics/renderer/RenderCommand.h"

#include <cstddef>
#include <vector>

namespace locus::graphics
{
    class RenderScene;
    struct RenderObject;

    class RenderQueue
    {
    public:
        using CommandList = std::vector<RenderCommand>;

        RenderQueue() = default;
        ~RenderQueue() = default;

        RenderQueue(const RenderQueue&) = default;
        RenderQueue& operator=(const RenderQueue&) = default;

        RenderQueue(RenderQueue&&) noexcept = default;
        RenderQueue& operator=(RenderQueue&&) noexcept = default;

        void clear();
        void reserve(std::size_t count);

        void add_object(const RenderObject& object);
        void build_from_scene(const RenderScene& scene);

        void sort();

        [[nodiscard]] bool empty() const;
        [[nodiscard]] std::size_t command_count() const;
        [[nodiscard]] const CommandList& commands() const;

    private:
        [[nodiscard]] static u32 priority_for_layer(RenderLayer layer);

    private:
        CommandList commands_;
        u32 nextSequence_ = 0;
    };
}