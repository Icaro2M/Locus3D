#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <cstddef>
#include <string>
#include <vector>
#include <utility>

namespace locus::kernel::geometry
{
    enum class TopologyIssueSeverity
    {
        Info,
        Warning,
        Error
    };

    enum class TopologyIssueCode
    {
        InvalidVertexReference,
        InvalidEdgeReference,
        InvalidLoopReference,
        InvalidFaceReference,
        DegenerateEdge,
        BrokenFaceCycle,
        BrokenRadialCycle,
        FaceTooSmall,
        LoopFaceMismatch,
        LoopEdgeMismatch,
        EdgeEndpointMismatch,
        NonManifoldEdge
    };

    struct TopologyIssue
    {
        TopologyIssueSeverity severity = TopologyIssueSeverity::Error;
        TopologyIssueCode code = TopologyIssueCode::InvalidVertexReference;
        LEMElementType elementType = LEMElementType::Vertex;
        Id id{};
        std::string message{};
    };

    struct TopologyValidationReport
    {
        std::vector<TopologyIssue> issues{};

        [[nodiscard]] bool valid() const
        {
            for (const TopologyIssue& issue : issues)
            {
                if (issue.severity == TopologyIssueSeverity::Error)
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] bool has_issues() const
        {
            return !issues.empty();
        }

        [[nodiscard]] std::size_t error_count() const
        {
            std::size_t count = 0;

            for (const TopologyIssue& issue : issues)
            {
                if (issue.severity == TopologyIssueSeverity::Error)
                {
                    ++count;
                }
            }

            return count;
        }

        [[nodiscard]] std::size_t warning_count() const
        {
            std::size_t count = 0;

            for (const TopologyIssue& issue : issues)
            {
                if (issue.severity == TopologyIssueSeverity::Warning)
                {
                    ++count;
                }
            }

            return count;
        }
    };

    class TopologyValidator
    {
    public:
        [[nodiscard]] static TopologyValidationReport validate(const LEM& mesh)
        {
            TopologyValidationReport report{};

            validate_vertices(mesh, report);
            validate_edges(mesh, report);
            validate_faces(mesh, report);
            validate_loops(mesh, report);

            return report;
        }

    private:
        static void validate_vertices(const LEM& mesh, TopologyValidationReport& report)
        {
            for (VertexHandle vertexHandle : TopologyTraversal::vertices(mesh))
            {
                const Vertex& vertex = mesh.vertex(vertexHandle);

                if (vertex.edge.is_valid() && !mesh.is_valid(vertex.edge))
                {
                    add_issue(
                        report,
                        TopologyIssueCode::InvalidEdgeReference,
                        LEMElementType::Vertex,
                        vertexHandle.id,
                        "Vertex references an invalid edge."
                    );
                }
            }
        }

        static void validate_edges(const LEM& mesh, TopologyValidationReport& report)
        {
            for (EdgeHandle edgeHandle : TopologyTraversal::edges(mesh))
            {
                const Edge& edge = mesh.edge(edgeHandle);

                if (!mesh.is_valid(edge.vertexA))
                {
                    add_issue(
                        report,
                        TopologyIssueCode::InvalidVertexReference,
                        LEMElementType::Edge,
                        edgeHandle.id,
                        "Edge references an invalid first vertex."
                    );
                }

                if (!mesh.is_valid(edge.vertexB))
                {
                    add_issue(
                        report,
                        TopologyIssueCode::InvalidVertexReference,
                        LEMElementType::Edge,
                        edgeHandle.id,
                        "Edge references an invalid second vertex."
                    );
                }

                if (edge.vertexA == edge.vertexB)
                {
                    add_issue(
                        report,
                        TopologyIssueCode::DegenerateEdge,
                        LEMElementType::Edge,
                        edgeHandle.id,
                        "Edge references the same vertex twice."
                    );
                }

                if (edge.loop.is_valid() && !mesh.is_valid(edge.loop))
                {
                    add_issue(
                        report,
                        TopologyIssueCode::InvalidLoopReference,
                        LEMElementType::Edge,
                        edgeHandle.id,
                        "Edge references an invalid loop."
                    );
                }

                validate_radial_cycle(mesh, edgeHandle, report);

                const std::vector<LoopHandle> edgeLoops = TopologyTraversal::edge_loops(mesh, edgeHandle);
                if (edgeLoops.size() > 2)
                {
                    add_issue(
                        report,
                        TopologyIssueCode::NonManifoldEdge,
                        LEMElementType::Edge,
                        edgeHandle.id,
                        "Edge has more than two radial loops."
                    );
                }
            }
        }

