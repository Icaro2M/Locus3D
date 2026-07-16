/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "editor/sync/PickingSync.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/core/ToolResult.h"
#include "editor/tools/core/ToolState.h"
#include "editor/tools/mesh/edge/EdgeSlideTool.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

    using locus::editor::CommandDispatcher;
    using locus::editor::CommandResult;
    using locus::editor::Editor;
    using locus::editor::EditorDirtyFlags;
    using locus::editor::EditorMode;
    using locus::editor::EdgeSlideTool;
    using locus::editor::EdgeSlideToolOptions;
    using locus::editor::HistoryStack;
    using locus::editor::MeshNode;
    using locus::editor::PickingSync;
    using locus::editor::SceneNodeId;
    using locus::editor::ToolContext;
    using locus::editor::ToolEvent;
    using locus::editor::ToolEventType;
    using locus::editor::ToolPointerButton;
    using locus::editor::ToolResult;
    using locus::editor::ToolResultCode;
    using locus::editor::ToolState;

    using locus::kernel::geometry::EdgeHandle;
    using locus::kernel::geometry::FaceHandle;
    using locus::kernel::geometry::LEM;
    using locus::kernel::geometry::TopologyTraversal;
    using locus::kernel::geometry::VertexHandle;

    struct QuadFixture {
        SceneNodeId nodeId{};

        VertexHandle vertex0{};
        VertexHandle vertex1{};
        VertexHandle vertex2{};
        VertexHandle vertex3{};

        EdgeHandle bottomEdge{};
        EdgeHandle rightEdge{};
        EdgeHandle topEdge{};
        EdgeHandle leftEdge{};

        FaceHandle face{};
    };

    struct ToolServices {
        explicit ToolServices(
            Editor& editor)
            : dispatcher(editor),
            context(
                editor,
                dispatcher,
                history,
                pickingSync)
        {
        }

        CommandDispatcher dispatcher;
        HistoryStack history;
        PickingSync pickingSync;
        ToolContext context;
    };

    bool expect(
        const bool condition,
        const std::string& message)
    {
        if (condition) {
            std::cout
                << "[OK] "
                << message
                << '\n';

            return true;
        }

        std::cout
            << "[FAIL] "
            << message
            << '\n';

        return false;
    }

    bool nearly_equal(
        const float lhs,
        const float rhs,
        const float epsilon = 0.0001f)
    {
        return std::abs(lhs - rhs) <= epsilon;
    }

    bool nearly_equal(
        const glm::vec3& lhs,
        const glm::vec3& rhs,
        const float epsilon = 0.0001f)
    {
        return glm::length(lhs - rhs) <= epsilon;
    }

    bool has_dirty_flag(
        const ToolResult& result,
        const EditorDirtyFlags flag)
    {
        return locus::editor::has_flag(
            result.dirtyFlags,
            flag);
    }

    QuadFixture create_quad(
        Editor& editor,
        const std::string& name = "Edge Slide Fixture")
    {
        QuadFixture fixture{};

        fixture.nodeId =
            editor.scene().create_mesh(name);

        MeshNode* node =
            editor.scene().find_mesh(
                fixture.nodeId);

        if (!node) {
            return fixture;
        }

        LEM& mesh =
            node->mesh();

        /*
         * vertex3 -------- vertex2
         *    |                |
         *    |                |
         * vertex0 -------- vertex1
         *
         * bottomEdge is the edge targeted by the slide operation.
         */
        fixture.vertex0 =
            mesh.add_vertex(
                glm::vec3{
                    -1.0f,
                    -1.0f,
                    0.0f
                });

        fixture.vertex1 =
            mesh.add_vertex(
                glm::vec3{
                    1.0f,
                    -1.0f,
                    0.0f
                });

        fixture.vertex2 =
            mesh.add_vertex(
                glm::vec3{
                    1.0f,
                    1.0f,
                    0.0f
                });

        fixture.vertex3 =
            mesh.add_vertex(
                glm::vec3{
                    -1.0f,
                    1.0f,
                    0.0f
                });

        fixture.face =
            mesh.add_face({
                fixture.vertex0,
                fixture.vertex1,
                fixture.vertex2,
                fixture.vertex3
                });

        fixture.bottomEdge =
            mesh.find_edge(
                fixture.vertex0,
                fixture.vertex1);

        fixture.rightEdge =
            mesh.find_edge(
                fixture.vertex1,
                fixture.vertex2);

        fixture.topEdge =
            mesh.find_edge(
                fixture.vertex2,
                fixture.vertex3);

        fixture.leftEdge =
            mesh.find_edge(
                fixture.vertex3,
                fixture.vertex0);

        return fixture;
    }

    void select_bottom_edge(
        Editor& editor,
        const QuadFixture& fixture)
    {
        auto& selection =
            editor.selection().mesh();

        selection.set_active_mesh(
            fixture.nodeId);

        selection.set_edge(
            fixture.bottomEdge);
    }

    ToolEvent make_pointer_press(
        const glm::vec2 position,
        const float visualScale = 1.0f)
    {
        ToolEvent event{};

        event.type =
            ToolEventType::PointerPress;

        event.button =
            ToolPointerButton::Primary;

        event.pointer.viewportPosition =
            position;

        event.pointer.visualScale =
            visualScale;

        return event;
    }

    ToolEvent make_pointer_move(
        const glm::vec2 position,
        const glm::vec2 delta = glm::vec2{ 0.0f })
    {
        ToolEvent event{};

        event.type =
            ToolEventType::PointerMove;

        event.pointer.viewportPosition =
            position;

        event.pointer.viewportDelta =
            delta;

        return event;
    }

    ToolEvent make_pointer_release(
        const glm::vec2 position)
    {
        ToolEvent event{};

        event.type =
            ToolEventType::PointerRelease;

        event.button =
            ToolPointerButton::Primary;

        event.pointer.viewportPosition =
            position;

        return event;
    }

    std::size_t active_vertex_count(
        const LEM& mesh)
    {
        return TopologyTraversal::vertices(
            mesh).size();
    }

    std::size_t active_edge_count(
        const LEM& mesh)
    {
        return TopologyTraversal::edges(
            mesh).size();
    }

    std::size_t active_loop_count(
        const LEM& mesh)
    {
        return TopologyTraversal::loops(
            mesh).size();
    }

    std::size_t active_face_count(
        const LEM& mesh)
    {
        return TopologyTraversal::faces(
            mesh).size();
    }

    bool test_fixture()
    {
        std::cout
            << "\n=== Quad fixture ===\n";

        bool ok = true;

        Editor editor{};

        const QuadFixture fixture =
            create_quad(editor);

        const MeshNode* node =
            editor.scene().find_mesh(
                fixture.nodeId);

        ok &= expect(
            node != nullptr,
            "fixture cria MeshNode");

        if (!node) {
            return false;
        }

        const LEM& mesh =
            node->mesh();

        ok &= expect(
            mesh.is_valid(
                fixture.vertex0) &&
            mesh.is_valid(
                fixture.vertex1) &&
            mesh.is_valid(
                fixture.vertex2) &&
            mesh.is_valid(
                fixture.vertex3),
            "fixture possui quatro vertices validos");

        ok &= expect(
            mesh.is_valid(
                fixture.bottomEdge) &&
            mesh.is_valid(
                fixture.rightEdge) &&
            mesh.is_valid(
                fixture.topEdge) &&
            mesh.is_valid(
                fixture.leftEdge),
            "fixture possui quatro arestas validas");

        ok &= expect(
            mesh.is_valid(
                fixture.face),
            "fixture possui face valida");

        ok &= expect(
            active_vertex_count(mesh) == 4,
            "quad possui quatro vertices ativos");

        ok &= expect(
            active_edge_count(mesh) == 4,
            "quad possui quatro arestas ativas");

        ok &= expect(
            active_loop_count(mesh) == 4,
            "quad possui quatro loops ativos");

        ok &= expect(
            active_face_count(mesh) == 1,
            "quad possui uma face ativa");

        return ok;
    }

    bool test_activation_and_options()
    {
        std::cout
            << "\n=== Activation and options ===\n";

        bool ok = true;

        Editor editor{};
        ToolServices services{ editor };

        EdgeSlideTool tool{};

        ok &= expect(
            tool.state() ==
            ToolState::Inactive,
            "EdgeSlideTool comeca Inactive");

        ok &= expect(
            !tool.can_activate(
                services.context),
            "tool nao ativa em Object mode");

        editor.set_mode(
            EditorMode::Mesh);

        ok &= expect(
            tool.can_activate(
                services.context),
            "tool ativa em Mesh mode");

        const ToolResult activation =
            tool.activate(
                services.context);

        ok &= expect(
            !activation.failed(),
            "ativacao em Mesh mode nao falha");

        ok &= expect(
            tool.state() ==
            ToolState::Ready,
            "ativacao move tool para Ready");

        ok &= expect(
            tool.descriptor().is_valid(),
            "descriptor do edge slide e valido");

        ok &= expect(
            tool.descriptor().id.value ==
            std::string{
                EdgeSlideTool::Id
            },
            "descriptor usa id estavel do edge slide");

        EdgeSlideToolOptions options =
            tool.options();

        options.distancePerPixel =
            0.05f;

        options.distanceEpsilon =
            0.0001f;

        options.invertDragDirection =
            true;

        options.excludeTargetEdgesFromRails =
            false;

        ok &= expect(
            tool.set_options(options),
            "opcoes podem mudar fora da interacao");

        ok &= expect(
            nearly_equal(
                tool.options().distancePerPixel,
                0.05f),
            "distancePerPixel foi atualizado");

        ok &= expect(
            tool.options().invertDragDirection,
            "invertDragDirection foi atualizado");

        ok &= expect(
            !tool.options()
            .excludeTargetEdgesFromRails,
            "opcao de trilhos foi atualizada");

        EdgeSlideToolOptions invalidOptions{};

        invalidOptions.distancePerPixel =
            -1.0f;

        invalidOptions.distanceEpsilon =
            -0.5f;

        ok &= expect(
            tool.set_options(
                invalidOptions),
            "opcoes numericas invalidas sao sanitizadas");

        ok &= expect(
            nearly_equal(
                tool.options().distancePerPixel,
                0.0f),
            "distancePerPixel negativo e limitado a zero");

        ok &= expect(
            nearly_equal(
                tool.options().distanceEpsilon,
                0.0f),
            "distanceEpsilon negativo e limitado a zero");

        tool.deactivate(
            services.context);

        return ok;
    }

    bool test_preview_is_non_destructive()
    {
        std::cout
            << "\n=== Non-destructive edge slide preview ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_bottom_edge(
            editor,
            fixture);

        ToolServices services{ editor };
        EdgeSlideTool tool{};

        tool.activate(
            services.context);

        const MeshNode* node =
            editor.scene().find_mesh(
                fixture.nodeId);

        if (!node) {
            return expect(
                false,
                "fixture possui MeshNode");
        }

        const LEM& originalMesh =
            node->mesh();

        const std::size_t originalVertices =
            active_vertex_count(
                originalMesh);

        const std::size_t originalEdges =
            active_edge_count(
                originalMesh);

        const std::size_t originalLoops =
            active_loop_count(
                originalMesh);

        const std::size_t originalFaces =
            active_face_count(
                originalMesh);

        const glm::vec3 originalPosition0 =
            originalMesh
            .vertex(fixture.vertex0)
            .position;

        const glm::vec3 originalPosition1 =
            originalMesh
            .vertex(fixture.vertex1)
            .position;

        const ToolResult pressResult =
            tool.handle_event(
                services.context,
                make_pointer_press(
                    glm::vec2{
                        100.0f,
                        100.0f
                    }));

        ok &= expect(
            pressResult.code ==
            ToolResultCode::Started,
            "pointer press inicia edge slide");

        ok &= expect(
            tool.state() ==
            ToolState::Interacting,
            "tool entra em Interacting");

        ok &= expect(
            tool.mesh_session().is_active(),
            "edge slide inicia MeshOperationSession");

        ok &= expect(
            nearly_equal(
                tool.distance(),
                0.0f),
            "distancia inicial e zero");

        ok &= expect(
            !tool.has_operation_preview(),
            "distancia inicial zero nao gera preview pronto");

        ok &= expect(
            tool.operation_preview().is_empty(),
            "preview inicial do edge slide e Empty");

        const ToolResult moveResult =
            tool.handle_event(
                services.context,
                make_pointer_move(
                    glm::vec2{
                        150.0f,
                        100.0f
                    },
                    glm::vec2{
                        50.0f,
                        0.0f
                    }));

        ok &= expect(
            moveResult.code ==
            ToolResultCode::Updated,
            "pointer move atualiza edge slide");

        ok &= expect(
            nearly_equal(
                tool.distance(),
                0.5f),
            "arrasto de 50 pixels gera distancia 0.5");

        ok &= expect(
            tool.has_operation_preview(),
            "distancia valida gera preview pronto");

        ok &= expect(
            tool.operation_preview().is_ready(),
            "OperationPreview do edge slide fica Ready");

        ok &= expect(
            tool.operation_preview()
            .mesh()
            .valid(),
            "preview contem payload valido");

        ok &= expect(
            !tool.operation_preview()
            .mesh()
            .solid_mesh()
            .empty(),
            "preview possui geometria solida");

        ok &= expect(
            !tool.operation_preview()
            .mesh()
            .wire_mesh()
            .empty(),
            "preview possui geometria wire");

        const LEM& meshAfterPreview =
            node->mesh();

        ok &= expect(
            active_vertex_count(
                meshAfterPreview) ==
            originalVertices,
            "preview nao altera vertices autoritativos");

        ok &= expect(
            active_edge_count(
                meshAfterPreview) ==
            originalEdges,
            "preview nao altera arestas autoritativas");

        ok &= expect(
            active_loop_count(
                meshAfterPreview) ==
            originalLoops,
            "preview nao altera loops autoritativos");

        ok &= expect(
            active_face_count(
                meshAfterPreview) ==
            originalFaces,
            "preview nao altera faces autoritativas");

        ok &= expect(
            nearly_equal(
                meshAfterPreview
                .vertex(fixture.vertex0)
                .position,
                originalPosition0) &&
            nearly_equal(
                meshAfterPreview
                .vertex(fixture.vertex1)
                .position,
                originalPosition1),
            "preview nao altera posicoes autoritativas");

        ok &= expect(
            services.history.empty(),
            "preview nao cria entrada no historico");

        ok &= expect(
            !tool.set_options(
                EdgeSlideToolOptions{}),
            "opcoes nao mudam durante interacao");

        tool.cancel(
            services.context);

        return ok;
    }

    bool test_positive_commit_and_history()
    {
        std::cout
            << "\n=== Positive commit, undo and redo ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_bottom_edge(
            editor,
            fixture);

        ToolServices services{ editor };
        EdgeSlideTool tool{};

        tool.activate(
            services.context);

        MeshNode* node =
            editor.scene().find_mesh(
                fixture.nodeId);

        if (!node) {
            return expect(
                false,
                "fixture possui MeshNode");
        }

        const LEM& initialMesh =
            node->mesh();

        const std::size_t originalVertices =
            active_vertex_count(
                initialMesh);

        const std::size_t originalEdges =
            active_edge_count(
                initialMesh);

        const std::size_t originalLoops =
            active_loop_count(
                initialMesh);

        const std::size_t originalFaces =
            active_face_count(
                initialMesh);

        const glm::vec3 originalPosition0 =
            initialMesh
            .vertex(fixture.vertex0)
            .position;

        const glm::vec3 originalPosition1 =
            initialMesh
            .vertex(fixture.vertex1)
            .position;

        const glm::vec3 originalPosition2 =
            initialMesh
            .vertex(fixture.vertex2)
            .position;

        const glm::vec3 originalPosition3 =
            initialMesh
            .vertex(fixture.vertex3)
            .position;

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    100.0f
                }));

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    50.0f,
                    100.0f
                }));

        ok &= expect(
            nearly_equal(
                tool.distance(),
                0.5f),
            "arrasto configura distancia final 0.5");

        const ToolResult releaseResult =
            tool.handle_event(
                services.context,
                make_pointer_release(
                    glm::vec2{
                        50.0f,
                        100.0f
                    }));

        ok &= expect(
            releaseResult.code ==
            ToolResultCode::Confirmed,
            "pointer release confirma edge slide");

        ok &= expect(
            tool.state() ==
            ToolState::Ready,
            "confirmacao retorna tool para Ready");

        ok &= expect(
            !tool.mesh_session().is_active(),
            "confirmacao limpa MeshOperationSession");

        ok &= expect(
            has_dirty_flag(
                releaseResult,
                EditorDirtyFlags::Mesh),
            "commit marca Mesh dirty");

        ok &= expect(
            has_dirty_flag(
                releaseResult,
                EditorDirtyFlags::Render),
            "commit marca Render dirty");

        ok &= expect(
            has_dirty_flag(
                releaseResult,
                EditorDirtyFlags::Picking),
            "commit marca Picking dirty");

        const LEM& slidMesh =
            node->mesh();

        ok &= expect(
            active_vertex_count(
                slidMesh) ==
            originalVertices,
            "edge slide preserva quantidade de vertices");

        ok &= expect(
            active_edge_count(
                slidMesh) ==
            originalEdges,
            "edge slide preserva quantidade de arestas");

        ok &= expect(
            active_loop_count(
                slidMesh) ==
            originalLoops,
            "edge slide preserva quantidade de loops");

        ok &= expect(
            active_face_count(
                slidMesh) ==
            originalFaces,
            "edge slide preserva quantidade de faces");

        ok &= expect(
            slidMesh.is_valid(
                fixture.vertex0) &&
            slidMesh.is_valid(
                fixture.vertex1) &&
            slidMesh.is_valid(
                fixture.vertex2) &&
            slidMesh.is_valid(
                fixture.vertex3),
            "edge slide preserva handles de vertices");

        ok &= expect(
            slidMesh.is_valid(
                fixture.bottomEdge) &&
            slidMesh.is_valid(
                fixture.rightEdge) &&
            slidMesh.is_valid(
                fixture.topEdge) &&
            slidMesh.is_valid(
                fixture.leftEdge),
            "edge slide preserva handles de arestas");

        const glm::vec3 expectedPosition0 =
            originalPosition0 +
            glm::vec3{
                0.0f,
                0.5f,
                0.0f
        };

        const glm::vec3 expectedPosition1 =
            originalPosition1 +
            glm::vec3{
                0.0f,
                0.5f,
                0.0f
        };

        ok &= expect(
            nearly_equal(
                slidMesh
                .vertex(fixture.vertex0)
                .position,
                expectedPosition0),
            "vertice esquerdo desliza pela aresta lateral");

        ok &= expect(
            nearly_equal(
                slidMesh
                .vertex(fixture.vertex1)
                .position,
                expectedPosition1),
            "vertice direito desliza pela aresta lateral");

        ok &= expect(
            nearly_equal(
                slidMesh
                .vertex(fixture.vertex2)
                .position,
                originalPosition2) &&
            nearly_equal(
                slidMesh
                .vertex(fixture.vertex3)
                .position,
                originalPosition3),
            "vertices nao alvo permanecem parados");

        ok &= expect(
            services.history.can_undo(),
            "commit cria entrada de undo");

        ok &= expect(
            services.history.undo_size() == 1,
            "historico possui uma entrada");

        ok &= expect(
            services.history.undo_name() ==
            "Slide Edges",
            "entrada possui nome Slide Edges");

        ok &= expect(
            !services.history.can_redo(),
            "redo nao esta disponivel antes do undo");

        const CommandResult undoResult =
            services.history.undo(
                services.dispatcher);

        ok &= expect(
            undoResult.success,
            "undo do edge slide funciona");

        ok &= expect(
            nearly_equal(
                node->mesh()
                .vertex(fixture.vertex0)
                .position,
                originalPosition0) &&
            nearly_equal(
                node->mesh()
                .vertex(fixture.vertex1)
                .position,
                originalPosition1),
            "undo restaura vertices alvo");

        ok &= expect(
            active_vertex_count(
                node->mesh()) ==
            originalVertices &&
            active_edge_count(
                node->mesh()) ==
            originalEdges &&
            active_loop_count(
                node->mesh()) ==
            originalLoops &&
            active_face_count(
                node->mesh()) ==
            originalFaces,
            "undo preserva topologia original");

        ok &= expect(
            services.history.can_redo(),
            "redo fica disponivel depois do undo");

        ok &= expect(
            services.history.redo_name() ==
            "Slide Edges",
            "entrada de redo preserva nome");

        const CommandResult redoResult =
            services.history.redo(
                services.dispatcher);

        ok &= expect(
            redoResult.success,
            "redo do edge slide funciona");

        ok &= expect(
            nearly_equal(
                node->mesh()
                .vertex(fixture.vertex0)
                .position,
                expectedPosition0) &&
            nearly_equal(
                node->mesh()
                .vertex(fixture.vertex1)
                .position,
                expectedPosition1),
            "redo restaura posicoes deslizadas");

        return ok;
    }

    bool test_negative_distance()
    {
        std::cout
            << "\n=== Negative slide distance ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_bottom_edge(
            editor,
            fixture);

        ToolServices services{ editor };
        EdgeSlideTool tool{};

        tool.activate(
            services.context);

        MeshNode* node =
            editor.scene().find_mesh(
                fixture.nodeId);

        if (!node) {
            return expect(
                false,
                "fixture possui MeshNode");
        }

        const glm::vec3 originalPosition0 =
            node->mesh()
            .vertex(fixture.vertex0)
            .position;

        const glm::vec3 originalPosition1 =
            node->mesh()
            .vertex(fixture.vertex1)
            .position;

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    100.0f,
                    100.0f
                }));

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    75.0f,
                    100.0f
                }));

        ok &= expect(
            nearly_equal(
                tool.distance(),
                -0.25f),
            "arrasto para esquerda gera distancia negativa");

        const ToolResult result =
            tool.handle_event(
                services.context,
                make_pointer_release(
                    glm::vec2{
                        75.0f,
                        100.0f
                    }));

        ok &= expect(
            result.code ==
            ToolResultCode::Confirmed,
            "edge slide negativo confirma");

        ok &= expect(
            nearly_equal(
                node->mesh()
                .vertex(fixture.vertex0)
                .position,
                originalPosition0 +
                glm::vec3{
                    0.0f,
                    -0.25f,
                    0.0f
                }),
            "vertice esquerdo aceita deslocamento negativo");

        ok &= expect(
            nearly_equal(
                node->mesh()
                .vertex(fixture.vertex1)
                .position,
                originalPosition1 +
                glm::vec3{
                    0.0f,
                    -0.25f,
                    0.0f
                }),
            "vertice direito aceita deslocamento negativo");

        return ok;
    }

    bool test_cancel_does_not_commit()
    {
        std::cout
            << "\n=== Cancellation ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_bottom_edge(
            editor,
            fixture);

        ToolServices services{ editor };
        EdgeSlideTool tool{};

        tool.activate(
            services.context);

        MeshNode* node =
            editor.scene().find_mesh(
                fixture.nodeId);

        if (!node) {
            return expect(
                false,
                "fixture possui MeshNode");
        }

        const glm::vec3 originalPosition0 =
            node->mesh()
            .vertex(fixture.vertex0)
            .position;

        const glm::vec3 originalPosition1 =
            node->mesh()
            .vertex(fixture.vertex1)
            .position;

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    100.0f,
                    0.0f
                }));

        ok &= expect(
            tool.has_operation_preview(),
            "cancelamento parte de preview pronto");

        const ToolResult cancelResult =
            tool.cancel(
                services.context);

        ok &= expect(
            cancelResult.code ==
            ToolResultCode::Cancelled,
            "cancel retorna Cancelled");

        ok &= expect(
            tool.state() ==
            ToolState::Ready,
            "cancel retorna tool para Ready");

        ok &= expect(
            !tool.mesh_session().is_active(),
            "cancel limpa sessao");

        ok &= expect(
            nearly_equal(
                node->mesh()
                .vertex(fixture.vertex0)
                .position,
                originalPosition0) &&
            nearly_equal(
                node->mesh()
                .vertex(fixture.vertex1)
                .position,
                originalPosition1),
            "cancel nao altera vertices");

        ok &= expect(
            services.history.empty(),
            "cancel nao cria historico");

        return ok;
    }

    bool test_zero_distance_confirmation()
    {
        std::cout
            << "\n=== Zero-distance confirmation ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_bottom_edge(
            editor,
            fixture);

        ToolServices services{ editor };
        EdgeSlideTool tool{};

        tool.activate(
            services.context);

        MeshNode* node =
            editor.scene().find_mesh(
                fixture.nodeId);

        if (!node) {
            return expect(
                false,
                "fixture possui MeshNode");
        }

        const glm::vec3 originalPosition0 =
            node->mesh()
            .vertex(fixture.vertex0)
            .position;

        const glm::vec3 originalPosition1 =
            node->mesh()
            .vertex(fixture.vertex1)
            .position;

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    20.0f,
                    20.0f
                }));

        const ToolResult releaseResult =
            tool.handle_event(
                services.context,
                make_pointer_release(
                    glm::vec2{
                        20.0f,
                        20.0f
                    }));

        ok &= expect(
            releaseResult.code ==
            ToolResultCode::Confirmed,
            "release sem movimento conclui interacao");

        ok &= expect(
            nearly_equal(
                node->mesh()
                .vertex(fixture.vertex0)
                .position,
                originalPosition0) &&
            nearly_equal(
                node->mesh()
                .vertex(fixture.vertex1)
                .position,
                originalPosition1),
            "distancia zero nao move vertices");

        ok &= expect(
            services.history.empty(),
            "distancia zero nao cria entrada no historico");

        return ok;
    }

    bool test_visual_scale_and_direction()
    {
        std::cout
            << "\n=== Visual scale and drag direction ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_bottom_edge(
            editor,
            fixture);

        ToolServices services{ editor };

        EdgeSlideToolOptions options{};

        options.distancePerPixel =
            0.01f;

        options.invertDragDirection =
            true;

        EdgeSlideTool tool{
            options
        };

        tool.activate(
            services.context);

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                },
                2.0f));

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    50.0f,
                    0.0f
                }));

        ok &= expect(
            nearly_equal(
                tool.distance(),
                -1.0f),
            "visualScale e invertDragDirection afetam distancia");

        tool.cancel(
            services.context);

        return ok;
    }

    bool test_no_valid_rails()
    {
        std::cout
            << "\n=== Target without valid rails ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const SceneNodeId nodeId =
            editor.scene().create_mesh(
                "Loose Edge");

        MeshNode* node =
            editor.scene().find_mesh(
                nodeId);

        if (!node) {
            return expect(
                false,
                "fixture loose edge possui MeshNode");
        }

        LEM& mesh =
            node->mesh();

        const VertexHandle vertex0 =
            mesh.add_vertex(
                glm::vec3{
                    -1.0f,
                    0.0f,
                    0.0f
                });

        const VertexHandle vertex1 =
            mesh.add_vertex(
                glm::vec3{
                    1.0f,
                    0.0f,
                    0.0f
                });

        const EdgeHandle edge =
            mesh.find_or_create_edge(
                vertex0,
                vertex1);

        auto& selection =
            editor.selection().mesh();

        selection.set_active_mesh(
            nodeId);

        selection.set_edge(
            edge);

        ToolServices services{ editor };
        EdgeSlideTool tool{};

        tool.activate(
            services.context);

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        const ToolResult moveResult =
            tool.handle_event(
                services.context,
                make_pointer_move(
                    glm::vec2{
                        50.0f,
                        0.0f
                    }));

        ok &= expect(
            moveResult.code ==
            ToolResultCode::Updated,
            "operacao sem trilhos ainda processa update");

        ok &= expect(
            tool.operation_preview().is_empty(),
            "aresta solta sem trilhos produz preview Empty");

        ok &= expect(
            !tool.has_operation_preview(),
            "aresta solta nao produz preview pronto");

        const ToolResult releaseResult =
            tool.handle_event(
                services.context,
                make_pointer_release(
                    glm::vec2{
                        50.0f,
                        0.0f
                    }));

        ok &= expect(
            releaseResult.code ==
            ToolResultCode::Confirmed,
            "preview Empty conclui como no-op");

        ok &= expect(
            nearly_equal(
                node->mesh()
                .vertex(vertex0)
                .position,
                glm::vec3{
                    -1.0f,
                    0.0f,
                    0.0f
                }) &&
            nearly_equal(
                node->mesh()
                .vertex(vertex1)
                .position,
                glm::vec3{
                    1.0f,
                    0.0f,
                    0.0f
                }),
            "aresta sem trilhos nao move vertices");

        ok &= expect(
            services.history.empty(),
            "aresta sem trilhos nao cria historico");

        return ok;
    }

    bool test_stable_captured_target()
    {
        std::cout
            << "\n=== Stable captured target ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture firstFixture =
            create_quad(
                editor,
                "First Quad");

        const QuadFixture secondFixture =
            create_quad(
                editor,
                "Second Quad");

        select_bottom_edge(
            editor,
            firstFixture);

        ToolServices services{ editor };
        EdgeSlideTool tool{};

        tool.activate(
            services.context);

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        select_bottom_edge(
            editor,
            secondFixture);

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    50.0f,
                    0.0f
                }));

        const ToolResult releaseResult =
            tool.handle_event(
                services.context,
                make_pointer_release(
                    glm::vec2{
                        50.0f,
                        0.0f
                    }));

        const MeshNode* firstNode =
            editor.scene().find_mesh(
                firstFixture.nodeId);

        const MeshNode* secondNode =
            editor.scene().find_mesh(
                secondFixture.nodeId);

        ok &= expect(
            releaseResult.code ==
            ToolResultCode::Confirmed,
            "edge slide confirma apos mudanca de selecao");

        ok &= expect(
            firstNode != nullptr &&
            nearly_equal(
                firstNode->mesh()
                .vertex(firstFixture.vertex0)
                .position,
                glm::vec3{
                    -1.0f,
                    -0.5f,
                    0.0f
                }),
            "edge slide modifica node capturado");

        ok &= expect(
            secondNode != nullptr &&
            nearly_equal(
                secondNode->mesh()
                .vertex(secondFixture.vertex0)
                .position,
                glm::vec3{
                    -1.0f,
                    -1.0f,
                    0.0f
                }),
            "edge slide nao modifica node selecionado depois");

        return ok;
    }

    bool test_missing_command_services()
    {
        std::cout
            << "\n=== Missing command services ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_bottom_edge(
            editor,
            fixture);

        ToolContext context{
            editor
        };

        EdgeSlideTool tool{};

        tool.activate(
            context);

        tool.handle_event(
            context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        tool.handle_event(
            context,
            make_pointer_move(
                glm::vec2{
                    50.0f,
                    0.0f
                }));

        const ToolResult releaseResult =
            tool.handle_event(
                context,
                make_pointer_release(
                    glm::vec2{
                        50.0f,
                        0.0f
                    }));

        ok &= expect(
            releaseResult.code ==
            ToolResultCode::Failed,
            "commit falha sem command services");

        ok &= expect(
            tool.state() ==
            ToolState::Interacting,
            "falha de commit preserva interacao");

        ok &= expect(
            tool.mesh_session().is_active(),
            "falha de commit preserva preview e sessao");

        const ToolResult cancelResult =
            tool.cancel(
                context);

        ok &= expect(
            cancelResult.code ==
            ToolResultCode::Cancelled,
            "tool pode ser cancelada apos falha de commit");

        return ok;
    }

} // namespace

int main()
{
    std::cout
        << "=== Locus3D Editor EdgeSlideTool "
        << "Smoke Test ===\n";

    bool ok = true;

    ok &= test_fixture();
    ok &= test_activation_and_options();
    ok &= test_preview_is_non_destructive();
    ok &= test_positive_commit_and_history();
    ok &= test_negative_distance();
    ok &= test_cancel_does_not_commit();
    ok &= test_zero_distance_confirmation();
    ok &= test_visual_scale_and_direction();
    ok &= test_no_valid_rails();
    ok &= test_stable_captured_target();
    ok &= test_missing_command_services();

    std::cout
        << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes do "
            << "EdgeSlideTool falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes do "
        << "EdgeSlideTool passaram.\n";

    return EXIT_SUCCESS;
}