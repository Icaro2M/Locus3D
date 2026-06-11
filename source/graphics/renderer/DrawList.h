#pragma once

#include "graphics/renderer/RenderQueue.h"
#include "graphics/scene/RenderLayer.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"

#include <cstddef>
#include <vector>

namespace locus::graphics
{
    class DrawList
    {
    public:
        using ObjectList = std::vector<RenderObject>;

        DrawList() = default;
        ~DrawList() = default;

        DrawList(const DrawList&) = default;
        DrawList& operator=(const DrawList&) = default;

        DrawList(DrawList&&) noexcept = default;
        DrawList& operator=(DrawList&&) noexcept = default;

        void clear();
        void reserve(std::size_t count);

        RenderObject& add_object(RenderObject object);
        void add_objects(const RenderScene& scene);
        void add_objects(const ObjectList& objects);

        void append_to_scene(RenderScene& scene) const;
        [[nodiscard]] RenderScene to_scene() const;

        [[nodiscard]] RenderQueue build_queue() const;
        void build_queue(RenderQueue& queue) const;

        [[nodiscard]] ObjectList objects_in_layer(RenderLayer layer) const;

        [[nodiscard]] bool empty() const;
        [[nodiscard]] std::size_t object_count() const;
        [[nodiscard]] const ObjectList& objects() const;

    private:
        ObjectList objects_;
    };
}