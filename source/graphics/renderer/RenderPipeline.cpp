#include "graphics/renderer/RenderPipeline.h"

#include <utility>

namespace locus::graphics
{
    void RenderPipeline::begin_frame()
    {
        drawList_.clear();
        queue_.clear();
        stats_.reset();
        queueDirty_ = true;
    }

    void RenderPipeline::reserve(std::size_t objectCount)
    {
        drawList_.reserve(objectCount);
        queue_.reserve(objectCount);
    }

    RenderObject& RenderPipeline::submit(RenderObject object)
    {
        queueDirty_ = true;
        return drawList_.add_object(std::move(object));
    }

    void RenderPipeline::submit(const RenderScene& scene)
    {
        if (scene.empty())
        {
            return;
        }

        drawList_.add_objects(scene);
        queueDirty_ = true;
    }

    void RenderPipeline::submit(const DrawList& drawList)
    {
        if (drawList.empty())
        {
            return;
        }

        drawList_.add_objects(drawList.objects());
        queueDirty_ = true;
    }

    void RenderPipeline::build_queue()
    {
        if (!queueDirty_)
        {
            return;
        }

        drawList_.build_queue(queue_);
        queueDirty_ = false;
    }

    void RenderPipeline::render(Renderer& renderer)
    {
        build_queue();

        renderer.render(queue_);
        stats_ = renderer.stats();
    }

    bool RenderPipeline::empty() const
    {
        return drawList_.empty();
    }

    std::size_t RenderPipeline::object_count() const
    {
        return drawList_.object_count();
    }

    std::size_t RenderPipeline::command_count() const
    {
        return queue_.command_count();
    }

    const DrawList& RenderPipeline::draw_list() const
    {
        return drawList_;
    }

    const RenderQueue& RenderPipeline::queue() const
    {
        return queue_;
    }

    const RenderStats& RenderPipeline::stats() const
    {
        return stats_;
    }
}