        static void validate_faces(const LEM& mesh, TopologyValidationReport& report)
        {
            for (FaceHandle faceHandle : TopologyTraversal::faces(mesh))
            {
                const Face& face = mesh.face(faceHandle);

                if (!mesh.is_valid(face.loop))
                {
                    add_issue(
                        report,
                        TopologyIssueCode::InvalidLoopReference,
                        LEMElementType::Face,
                        faceHandle.id,
                        "Face references an invalid boundary loop."
                    );

                    continue;
                }

                const std::vector<LoopHandle> loops = mesh.face_loops(faceHandle);
                if (loops.size() < 3)
                {
                    add_issue(
                        report,
                        TopologyIssueCode::FaceTooSmall,
                        LEMElementType::Face,
                        faceHandle.id,
                        "Face has fewer than three boundary loops."
                    );
                }

                validate_face_cycle(mesh, faceHandle, report);
            }
        }

        static void validate_loops(const LEM& mesh, TopologyValidationReport& report)
        {
            for (LoopHandle loopHandle : TopologyTraversal::loops(mesh))
            {
                const Loop& loop = mesh.loop(loopHandle);

                if (!mesh.is_valid(loop.vertex))
                {
                    add_issue(
                        report,
                        TopologyIssueCode::InvalidVertexReference,
                        LEMElementType::Loop,
                        loopHandle.id,
                        "Loop references an invalid vertex."
                    );
                }

                if (!mesh.is_valid(loop.edge))
                {
                    add_issue(
                        report,
                        TopologyIssueCode::InvalidEdgeReference,
                        LEMElementType::Loop,
                        loopHandle.id,
                        "Loop references an invalid edge."
                    );
                }

                if (!mesh.is_valid(loop.face))
                {
                    add_issue(
                        report,
                        TopologyIssueCode::InvalidFaceReference,
                        LEMElementType::Loop,
                        loopHandle.id,
                        "Loop references an invalid face."
                    );
                }

                if (!mesh.is_valid(loop.next))
                {
                    add_issue(
                        report,
                        TopologyIssueCode::InvalidLoopReference,
                        LEMElementType::Loop,
                        loopHandle.id,
                        "Loop references an invalid next loop."
                    );
                }

                if (!mesh.is_valid(loop.previous))
                {
                    add_issue(
                        report,
                        TopologyIssueCode::InvalidLoopReference,
                        LEMElementType::Loop,
                        loopHandle.id,
                        "Loop references an invalid previous loop."
                    );
                }

                if (!mesh.is_valid(loop.radialNext))
                {
                    add_issue(
                        report,
                        TopologyIssueCode::InvalidLoopReference,
                        LEMElementType::Loop,
                        loopHandle.id,
                        "Loop references an invalid radial next loop."
                    );
                }

                if (!mesh.is_valid(loop.radialPrevious))
                {
                    add_issue(
                        report,
                        TopologyIssueCode::InvalidLoopReference,
                        LEMElementType::Loop,
                        loopHandle.id,
                        "Loop references an invalid radial previous loop."
                    );
                }

                validate_loop_edge_match(mesh, loopHandle, report);
            }
        }

