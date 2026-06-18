#include "kernel/modeling/operations/transform/TransformOp.h"

#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/glm.hpp>

namespace locus::kernel::modeling {

    TransformOp::TransformOp(const glm::mat4& transform)
        : transform_(transform)
    {
    }

    TransformOp::TransformOp(std::vector<geometry::VertexHandle> vertices, const glm::mat4& transform)
        : transform_(transform)
        , vertices_(std::move(vertices))
    {
    }

    std::string_view TransformOp::name() const
    {
        return "TransformOp";
    }

    void TransformOp::set_transform(const glm::mat4& transform)
    {
        transform_ = transform;
    }

    const glm::mat4& TransformOp::transform() const
    {
        return transform_;
    }

    void TransformOp::set_target(TransformTarget target)
    {
        target_ = target;
    }

    TransformTarget TransformOp::target() const
    {
        return target_;
    }

    void TransformOp::set_vertices(std::vector<geometry::VertexHandle> vertices)
    {
        vertices_ = std::move(vertices);
    }

    const std::vector<geometry::VertexHandle>& TransformOp::vertices() const
    {
        return vertices_;
    }

    void TransformOp::clear_vertices()
    {
        vertices_.clear();
    }

    OperationResult TransformOp::execute_impl(OperationContext& context)
    {
        geometry::LEM& mesh = context.editable_mesh();
        geometry::LEMEditor editor(mesh);

        const std::vector<geometry::VertexHandle> targets = collect_vertices(mesh);

        if (targets.empty()) {
            return OperationResult::no_change("Transform operation has no valid target vertices.");
        }

        std::size_t changedCount = 0;

        for (geometry::VertexHandle vertexHandle : targets) {
            if (!mesh.is_valid(vertexHandle)) {
                continue;
            }

            const glm::vec3 position = mesh.vertex(vertexHandle).position;
            const glm::vec4 transformed = transform_ * glm::vec4{ position, 1.0f };

            if (editor.set_vertex_position(vertexHandle, glm::vec3{ transformed })) {
                ++changedCount;
            }
        }

        if (changedCount == 0) {
            return OperationResult::no_change("Transform operation did not modify any vertex.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::VertexHandle> TransformOp::collect_vertices(const geometry::LEM& mesh) const
    {
        if (!vertices_.empty()) {
            std::vector<geometry::VertexHandle> result;
            result.reserve(vertices_.size());

            for (geometry::VertexHandle vertexHandle : vertices_) {
                if (mesh.is_valid(vertexHandle)) {
                    result.push_back(vertexHandle);
                }
            }

            return result;
        }

        std::vector<geometry::VertexHandle> result;

        for (geometry::VertexHandle vertexHandle : geometry::TopologyTraversal::vertices(mesh)) {
            if (!mesh.is_valid(vertexHandle)) {
                continue;
            }

            if (target_ == TransformTarget::SelectedVertices && !mesh.vertex(vertexHandle).selected) {
                continue;
            }

            result.push_back(vertexHandle);
        }

        return result;
    }

}