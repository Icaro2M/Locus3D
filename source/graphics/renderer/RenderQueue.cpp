/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/renderer/RenderQueue.h"

#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"

#include <algorithm>

namespace locus::graphics
{
    void RenderQueue::clear()
    {
        commands_.clear();
        nextSequence_ = 0;
    }

    void RenderQueue::reserve(std::size_t count)
    {
        commands_.reserve(count);
    }

    void RenderQueue::add_object(const RenderObject& object)
    {
        RenderCommand command;
        command.object = &object;
        command.layer = object.layer;
        command.priority = priority_for_layer(object.layer);
        command.sequence = nextSequence_;

        ++nextSequence_;

        commands_.push_back(command);
    }

    void RenderQueue::build_from_scene(const RenderScene& scene)
    {
        clear();
        reserve(scene.object_count());

        for (const RenderObject& object : scene.objects())
        {
            add_object(object);
        }
    }

    void RenderQueue::sort()
    {
        // Keep equal-priority objects in scene order so draw output remains deterministic.
        std::stable_sort(
            commands_.begin(),
            commands_.end(),
            [](const RenderCommand& a, const RenderCommand& b)
            {
                if (a.priority != b.priority)
                {
                    return a.priority < b.priority;
                }

                return a.sequence < b.sequence;
            }
        );
    }

    bool RenderQueue::empty() const
    {
        return commands_.empty();
    }

    std::size_t RenderQueue::command_count() const
    {
        return commands_.size();
    }

    const RenderQueue::CommandList& RenderQueue::commands() const
    {
        return commands_;
    }

    u32 RenderQueue::priority_for_layer(RenderLayer layer)
    {
        switch (layer)
        {
        case RenderLayer::Grid:
            return 0;

        case RenderLayer::Default:
            return 100;

        case RenderLayer::Overlay:
            return 200;

        case RenderLayer::Gizmo:
            return 300;

        case RenderLayer::Debug:
            return 400;
        }

        return 100;
    }
}
