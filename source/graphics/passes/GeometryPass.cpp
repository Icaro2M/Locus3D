#include "graphics/passes/GeometryPass.h"

#include "graphics/renderer/Renderer.h"
#include "graphics/scene/RenderLayer.h"
#include "graphics/scene/RenderScene.h"

namespace locus::graphics
{
    const char* GeometryPass::name() const
    {
        return "GeometryPass";
    }

    GraphicsResult<void> GeometryPass::execute(RenderPassContext& context)
    {
        if (context.renderer == nullptr)
        {
            return GraphicsError{
                GraphicsErrorCode::InvalidOperation,
                "GeometryPass requires a renderer."
            };
        }

        if (context.scene == nullptr)
        {
            return GraphicsError{
                GraphicsErrorCode::InvalidOperation,
                "GeometryPass requires a render scene."
            };
        }

        queue_.clear();

        const auto objects = context.scene->objects_in_layer(RenderLayer::Default);
        queue_.reserve(objects.size());

        for (const RenderObject& object : objects)
        {
            queue_.add_object(object);
        }

        queue_.sort();

        context.renderer->render(queue_);

        return {};
    }
}