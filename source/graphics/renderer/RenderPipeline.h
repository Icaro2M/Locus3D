#pragma once

#include "graphics/renderer/DrawList.h"
#include "graphics/renderer/RenderQueue.h"
#include "graphics/renderer/RenderStats.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"

#include <cstddef>

namespace locus::graphics
{
    class RenderPipeline
    {
    public:
        RenderPipeline() = default;
        ~RenderPipeline() = default;

        RenderPipeline(const RenderPipeline&) = delete;
        RenderPipeline& operator=(const RenderPipeline&) = delete;

        RenderPipeline(RenderPipeline&&) noexcept = default;
        RenderPipeline& operator=(RenderPipeline&&) noexcept = default;

        void begin_frame();
        void reserve(std::size_t objectCount);

        RenderObject& submit(RenderObject object);
        void submit(const RenderScene& scene);
        void submit(const DrawList& drawList);

        void build_queue();
        void render(Renderer& renderer);

        [[nodiscard]] bool empty() const;
        [[nodiscard]] std::size_t object_count() const;
        [[nodiscard]] std::size_t command_count() const;

        [[nodiscard]] const DrawList& draw_list() const;
        [[nodiscard]] const RenderQueue& queue() const;
        [[nodiscard]] const RenderStats& stats() const;

    private:
        DrawList drawList_;
        RenderQueue queue_;
        RenderStats stats_{};
        bool queueDirty_ = true;
    };
}