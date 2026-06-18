#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/geometry/mesh/LEMTypes.h"

#include <vector>

namespace locus::kernel::geometry {

enum class LEMChangeType {
    VertexAdded,
    EdgeAdded,
    LoopAdded,
    FaceAdded,
    VertexModified,
    EdgeModified,
    LoopModified,
    FaceModified,
    SelectionChanged,
    VisibilityChanged,
    NormalsChanged,
    MeshCleared
};

struct LEMChange {
    LEMChangeType type = LEMChangeType::VertexModified;
    LEMElementType elementType = LEMElementType::Vertex;
    Id id{};
};

class LEMDiff {
public:
    void record(LEMChangeType type, VertexHandle handle)
    {
        record(type, LEMElementType::Vertex, handle.id);
    }

    void record(LEMChangeType type, EdgeHandle handle)
    {
        record(type, LEMElementType::Edge, handle.id);
    }

    void record(LEMChangeType type, LoopHandle handle)
    {
        record(type, LEMElementType::Loop, handle.id);
    }

    void record(LEMChangeType type, FaceHandle handle)
    {
        record(type, LEMElementType::Face, handle.id);
    }

    void record(LEMChangeType type, LEMElementType elementType, Id id)
    {
        changes_.push_back(LEMChange{ type, elementType, id });
    }

    void append(const LEMChange& change)
    {
        changes_.push_back(change);
    }

    void merge(const LEMDiff& diff)
    {
        changes_.insert(changes_.end(), diff.changes_.begin(), diff.changes_.end());
    }

    void clear()
    {
        changes_.clear();
    }

    [[nodiscard]] bool empty() const
    {
        return changes_.empty();
    }

    [[nodiscard]] std::size_t size() const
    {
        return changes_.size();
    }

    [[nodiscard]] const std::vector<LEMChange>& changes() const
    {
        return changes_;
    }

private:
    std::vector<LEMChange> changes_{};
};

}