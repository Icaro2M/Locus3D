/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/transform/RandomizeOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <cmath>
#include <glm/vec3.hpp>
#include <random>
#include <utility>

namespace locus::kernel::modeling {

    RandomizeOp::RandomizeOp(float strength)
        : strength_(strength)
    {
    }

    RandomizeOp::RandomizeOp(
        std::vector<geometry::VertexHandle> vertices,
        float strength)
        : vertices_(std::move(vertices))
        , strength_(strength)
    {
    }

    RandomizeOp RandomizeOp::selected(float strength)
    {
        RandomizeOp op(strength);
        op.set_target(RandomizeTarget::SelectedVertices);
        return op;
    }

    std::string_view RandomizeOp::name() const
    {
        return "RandomizeOp";
    }

    void RandomizeOp::set_strength(float strength)
    {
        strength_ = strength;
    }

    float RandomizeOp::strength() const
    {
        return strength_;
    }

    void RandomizeOp::set_seed(std::uint32_t seed)
    {
        seed_ = seed;
    }

    std::uint32_t RandomizeOp::seed() const
    {
        return seed_;
    }

    void RandomizeOp::set_target(RandomizeTarget target)
    {
        target_ = target;
    }

    RandomizeTarget RandomizeOp::target() const
    {
        return target_;
    }

    void RandomizeOp::set_vertices(std::vector<geometry::VertexHandle> vertices)
    {
        vertices_ = std::move(vertices);
    }

    const std::vector<geometry::VertexHandle>& RandomizeOp::vertices() const
    {
        return vertices_;
    }

    void RandomizeOp::clear_vertices()
    {
        vertices_.clear();
    }

    OperationResult RandomizeOp::execute_impl(OperationContext& context)
    {
        geometry::LEM& mesh = context.editable_mesh();
        const std::vector<geometry::VertexHandle> targets = collect_vertices(mesh);

        if (targets.empty()) {
            return OperationResult::no_change(
                "Randomize operation has no valid target vertices.");
        }

        const float absoluteStrength = std::abs(strength_);

        if (absoluteStrength <= 0.0f) {
            return OperationResult::no_change(
                "Randomize operation has zero strength.");
        }

        geometry::LEMEditor editor(mesh);
        std::mt19937 generator(seed_);
        std::uniform_real_distribution<float> distribution(
            -absoluteStrength,
            absoluteStrength);

        std::size_t changedCount = 0;

        for (geometry::VertexHandle vertex : targets) {
            if (!mesh.is_valid(vertex)) {
                continue;
            }

            const glm::vec3 position = mesh.vertex(vertex).position;
            const glm::vec3 offset{
                distribution(generator),
                distribution(generator),
                distribution(generator)
            };

            if (editor.set_vertex_position(vertex, position + offset)) {
                ++changedCount;
            }
        }

        if (changedCount == 0) {
            return OperationResult::no_change(
                "Randomize operation did not modify any vertex.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::VertexHandle> RandomizeOp::collect_vertices(
        const geometry::LEM& mesh) const
    {
        if (!vertices_.empty()) {
            std::vector<geometry::VertexHandle> result;
            result.reserve(vertices_.size());

            for (geometry::VertexHandle vertex : vertices_) {
                if (mesh.is_valid(vertex)) {
                    result.push_back(vertex);
                }
            }

            return result;
        }

        std::vector<geometry::VertexHandle> result;

        for (geometry::VertexHandle vertex : geometry::TopologyTraversal::vertices(mesh)) {
            if (!mesh.is_valid(vertex)) {
                continue;
            }

            if (target_ == RandomizeTarget::SelectedVertices &&
                !mesh.vertex(vertex).selected) {
                continue;
            }

            result.push_back(vertex);
        }

        return result;
    }

}