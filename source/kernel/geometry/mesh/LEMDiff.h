/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/geometry/mesh/LEMTypes.h"

#include <cstddef>
#include <vector>

namespace locus::kernel::geometry {

/**
 * @brief Types of mesh changes recorded by editable mesh operations.
 */
enum class LEMChangeType {
    /**
     * @brief A vertex was added to the mesh.
     */
    VertexAdded,
    /**
     * @brief An edge was added to the mesh.
     */
    EdgeAdded,
    /**
     * @brief A loop was added to the mesh.
     */
    LoopAdded,
    /**
     * @brief A face was added to the mesh.
     */
    FaceAdded,
    /**
     * @brief Stored vertex data changed.
     */
    VertexModified,
    /**
     * @brief Stored edge data changed.
     */
    EdgeModified,
    /**
     * @brief Stored loop data changed.
     */
    LoopModified,
    /**
     * @brief Stored face data changed.
     */
    FaceModified,
    /**
     * @brief Selection flags changed.
     */
    SelectionChanged,
    /**
     * @brief Visibility flags changed.
     */
    VisibilityChanged,
    /**
     * @brief Face or derived normal data changed.
     */
    NormalsChanged,
    /**
     * @brief The mesh contents were cleared.
     */
    MeshCleared
};

/**
 * @brief Single change event for a Locus Editable Mesh element.
 */
struct LEMChange {
    /**
     * @brief Kind of modification that happened.
     */
    LEMChangeType type = LEMChangeType::VertexModified;
    /**
     * @brief Element category affected by the change.
     */
    LEMElementType elementType = LEMElementType::Vertex;
    /**
     * @brief Element identifier affected by the change.
     */
    Id id{};
};

/**
 * @brief Ordered collection of mesh changes produced by editing operations.
 */
class LEMDiff {
public:
    /**
     * @brief Records a vertex change.
     *
     * @param type Change type.
     * @param handle Vertex affected by the change.
     */
    void record(LEMChangeType type, VertexHandle handle)
    {
        record(type, LEMElementType::Vertex, handle.id);
    }

    /**
     * @brief Records an edge change.
     *
     * @param type Change type.
     * @param handle Edge affected by the change.
     */
    void record(LEMChangeType type, EdgeHandle handle)
    {
        record(type, LEMElementType::Edge, handle.id);
    }

    /**
     * @brief Records a loop change.
     *
     * @param type Change type.
     * @param handle Loop affected by the change.
     */
    void record(LEMChangeType type, LoopHandle handle)
    {
        record(type, LEMElementType::Loop, handle.id);
    }

    /**
     * @brief Records a face change.
     *
     * @param type Change type.
     * @param handle Face affected by the change.
     */
    void record(LEMChangeType type, FaceHandle handle)
    {
        record(type, LEMElementType::Face, handle.id);
    }

    /**
     * @brief Records a change for an arbitrary mesh element type.
     *
     * @param type Change type.
     * @param elementType Element category affected by the change.
     * @param id Element identifier affected by the change.
     */
    void record(LEMChangeType type, LEMElementType elementType, Id id)
    {
        changes_.push_back(LEMChange{ type, elementType, id });
    }

    /**
     * @brief Appends a prebuilt change event.
     *
     * @param change Change event to append.
     */
    void append(const LEMChange& change)
    {
        changes_.push_back(change);
    }

    /**
     * @brief Appends all change events from another diff.
     *
     * @param diff Diff whose changes are appended in order.
     */
    void merge(const LEMDiff& diff)
    {
        changes_.insert(changes_.end(), diff.changes_.begin(), diff.changes_.end());
    }

    /**
     * @brief Removes all recorded changes.
     */
    void clear()
    {
        changes_.clear();
    }

    /**
     * @brief Checks whether no changes are recorded.
     *
     * @return True when the diff is empty.
     */
    [[nodiscard]] bool empty() const
    {
        return changes_.empty();
    }

    /**
     * @brief Returns the number of recorded changes.
     *
     * @return Change event count.
     */
    [[nodiscard]] std::size_t size() const
    {
        return changes_.size();
    }

    /**
     * @brief Returns all recorded changes.
     *
     * @return Read-only change event list.
     */
    [[nodiscard]] const std::vector<LEMChange>& changes() const
    {
        return changes_;
    }

private:
    std::vector<LEMChange> changes_{};
};

}
