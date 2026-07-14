/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/scene/RenderScene.h"

#include <utility>

namespace locus::graphics
{
    RenderObject& RenderScene::add_object(RenderObject object)
    {
        objects_.push_back(std::move(object));
        return objects_.back();
    }

    void RenderScene::clear()
    {
        objects_.clear();
    }

    void RenderScene::reserve(std::size_t count)
    {
        objects_.reserve(count);
    }

    bool RenderScene::empty() const
    {
        return objects_.empty();
    }

    std::size_t RenderScene::object_count() const
    {
        return objects_.size();
    }

    RenderScene::ObjectList& RenderScene::objects() {
        return objects_;
    }

    const RenderScene::ObjectList& RenderScene::objects() const
    {
        return objects_;
    }

    RenderScene::ObjectList RenderScene::objects_in_layer(RenderLayer layer) const
    {
        ObjectList result;

        // Preserve scene order so layer-filtered draws remain deterministic.
        for (const RenderObject& object : objects_)
        {
            if (object.layer == layer)
            {
                result.push_back(object);
            }
        }

        return result;
    }
}