        static void validate_face_cycle(const LEM& mesh, FaceHandle faceHandle, TopologyValidationReport& report)
        {
            const std::vector<LoopHandle> loops = mesh.face_loops(faceHandle);

            for (LoopHandle loopHandle : loops)
            {
                const Loop& loop = mesh.loop(loopHandle);

                if (loop.face != faceHandle)
                {
                    add_issue(
                        report,
                        TopologyIssueCode::LoopFaceMismatch,
                        LEMElementType::Loop,
                        loopHandle.id,
                        "Loop face does not match the owning face cycle."
                    );
                }

                if (mesh.is_valid(loop.next))
                {
                    const Loop& next = mesh.loop(loop.next);
                    if (next.previous != loopHandle)
                    {
                        add_issue(
                            report,
                            TopologyIssueCode::BrokenFaceCycle,
                            LEMElementType::Loop,
                            loopHandle.id,
                            "Loop next and previous links are inconsistent."
                        );
                    }
                }

                if (mesh.is_valid(loop.previous))
                {
                    const Loop& previous = mesh.loop(loop.previous);
                    if (previous.next != loopHandle)
                    {
                        add_issue(
                            report,
                            TopologyIssueCode::BrokenFaceCycle,
                            LEMElementType::Loop,
                            loopHandle.id,
                            "Loop previous and next links are inconsistent."
                        );
                    }
                }
            }
        }

        static void validate_radial_cycle(const LEM& mesh, EdgeHandle edgeHandle, TopologyValidationReport& report)
        {
            const Edge& edge = mesh.edge(edgeHandle);

            if (edge.loop.is_invalid())
            {
                return;
            }

            if (!mesh.is_valid(edge.loop))
            {
                return;
            }

            const std::vector<LoopHandle> loops = TopologyTraversal::edge_loops(mesh, edgeHandle);

            for (LoopHandle loopHandle : loops)
            {
                const Loop& loop = mesh.loop(loopHandle);

                if (loop.edge != edgeHandle)
                {
                    add_issue(
                        report,
                        TopologyIssueCode::LoopEdgeMismatch,
                        LEMElementType::Loop,
                        loopHandle.id,
                        "Loop edge does not match the owning radial cycle."
                    );
                }

                if (mesh.is_valid(loop.radialNext))
                {
                    const Loop& next = mesh.loop(loop.radialNext);
                    if (next.radialPrevious != loopHandle)
                    {
                        add_issue(
                            report,
                            TopologyIssueCode::BrokenRadialCycle,
                            LEMElementType::Loop,
                            loopHandle.id,
                            "Loop radial next and previous links are inconsistent."
                        );
                    }
                }

                if (mesh.is_valid(loop.radialPrevious))
                {
                    const Loop& previous = mesh.loop(loop.radialPrevious);
                    if (previous.radialNext != loopHandle)
                    {
                        add_issue(
                            report,
                            TopologyIssueCode::BrokenRadialCycle,
                            LEMElementType::Loop,
                            loopHandle.id,
                            "Loop radial previous and next links are inconsistent."
                        );
                    }
                }
            }
        }

        static void validate_loop_edge_match(const LEM& mesh, LoopHandle loopHandle, TopologyValidationReport& report)
        {
            const Loop& loop = mesh.loop(loopHandle);

            if (!mesh.is_valid(loop.edge) || !mesh.is_valid(loop.vertex))
            {
                return;
            }

            const Edge& edge = mesh.edge(loop.edge);
            if (loop.vertex != edge.vertexA && loop.vertex != edge.vertexB)
            {
                add_issue(
                    report,
                    TopologyIssueCode::EdgeEndpointMismatch,
                    LEMElementType::Loop,
                    loopHandle.id,
                    "Loop vertex is not one of its edge endpoints."
                );
            }
        }

        static void add_issue(
            TopologyValidationReport& report,
            TopologyIssueCode code,
            LEMElementType elementType,
            Id id,
            std::string message,
            TopologyIssueSeverity severity = TopologyIssueSeverity::Error)
        {
            report.issues.push_back(TopologyIssue{
                severity,
                code,
                elementType,
                id,
                std::move(message)
                });
        }
    };
}