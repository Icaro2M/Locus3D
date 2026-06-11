#include "graphics/renderer/DrawList.h"

#include <utility>

namespace locus::graphics
{
    void DrawList::clear()
    {
        objects_.clear();
    }

    void DrawList::reserve(std::size_t count)
    {
        objects_.reserve(count);
    }

    RenderObject& DrawList::add_object(RenderObject object)
    {
        objects_.push_back(std::move(object));
        return objects_.back();
    }

    void DrawList::add_objects(const RenderScene& scene)
    {
        reserve(objects_.size() + scene.object_count());

        for (const RenderObject& object : scene.objects())
        {
            objects_.push_back(object);
        }
    }

    void DrawList::add_objects(const ObjectList& objects)
    {
        reserve(objects_.size() + objects.size());

        for (const RenderObject& object : objects)
        {
            objects_.push_back(object);
        }
    }

    void DrawList::append_to_scene(RenderScene& scene) const
    {
        for (const RenderObject& object : objects_)
        {
            scene.add_object(object);
        }
    }

    RenderScene DrawList::to_scene() const
    {
        RenderScene scene;
        scene.reserve(objects_.size());
        append_to_scene(scene);

        return scene;
    }

    RenderQueue DrawList::build_queue() const
    {
        RenderQueue queue;
        build_queue(queue);

        return queue;
    }

    void DrawList::build_queue(RenderQueue& queue) const
    {
        queue.clear();
        queue.reserve(objects_.size());

        for (const RenderObject& object : objects_)
        {
            queue.add_object(object);
        }

        queue.sort();
    }

    DrawList::ObjectList DrawList::objects_in_layer(RenderLayer layer) const
    {
        ObjectList result;

        for (const RenderObject& object : objects_)
        {
            if (object.layer == layer)
            {
                result.push_back(object);
            }
        }

        return result;
    }

    bool DrawList::empty() const
    {
        return objects_.empty();
    }

    std::size_t DrawList::object_count() const
    {
        return objects_.size();
    }

    const DrawList::ObjectList& DrawList::objects() const
    {
        return objects_;
    }
}