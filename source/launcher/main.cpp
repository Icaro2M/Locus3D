/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/OverlayRenderAdapter.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/MeshSelection.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace {

    using locus::editor::MeshNode;
    using locus::editor::MeshSelection;
    using locus::editor::OverlayGeometry;
    using locus::editor::OverlayPrimitiveGroup;
    using locus::editor::OverlayPrimitiveRole;
    using locus::editor::OverlayRenderAdapter;
    using locus::editor::OverlayRenderOptions;
    using locus::editor::OverlayRenderResult;
    using locus::editor::SceneNodeId;

    using locus::graphics::ColorRGBA;
    using locus::graphics::PrimitiveTopology;

    using locus::kernel::geometry::EdgeHandle;
    using locus::kernel::geometry::FaceHandle;
    using locus::kernel::geometry::LEM;
    using locus::kernel::geometry::TopologyTraversal;
    using locus::kernel::geometry::VertexHandle;

    constexpr float FloatTolerance = 0.0001f;

    bool expect(
        const bool condition,
        const std::string& message
    ) {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return true;
        }

        std::cout << "[FAIL] " << message << '\n';
        return false;
    }

    bool nearly_equal(
        const float lhs,
        const float rhs,
        const float tolerance = FloatTolerance
    ) {
        return std::abs(lhs - rhs) <= tolerance;
    }

    bool color_equals(
        const ColorRGBA& lhs,
        const ColorRGBA& rhs,
        const float tolerance = FloatTolerance
    ) {
        return nearly_equal(lhs.r, rhs.r, tolerance)
            && nearly_equal(lhs.g, rhs.g, tolerance)
            && nearly_equal(lhs.b, rhs.b, tolerance)
            && nearly_equal(lhs.a, rhs.a, tolerance);
    }

    struct QuadFixture {
        MeshNode node{
            SceneNodeId{ 42 },
            "Overlay Quad"
        };

        VertexHandle vertex0{};
        VertexHandle vertex1{};
        VertexHandle vertex2{};
        VertexHandle vertex3{};

        EdgeHandle edge01{};
        EdgeHandle edge12{};
        EdgeHandle edge23{};
        EdgeHandle edge30{};

        FaceHandle face{};

        QuadFixture() {
            LEM& mesh = node.mesh();

            vertex0 = mesh.add_vertex(
                glm::vec3{ -1.0f, -1.0f, 0.0f }
            );

            vertex1 = mesh.add_vertex(
                glm::vec3{ 1.0f, -1.0f, 0.0f }
            );

            vertex2 = mesh.add_vertex(
                glm::vec3{ 1.0f, 1.0f, 0.0f }
            );

            vertex3 = mesh.add_vertex(
                glm::vec3{ -1.0f, 1.0f, 0.0f }
            );

            face = mesh.add_face({
                vertex0,
                vertex1,
                vertex2,
                vertex3
                });

            edge01 = mesh.find_edge(
                vertex0,
                vertex1
            );

            edge12 = mesh.find_edge(
                vertex1,
                vertex2
            );

            edge23 = mesh.find_edge(
                vertex2,
                vertex3
            );

            edge30 = mesh.find_edge(
                vertex3,
                vertex0
            );
        }

        QuadFixture(const QuadFixture&) = delete;
        QuadFixture& operator=(const QuadFixture&) = delete;
    };


    const OverlayPrimitiveGroup* find_group(
        const OverlayGeometry& geometry,
        const OverlayPrimitiveRole role
    ) {
        return geometry.find_group(role);
    }

    bool test_fixture_creation() {
        std::cout << "\n=== Quad fixture ===\n";

        bool ok = true;

        QuadFixture fixture;
        const LEM& mesh = fixture.node.mesh();

        ok &= expect(
            fixture.node.id() == SceneNodeId{ 42 },
            "MeshNode preserva SceneNodeId"
        );

        ok &= expect(
            mesh.vertex_count() == 4,
            "quad possui quatro vertices"
        );

        ok &= expect(
            TopologyTraversal::vertices(mesh).size() == 4,
            "quad possui quatro vertices ativos"
        );

        ok &= expect(
            TopologyTraversal::edges(mesh).size() == 4,
            "quad possui quatro arestas ativas"
        );

        ok &= expect(
            TopologyTraversal::faces(mesh).size() == 1,
            "quad possui uma face ativa"
        );

        ok &= expect(
            mesh.is_valid(fixture.face),
            "face do quad possui handle valido"
        );

        ok &= expect(
            mesh.is_valid(fixture.edge01)
            && mesh.is_valid(fixture.edge12)
            && mesh.is_valid(fixture.edge23)
            && mesh.is_valid(fixture.edge30),
            "arestas do quad possuem handles validos"
        );

        return ok;
    }

    bool test_complete_overlay() {
        std::cout << "\n=== Complete mesh overlay ===\n";

        bool ok = true;

        QuadFixture fixture;

        MeshSelection selection;
        selection.set_active_mesh(fixture.node.id());

        selection.add_vertex(fixture.vertex0);
        selection.add_edge(fixture.edge01);
        selection.add_face(fixture.face);

        selection.set_hovered_vertex(fixture.vertex1);
        selection.set_hovered_edge(fixture.edge12);
        selection.set_hovered_face(fixture.face);

        OverlayRenderResult result;

        const OverlayGeometry geometry =
            OverlayRenderAdapter::build_mesh_overlay(
                fixture.node,
                selection,
                {},
                &result
            );

        ok &= expect(
            geometry.nodeId == fixture.node.id(),
            "overlay preserva node id"
        );

        ok &= expect(
            geometry.has_geometry(),
            "overlay completo possui geometria"
        );

        ok &= expect(
            geometry.groups.size() == 7,
            "overlay completo gera sete grupos semanticos"
        );

        ok &= expect(
            result.groupCount == 7,
            "resultado informa sete grupos"
        );

        ok &= expect(
            result.visitedVertexCount == 4,
            "resultado informa quatro vertices visitados"
        );

        ok &= expect(
            result.visitedEdgeCount == 4,
            "resultado informa quatro arestas visitadas"
        );

        ok &= expect(
            result.visitedFaceCount == 1,
            "resultado informa uma face visitada"
        );

        ok &= expect(
            result.invalidHandleCount == 0,
            "overlay valido nao encontra handles invalidos"
        );

        const OverlayPrimitiveGroup* wireframe =
            find_group(
                geometry,
                OverlayPrimitiveRole::Wireframe
            );

        ok &= expect(
            wireframe != nullptr,
            "grupo de wireframe foi gerado"
        );

        if (wireframe) {
            ok &= expect(
                wireframe->mesh.topology
                == PrimitiveTopology::Lines,
                "wireframe usa topologia Lines"
            );

            ok &= expect(
                wireframe->mesh.vertices.size() == 8,
                "quatro arestas geram oito vertices de linha"
            );

            ok &= expect(
                wireframe->mesh.is_valid(),
                "mesh do wireframe e valida"
            );
        }

        const OverlayPrimitiveGroup* selectedVertices =
            find_group(
                geometry,
                OverlayPrimitiveRole::SelectedVertices
            );

        ok &= expect(
            selectedVertices != nullptr,
            "grupo de vertices selecionados foi gerado"
        );

        if (selectedVertices) {
            ok &= expect(
                selectedVertices->mesh.topology
                == PrimitiveTopology::Points,
                "vertices selecionados usam Points"
            );

            ok &= expect(
                selectedVertices->mesh.vertices.size() == 1,
                "um vertice selecionado gera um ponto"
            );
        }

        const OverlayPrimitiveGroup* selectedEdges =
            find_group(
                geometry,
                OverlayPrimitiveRole::SelectedEdges
            );

        ok &= expect(
            selectedEdges != nullptr,
            "grupo de arestas selecionadas foi gerado"
        );

        if (selectedEdges) {
            ok &= expect(
                selectedEdges->mesh.topology
                == PrimitiveTopology::Lines,
                "arestas selecionadas usam Lines"
            );

            ok &= expect(
                selectedEdges->mesh.vertices.size() == 2,
                "uma aresta selecionada gera dois vertices"
            );
        }

        const OverlayPrimitiveGroup* selectedFaces =
            find_group(
                geometry,
                OverlayPrimitiveRole::SelectedFaces
            );

        ok &= expect(
            selectedFaces != nullptr,
            "grupo de faces selecionadas foi gerado"
        );

        if (selectedFaces) {
            ok &= expect(
                selectedFaces->mesh.topology
                == PrimitiveTopology::Triangles,
                "faces selecionadas usam Triangles"
            );

            ok &= expect(
                selectedFaces->mesh.vertices.size() == 6,
                "face quad gera dois triangulos"
            );

            ok &= expect(
                selectedFaces->mesh.is_valid(),
                "mesh da face selecionada e valida"
            );
        }

        const OverlayPrimitiveGroup* hoveredVertex =
            find_group(
                geometry,
                OverlayPrimitiveRole::HoveredVertex
            );

        ok &= expect(
            hoveredVertex != nullptr,
            "grupo de vertice em hover foi gerado"
        );

        if (hoveredVertex) {
            ok &= expect(
                hoveredVertex->mesh.vertices.size() == 1,
                "hover de vertice gera um ponto"
            );
        }

        const OverlayPrimitiveGroup* hoveredEdge =
            find_group(
                geometry,
                OverlayPrimitiveRole::HoveredEdge
            );

        ok &= expect(
            hoveredEdge != nullptr,
            "grupo de aresta em hover foi gerado"
        );

        if (hoveredEdge) {
            ok &= expect(
                hoveredEdge->mesh.vertices.size() == 2,
                "hover de aresta gera dois vertices"
            );
        }

        const OverlayPrimitiveGroup* hoveredFace =
            find_group(
                geometry,
                OverlayPrimitiveRole::HoveredFace
            );

        ok &= expect(
            hoveredFace != nullptr,
            "grupo de face em hover foi gerado"
        );

        if (hoveredFace) {
            ok &= expect(
                hoveredFace->mesh.vertices.size() == 6,
                "hover de face quad gera seis vertices"
            );
        }

        ok &= expect(
            result.pointVertexCount == 2,
            "resultado conta dois vertices de pontos"
        );

        ok &= expect(
            result.lineVertexCount == 12,
            "resultado conta doze vertices de linhas"
        );

        ok &= expect(
            result.triangleVertexCount == 12,
            "resultado conta doze vertices de triangulos"
        );

        return ok;
    }

    bool test_selection_from_other_node() {
        std::cout << "\n=== Selection from another node ===\n";

        bool ok = true;

        QuadFixture fixture;

        MeshSelection selection;
        selection.set_active_mesh(SceneNodeId{ 999 });

        selection.add_vertex(fixture.vertex0);
        selection.add_edge(fixture.edge01);
        selection.add_face(fixture.face);

        selection.set_hovered_vertex(fixture.vertex1);
        selection.set_hovered_edge(fixture.edge12);
        selection.set_hovered_face(fixture.face);

        OverlayRenderResult result;

        const OverlayGeometry geometry =
            OverlayRenderAdapter::build_mesh_overlay(
                fixture.node,
                selection,
                {},
                &result
            );

        ok &= expect(
            geometry.groups.size() == 1,
            "selecao de outro node gera apenas wireframe"
        );

        ok &= expect(
            find_group(
                geometry,
                OverlayPrimitiveRole::Wireframe
            ) != nullptr,
            "wireframe continua sendo gerado"
        );

        ok &= expect(
            find_group(
                geometry,
                OverlayPrimitiveRole::SelectedVertices
            ) == nullptr,
            "vertices de outro node nao sao emitidos"
        );

        ok &= expect(
            find_group(
                geometry,
                OverlayPrimitiveRole::SelectedEdges
            ) == nullptr,
            "arestas de outro node nao sao emitidas"
        );

        ok &= expect(
            find_group(
                geometry,
                OverlayPrimitiveRole::SelectedFaces
            ) == nullptr,
            "faces de outro node nao sao emitidas"
        );

        ok &= expect(
            result.groupCount == 1,
            "resultado registra somente um grupo"
        );

        return ok;
    }

    bool test_overlay_options() {
        std::cout << "\n=== Overlay options ===\n";

        bool ok = true;

        QuadFixture fixture;

        MeshSelection selection;
        selection.set_active_mesh(fixture.node.id());
        selection.add_vertex(fixture.vertex0);
        selection.add_edge(fixture.edge01);
        selection.add_face(fixture.face);
        selection.set_hovered_vertex(fixture.vertex1);
        selection.set_hovered_edge(fixture.edge12);
        selection.set_hovered_face(fixture.face);

        OverlayRenderOptions options;
        options.includeWireframe = false;
        options.includeSelectedVertices = true;
        options.includeSelectedEdges = false;
        options.includeSelectedFaces = false;
        options.includeHoveredVertex = false;
        options.includeHoveredEdge = false;
        options.includeHoveredFace = false;

        OverlayRenderResult result;

        const OverlayGeometry geometry =
            OverlayRenderAdapter::build_mesh_overlay(
                fixture.node,
                selection,
                options,
                &result
            );

        ok &= expect(
            geometry.groups.size() == 1,
            "opcoes geram somente o grupo solicitado"
        );

        ok &= expect(
            find_group(
                geometry,
                OverlayPrimitiveRole::SelectedVertices
            ) != nullptr,
            "grupo solicitado foi gerado"
        );

        ok &= expect(
            find_group(
                geometry,
                OverlayPrimitiveRole::Wireframe
            ) == nullptr,
            "wireframe desabilitado nao foi gerado"
        );

        ok &= expect(
            find_group(
                geometry,
                OverlayPrimitiveRole::SelectedEdges
            ) == nullptr,
            "arestas selecionadas desabilitadas nao foram geradas"
        );

        ok &= expect(
            result.pointVertexCount == 1,
            "resultado conta apenas um ponto"
        );

        ok &= expect(
            result.lineVertexCount == 0,
            "resultado nao conta linhas"
        );

        ok &= expect(
            result.triangleVertexCount == 0,
            "resultado nao conta triangulos"
        );

        return ok;
    }

    bool test_overlay_colors() {
        std::cout << "\n=== Overlay colors ===\n";

        bool ok = true;

        QuadFixture fixture;

        MeshSelection selection;
        selection.set_active_mesh(fixture.node.id());
        selection.add_vertex(fixture.vertex0);

        OverlayRenderOptions options;
        options.includeWireframe = false;
        options.includeSelectedVertices = true;
        options.includeSelectedEdges = false;
        options.includeSelectedFaces = false;
        options.includeHoveredVertex = false;
        options.includeHoveredEdge = false;
        options.includeHoveredFace = false;

        options.selectedVertexColor = ColorRGBA{
            0.2f,
            0.4f,
            0.6f,
            0.8f
        };

        const OverlayGeometry geometry =
            OverlayRenderAdapter::build_mesh_overlay(
                fixture.node,
                selection,
                options
            );

        const OverlayPrimitiveGroup* group =
            find_group(
                geometry,
                OverlayPrimitiveRole::SelectedVertices
            );

        ok &= expect(
            group != nullptr,
            "grupo colorido foi gerado"
        );

        if (group && !group->mesh.vertices.empty()) {
            ok &= expect(
                color_equals(
                    group->mesh.vertices[0].color,
                    options.selectedVertexColor
                ),
                "cor configurada foi preservada"
            );
        }
        else {
            ok &= expect(
                false,
                "grupo colorido possui pelo menos um vertice"
            );
        }

        return ok;
    }

    bool test_hidden_components() {
        std::cout << "\n=== Hidden components ===\n";

        bool ok = true;

        QuadFixture fixture;

        fixture.node.mesh().edge(fixture.edge01).hidden = true;

        MeshSelection selection;
        selection.set_active_mesh(fixture.node.id());
        selection.add_edge(fixture.edge01);

        OverlayRenderOptions options;
        options.includeWireframe = true;
        options.includeSelectedVertices = false;
        options.includeSelectedEdges = true;
        options.includeSelectedFaces = false;
        options.includeHoveredVertex = false;
        options.includeHoveredEdge = false;
        options.includeHoveredFace = false;
        options.skipHiddenComponents = true;

        OverlayRenderResult result;

        const OverlayGeometry geometry =
            OverlayRenderAdapter::build_mesh_overlay(
                fixture.node,
                selection,
                options,
                &result
            );

        const OverlayPrimitiveGroup* wireframe =
            find_group(
                geometry,
                OverlayPrimitiveRole::Wireframe
            );

        ok &= expect(
            wireframe != nullptr,
            "wireframe restante foi gerado"
        );

        if (wireframe) {
            ok &= expect(
                wireframe->mesh.vertices.size() == 6,
                "aresta oculta e removida do wireframe"
            );
        }

        ok &= expect(
            find_group(
                geometry,
                OverlayPrimitiveRole::SelectedEdges
            ) == nullptr,
            "aresta selecionada oculta nao gera grupo"
        );

        options.skipHiddenComponents = false;

        const OverlayGeometry visibleHiddenGeometry =
            OverlayRenderAdapter::build_mesh_overlay(
                fixture.node,
                selection,
                options
            );

        const OverlayPrimitiveGroup* completeWireframe =
            find_group(
                visibleHiddenGeometry,
                OverlayPrimitiveRole::Wireframe
            );

        const OverlayPrimitiveGroup* selectedEdge =
            find_group(
                visibleHiddenGeometry,
                OverlayPrimitiveRole::SelectedEdges
            );

        ok &= expect(
            completeWireframe != nullptr
            && completeWireframe->mesh.vertices.size() == 8,
            "skipHiddenComponents false restaura a aresta no wireframe"
        );

        ok &= expect(
            selectedEdge != nullptr
            && selectedEdge->mesh.vertices.size() == 2,
            "skipHiddenComponents false restaura a aresta selecionada"
        );

        return ok;
    }

    bool test_empty_mesh() {
        std::cout << "\n=== Empty mesh overlay ===\n";

        bool ok = true;

        MeshNode node{
            SceneNodeId{ 100 },
            "Empty Mesh"
        };

        MeshSelection selection;
        selection.set_active_mesh(node.id());

        OverlayRenderResult result;

        const OverlayGeometry geometry =
            OverlayRenderAdapter::build_mesh_overlay(
                node,
                selection,
                {},
                &result
            );

        ok &= expect(
            !geometry.has_geometry(),
            "mesh vazia nao gera geometria"
        );

        ok &= expect(
            geometry.groups.empty(),
            "mesh vazia nao gera grupos"
        );

        ok &= expect(
            result.groupCount == 0,
            "resultado de mesh vazia informa zero grupos"
        );

        ok &= expect(
            result.visitedVertexCount == 0
            && result.visitedEdgeCount == 0
            && result.visitedFaceCount == 0,
            "mesh vazia nao possui componentes visitados"
        );

        return ok;
    }

    bool test_invalid_node_id() {
        std::cout << "\n=== Invalid node id ===\n";

        bool ok = true;

        MeshNode node{
            SceneNodeId{},
            "Invalid Mesh"
        };

        LEM& mesh = node.mesh();

        const VertexHandle vertex0 = mesh.add_vertex(
            glm::vec3{ 0.0f, 0.0f, 0.0f }
        );

        const VertexHandle vertex1 = mesh.add_vertex(
            glm::vec3{ 1.0f, 0.0f, 0.0f }
        );

        mesh.find_or_create_edge(vertex0, vertex1);

        MeshSelection selection;

        OverlayRenderResult result;

        const OverlayGeometry geometry =
            OverlayRenderAdapter::build_mesh_overlay(
                node,
                selection,
                {},
                &result
            );

        ok &= expect(
            !geometry.has_geometry(),
            "node com id invalido nao gera overlay"
        );

        ok &= expect(
            result.groupCount == 0,
            "node invalido informa zero grupos"
        );

        ok &= expect(
            !result.message.empty(),
            "node invalido produz diagnostico"
        );

        return ok;
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor OverlayRenderAdapter "
        "Smoke Test ===\n";

    bool ok = true;

    ok &= test_fixture_creation();
    ok &= test_complete_overlay();
    ok &= test_selection_from_other_node();
    ok &= test_overlay_options();
    ok &= test_overlay_colors();
    ok &= test_hidden_components();
    ok &= test_empty_mesh();
    ok &= test_invalid_node_id();

    std::cout << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes do "
            "OverlayRenderAdapter falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes do "
        "OverlayRenderAdapter passaram.\n";

    return EXIT_SUCCESS;
}