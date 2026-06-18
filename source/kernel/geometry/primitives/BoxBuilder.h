/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/primitives/PrimitiveParameters.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/glm.hpp>

#include <array>
#include <vector>

namespace locus::kernel::geometry {

/**
 * @brief Builds an axis-aligned box as editable LEM topology.
 */
class BoxBuilder {
public:
    /**
     * @brief Creates a new mesh containing a box primitive.
     *
     * @param parameters Box creation parameters.
     * @return Mesh containing the created box, or an empty mesh for invalid parameters.
     */
    [[nodiscard]] static LEM build(const BoxParameters& parameters = {})
    {
        LEM mesh;
        build_into(mesh, parameters);
        return mesh;
    }

    /**
     * @brief Appends a box primitive to an existing mesh.
     *
     * @param mesh Mesh that receives the new topology.
     * @param parameters Box creation parameters.
     * @return Created element handles, recorded diff, and success state.
     */
    [[nodiscard]] static PrimitiveBuildResult build_into(LEM& mesh, const BoxParameters& parameters = {})
    {
        PrimitiveBuildResult result;

        if (!parameters.is_valid()) {
            return result;
        }

        LEMEditor editor(mesh);

        const std::size_t edgeCount = mesh.edge_count();

        const glm::vec3 half = parameters.size * 0.5f;
        const glm::vec3 min = parameters.center - half;
        const glm::vec3 max = parameters.center + half;

        const VertexHandle v000 = editor.add_vertex({ min.x, min.y, min.z });
        const VertexHandle v100 = editor.add_vertex({ max.x, min.y, min.z });
        const VertexHandle v110 = editor.add_vertex({ max.x, max.y, min.z });
        const VertexHandle v010 = editor.add_vertex({ min.x, max.y, min.z });
        const VertexHandle v001 = editor.add_vertex({ min.x, min.y, max.z });
        const VertexHandle v101 = editor.add_vertex({ max.x, min.y, max.z });
        const VertexHandle v111 = editor.add_vertex({ max.x, max.y, max.z });
        const VertexHandle v011 = editor.add_vertex({ min.x, max.y, max.z });

        result.vertices = {
            v000,
            v100,
            v110,
            v010,
            v001,
            v101,
            v111,
            v011
        };

        const std::array<std::array<VertexHandle, 4>, 6> faces = {
            std::array<VertexHandle, 4>{ v000, v100, v101, v001 },
            std::array<VertexHandle, 4>{ v010, v011, v111, v110 },
            std::array<VertexHandle, 4>{ v001, v101, v111, v011 },
            std::array<VertexHandle, 4>{ v000, v010, v110, v100 },
            std::array<VertexHandle, 4>{ v100, v110, v111, v101 },
            std::array<VertexHandle, 4>{ v000, v001, v011, v010 }
        };

        result.faces.reserve(faces.size());

        for (const auto& faceVertices : faces) {
            FaceHandle faceHandle = editor.add_face({
                faceVertices[0],
                faceVertices[1],
                faceVertices[2],
                faceVertices[3]
            });

            if (!mesh.is_valid(faceHandle)) {
                result.diff = editor.take_diff();
                result.success = false;
                return result;
            }

            if (parameters.selectCreatedFaces) {
                editor.set_selected(faceHandle, true);
            }

            result.faces.push_back(faceHandle);
        }

        for (std::size_t index = edgeCount; index < mesh.edge_count(); ++index) {
            EdgeHandle edgeHandle(static_cast<IdValue>(index));

            if (mesh.is_valid(edgeHandle)) {
                result.edges.push_back(edgeHandle);
            }
        }

        result.diff = editor.take_diff();
        result.success = result.vertices.size() == 8 && result.edges.size() == 12 && result.faces.size() == 6;

        return result;
    }
};

}
