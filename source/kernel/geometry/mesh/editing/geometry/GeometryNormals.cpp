/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/geometry/GeometryNormals.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/geometric.hpp>

namespace locus::kernel::geometry {

    GeometryNormals::GeometryNormals(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff)
    {
    }

    void GeometryNormals::rebuild_face_normals()
    {
        NormalBuilder::rebuild_face_normals(mesh_);

        for (FaceHandle faceHandle : TopologyTraversal::faces(mesh_)) {
            diff_.record(LEMChangeType::NormalsChanged, faceHandle);
        }
    }

    void GeometryNormals::rebuild_normals_around_vertex(VertexHandle vertex)
    {
        if (!mesh_.is_valid(vertex)) {
            return;
        }

        for (FaceHandle faceHandle : TopologyTraversal::vertex_faces(mesh_, vertex)) {
            rebuild_normals_around_face(faceHandle);
        }
    }

    bool GeometryNormals::rebuild_normals_around_face(FaceHandle face)
    {
        if (!mesh_.is_valid(face)) {
            return false;
        }

        mesh_.face(face).normal = NormalBuilder::face_normal(mesh_, face);
        diff_.record(LEMChangeType::NormalsChanged, face);
        return true;
    }

    glm::vec3 GeometryNormals::vertex_normal(VertexHandle vertex) const
    {
        if (!mesh_.is_valid(vertex)) {
            return glm::vec3{ 0.0f, 1.0f, 0.0f };
        }

        glm::vec3 normal{ 0.0f, 0.0f, 0.0f };

        for (FaceHandle faceHandle : TopologyTraversal::vertex_faces(mesh_, vertex)) {
            if (!mesh_.is_valid(faceHandle)) {
                continue;
            }

            normal += NormalBuilder::face_normal(mesh_, faceHandle);
        }

        const float length = glm::length(normal);
        if (length <= 0.000001f) {
            return glm::vec3{ 0.0f, 1.0f, 0.0f };
        }

        return normal / length;
    }

}