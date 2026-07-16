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
#include "editor/tools/mesh/topology/LoopCutTool.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

    using locus::editor::CommandDispatcher;
    using locus::editor::CommandResult;
    using locus::editor::Editor;
    using locus::editor::EditorDirtyFlags;
    using locus::editor::EditorMode;
    using locus::editor::HistoryStack;
    using locus::editor::LoopCutTool;
    using locus::editor::LoopCutToolOptions;
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
        const std::string& name = "Loop Cut Fixture")
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
         * bottomEdge and topEdge are the two loop-cut targets.
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

    void select_opposite_edges(
        Editor& editor,
        const QuadFixture& fixture)
    {
        auto& selection =
            editor.selection().mesh();

        selection.set_active_mesh(
            fixture.nodeId);

        selection.set_edge(
            fixture.bottomEdge);

        selection.add_edge(
            fixture.topEdge);
    }

    void select_single_edge(
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

    bool contains_vertex_near_x(
        const LEM& mesh,
        const float expectedX,
        const float expectedY,
        const float epsilon = 0.0001f)
    {
        const std::vector<VertexHandle> vertices =
            TopologyTraversal::vertices(mesh);

        for (const VertexHandle vertex : vertices) {
            if (!mesh.is_valid(vertex)) {
                continue;
            }

            const glm::vec3 position =
                mesh.vertex(vertex).position;

            if (std::abs(position.x - expectedX) <= epsilon &&
                std::abs(position.y - expectedY) <= epsilon) {
                return true;
            }
        }

        return false;
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
            mesh.is_valid(fixture.vertex0) &&
            mesh.is_valid(fixture.vertex1) &&
            mesh.is_valid(fixture.vertex2) &&
            mesh.is_valid(fixture.vertex3),
            "fixture possui quatro vertices validos");

        ok &= expect(
            mesh.is_valid(fixture.bottomEdge) &&
            mesh.is_valid(fixture.rightEdge) &&
            mesh.is_valid(fixture.topEdge) &&
            mesh.is_valid(fixture.leftEdge),
            "fixture possui quatro arestas validas");

        ok &= expect(
            mesh.is_valid(fixture.face),
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

        LoopCutTool tool{};

        ok &= expect(
            tool.state() ==
            ToolState::Inactive,
            "LoopCutTool comeca Inactive");

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
            "descriptor do loop cut e valido");

        ok &= expect(
            tool.descriptor().id.value ==
            std::string{
                LoopCutTool::Id
            },
            "descriptor usa id estavel do loop cut");

        LoopCutToolOptions options =
            tool.options();

        options.factorPerPixel =
            0.01f;

        options.factorEpsilon =
            0.0001f;

        options.minimumFactor =
            0.1f;

        options.maximumFactor =
            0.9f;

        options.initialFactor =
            0.25f;

        options.cuts =
            2;

        options.evenSpacing =
            true;

        ok &= expect(
            tool.set_options(options),
            "opcoes podem mudar fora da interacao");

        ok &= expect(
            nearly_equal(
                tool.options().factorPerPixel,
                0.01f),
            "factorPerPixel foi atualizado");

        ok &= expect(
            nearly_equal(
                tool.options().initialFactor,
                0.25f),
            "initialFactor foi atualizado");

        ok &= expect(
            tool.cuts() == 2,
            "cuts foi atualizado");

        ok &= expect(
            tool.options().evenSpacing,
            "evenSpacing foi atualizado");

        LoopCutToolOptions invalidOptions{};

        invalidOptions.factorPerPixel =
            -1.0f;

        invalidOptions.factorEpsilon =
            -0.5f;

        invalidOptions.minimumFactor =
            -10.0f;

        invalidOptions.maximumFactor =
            10.0f;

        invalidOptions.initialFactor =
            5.0f;

        invalidOptions.cuts =
            0;

        ok &= expect(
            tool.set_options(
                invalidOptions),
            "opcoes invalidas sao sanitizadas");

        ok &= expect(
            nearly_equal(
                tool.options().factorPerPixel,
                0.0f),
            "factorPerPixel negativo e limitado a zero");

        ok &= expect(
            nearly_equal(
                tool.options().factorEpsilon,
                0.0f),
            "factorEpsilon negativo e limitado a zero");

        ok &= expect(
            tool.options().minimumFactor >= 0.0001f,
            "minimumFactor respeita limite do kernel");

        ok &= expect(
            tool.options().maximumFactor <= 0.9999f,
            "maximumFactor respeita limite do kernel");

        ok &= expect(
            tool.options().initialFactor >=
            tool.options().minimumFactor &&
            tool.options().initialFactor <=
            tool.options().maximumFactor,
            "initialFactor fica dentro do intervalo");

        ok &= expect(
            tool.cuts() == 1,
            "cuts zero e limitado a um");

        tool.deactivate(
            services.context);

        return ok;
    }

    bool test_interactive_preview_is_non_destructive()
    {
        std::cout
            << "\n=== Non-destructive interactive preview ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_opposite_edges(
            editor,
            fixture);

        ToolServices services{ editor };

        LoopCutToolOptions options{};

        options.factorPerPixel =
            0.005f;

        options.initialFactor =
            0.5f;

        options.minimumFactor =
            0.1f;

        options.maximumFactor =
            0.9f;

        options.cuts =
            1;

        options.evenSpacing =
            false;

        LoopCutTool tool{
            options
        };

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

        const std::size_t originalVertices =
            active_vertex_count(
                node->mesh());

        const std::size_t originalEdges =
            active_edge_count(
                node->mesh());

        const std::size_t originalLoops =
            active_loop_count(
                node->mesh());

        const std::size_t originalFaces =
            active_face_count(
                node->mesh());

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
            "pointer press inicia loop cut");

        ok &= expect(
            tool.state() ==
            ToolState::Interacting,
            "tool entra em Interacting");

        ok &= expect(
            tool.mesh_session().is_active(),
            "loop cut inicia MeshOperationSession");

        ok &= expect(
            nearly_equal(
                tool.factor(),
                0.5f),
            "interacao comeca no initialFactor");

        ok &= expect(
            tool.has_operation_preview(),
            "loop cut gera preview inicial pronto");

        ok &= expect(
            tool.operation_preview().is_ready(),
            "preview inicial possui status Ready");

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
            "pointer move atualiza loop cut");

        ok &= expect(
            nearly_equal(
                tool.factor(),
                0.75f),
            "arrasto de 50 pixels move fator para 0.75");

        ok &= expect(
            tool.has_operation_preview(),
            "fator atualizado preserva preview pronto");

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

        ok &= expect(
            active_vertex_count(
                node->mesh()) ==
            originalVertices,
            "preview nao altera vertices autoritativos");

        ok &= expect(
            active_edge_count(
                node->mesh()) ==
            originalEdges,
            "preview nao altera arestas autoritativas");

        ok &= expect(
            active_loop_count(
                node->mesh()) ==
            originalLoops,
            "preview nao altera loops autoritativos");

        ok &= expect(
            active_face_count(
                node->mesh()) ==
            originalFaces,
            "preview nao altera faces autoritativas");

        ok &= expect(
            services.history.empty(),
            "preview nao cria entrada no historico");

        ok &= expect(
            !tool.set_options(
                LoopCutToolOptions{}),
            "opcoes nao mudam durante interacao");

        tool.cancel(
            services.context);

        return ok;
    }

    bool test_single_cut_commit_and_history()
    {
        std::cout
            << "\n=== Single cut commit, undo and redo ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_opposite_edges(
            editor,
            fixture);

        ToolServices services{ editor };

        LoopCutToolOptions options{};

        options.initialFactor =
            0.5f;

        options.cuts =
            1;

        options.evenSpacing =
            false;

        LoopCutTool tool{
            options
        };

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

        const std::size_t originalVertices =
            active_vertex_count(
                node->mesh());

        const std::size_t originalEdges =
            active_edge_count(
                node->mesh());

        const std::size_t originalLoops =
            active_loop_count(
                node->mesh());

        const std::size_t originalFaces =
            active_face_count(
                node->mesh());

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    100.0f,
                    100.0f
                }));

        ok &= expect(
            tool.has_operation_preview(),
            "single cut possui preview pronto antes do release");

        const ToolResult releaseResult =
            tool.handle_event(
                services.context,
                make_pointer_release(
                    glm::vec2{
                        100.0f,
                        100.0f
                    }));

        ok &= expect(
            releaseResult.code ==
            ToolResultCode::Confirmed,
            "pointer release confirma loop cut");

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

        const LEM& cutMesh =
            node->mesh();

        ok &= expect(
            active_vertex_count(
                cutMesh) ==
            originalVertices + 2,
            "single cut adiciona um vertice em cada aresta alvo");

        ok &= expect(
            active_edge_count(
                cutMesh) >
            originalEdges,
            "single cut aumenta quantidade de arestas");

        ok &= expect(
            active_loop_count(
                cutMesh) >
            originalLoops,
            "single cut aumenta quantidade de loops");

        ok &= expect(
            active_face_count(
                cutMesh) == 2,
            "single cut divide o quad em duas faces");

        ok &= expect(
            contains_vertex_near_x(
                cutMesh,
                0.0f,
                -1.0f),
            "single cut cria vertice central na aresta inferior");

        ok &= expect(
            contains_vertex_near_x(
                cutMesh,
                0.0f,
                1.0f),
            "single cut cria vertice central na aresta superior");

        ok &= expect(
            services.history.can_undo(),
            "commit cria entrada de undo");

        ok &= expect(
            services.history.undo_size() == 1,
            "historico possui uma entrada");

        ok &= expect(
            services.history.undo_name() ==
            "Loop Cut",
            "entrada possui nome Loop Cut");

        ok &= expect(
            !services.history.can_redo(),
            "redo nao esta disponivel antes do undo");

        const CommandResult undoResult =
            services.history.undo(
                services.dispatcher);

        ok &= expect(
            undoResult.success,
            "undo do loop cut funciona");

        ok &= expect(
            active_vertex_count(
                node->mesh()) ==
            originalVertices,
            "undo restaura vertices originais");

        ok &= expect(
            active_edge_count(
                node->mesh()) ==
            originalEdges,
            "undo restaura arestas originais");

        ok &= expect(
            active_loop_count(
                node->mesh()) ==
            originalLoops,
            "undo restaura loops originais");

        ok &= expect(
            active_face_count(
                node->mesh()) ==
            originalFaces,
            "undo restaura face original");

        ok &= expect(
            node->mesh().is_valid(
                fixture.face),
            "undo restaura handle da face original");

        ok &= expect(
            services.history.can_redo(),
            "redo fica disponivel depois do undo");

        ok &= expect(
            services.history.redo_name() ==
            "Loop Cut",
            "entrada de redo preserva nome");

        const CommandResult redoResult =
            services.history.redo(
                services.dispatcher);

        ok &= expect(
            redoResult.success,
            "redo do loop cut funciona");

        ok &= expect(
            active_vertex_count(
                node->mesh()) ==
            originalVertices + 2,
            "redo restaura vertices de corte");

        ok &= expect(
            active_face_count(
                node->mesh()) == 2,
            "redo restaura divisao do quad");

        return ok;
    }

    bool test_multiple_even_cuts()
    {
        std::cout
            << "\n=== Multiple evenly spaced cuts ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_opposite_edges(
            editor,
            fixture);

        ToolServices services{ editor };

        LoopCutToolOptions options{};

        options.cuts =
            2;

        options.evenSpacing =
            true;

        options.initialFactor =
            0.2f;

        LoopCutTool tool{
            options
        };

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

        const std::size_t originalVertices =
            active_vertex_count(
                node->mesh());

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        ok &= expect(
            tool.has_operation_preview(),
            "dois cortes geram preview pronto");

        const float factorBeforeMove =
            tool.factor();

        const ToolResult moveResult =
            tool.handle_event(
                services.context,
                make_pointer_move(
                    glm::vec2{
                        100.0f,
                        0.0f
                    }));

        ok &= expect(
            moveResult.code ==
            ToolResultCode::Ignored,
            "multiple cuts ignoram movimento de posicionamento");

        ok &= expect(
            nearly_equal(
                tool.factor(),
                factorBeforeMove),
            "multiple cuts preservam factor interno");

        const ToolResult releaseResult =
            tool.handle_event(
                services.context,
                make_pointer_release(
                    glm::vec2{
                        100.0f,
                        0.0f
                    }));

        ok &= expect(
            releaseResult.code ==
            ToolResultCode::Confirmed,
            "multiple cuts confirmam");

        ok &= expect(
            active_vertex_count(
                node->mesh()) ==
            originalVertices + 4,
            "dois cortes adicionam quatro vertices");

        const std::size_t resultingFaceCount =
            active_face_count(
                node->mesh());

        std::cout
            << "faces apos dois cortes: "
            << resultingFaceCount
            << '\n';

        ok &= expect(
            resultingFaceCount == 3,
            "dois cortes dividem o quad em tres faces");

        ok &= expect(
            services.history.undo_size() == 1,
            "multiple cuts criam uma entrada de historico");

        return ok;
    }

    bool test_single_edge_only()
    {
        std::cout
            << "\n=== Single selected edge ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_single_edge(
            editor,
            fixture);

        ToolServices services{ editor };

        LoopCutToolOptions options{};

        options.cuts =
            1;

        options.evenSpacing =
            false;

        options.initialFactor =
            0.5f;

        LoopCutTool tool{
            options
        };

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

        const std::size_t originalVertices =
            active_vertex_count(
                node->mesh());

        const std::size_t originalFaces =
            active_face_count(
                node->mesh());

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        const ToolResult releaseResult =
            tool.handle_event(
                services.context,
                make_pointer_release(
                    glm::vec2{
                        0.0f,
                        0.0f
                    }));

        ok &= expect(
            releaseResult.code ==
            ToolResultCode::Confirmed,
            "uma aresta selecionada ainda executa corte");

        ok &= expect(
            active_vertex_count(
                node->mesh()) ==
            originalVertices + 1,
            "uma aresta cria um vertice de corte");

        ok &= expect(
            active_face_count(
                node->mesh()) ==
            originalFaces,
            "um unico vertice de corte nao divide a face");

        ok &= expect(
            services.history.undo_size() == 1,
            "subdivisao de uma aresta cria historico");

        return ok;
    }

    bool test_factor_clamping_and_direction()
    {
        std::cout
            << "\n=== Factor clamping and direction ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_opposite_edges(
            editor,
            fixture);

        ToolServices services{ editor };

        LoopCutToolOptions options{};

        options.factorPerPixel =
            0.01f;

        options.minimumFactor =
            0.2f;

        options.maximumFactor =
            0.8f;

        options.initialFactor =
            0.5f;

        options.cuts =
            1;

        options.evenSpacing =
            false;

        options.invertDragDirection =
            true;

        LoopCutTool tool{
            options
        };

        tool.activate(
            services.context);

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    100.0f,
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
                tool.factor(),
                0.8f),
            "visualScale, direcao invertida e clamp afetam fator");

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    200.0f,
                    0.0f
                }));

        ok &= expect(
            nearly_equal(
                tool.factor(),
                0.2f),
            "fator tambem e limitado pelo minimo");

        tool.cancel(
            services.context);

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

        select_opposite_edges(
            editor,
            fixture);

        ToolServices services{ editor };
        LoopCutTool tool{};

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

        const std::size_t originalVertices =
            active_vertex_count(
                node->mesh());

        const std::size_t originalEdges =
            active_edge_count(
                node->mesh());

        const std::size_t originalFaces =
            active_face_count(
                node->mesh());

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
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
            active_vertex_count(
                node->mesh()) ==
            originalVertices,
            "cancel nao altera vertices");

        ok &= expect(
            active_edge_count(
                node->mesh()) ==
            originalEdges,
            "cancel nao altera arestas");

        ok &= expect(
            active_face_count(
                node->mesh()) ==
            originalFaces,
            "cancel nao altera faces");

        ok &= expect(
            services.history.empty(),
            "cancel nao cria historico");

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

        select_opposite_edges(
            editor,
            firstFixture);

        ToolServices services{ editor };
        LoopCutTool tool{};

        tool.activate(
            services.context);

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        select_opposite_edges(
            editor,
            secondFixture);

        const ToolResult releaseResult =
            tool.handle_event(
                services.context,
                make_pointer_release(
                    glm::vec2{
                        0.0f,
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
            "loop cut confirma apos mudanca de selecao");

        ok &= expect(
            firstNode != nullptr &&
            active_face_count(
                firstNode->mesh()) == 2,
            "loop cut modifica node capturado");

        ok &= expect(
            secondNode != nullptr &&
            active_face_count(
                secondNode->mesh()) == 1,
            "loop cut nao modifica node selecionado depois");

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

        select_opposite_edges(
            editor,
            fixture);

        ToolContext context{
            editor
        };

        LoopCutTool tool{};

        tool.activate(
            context);

        tool.handle_event(
            context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        const ToolResult releaseResult =
            tool.handle_event(
                context,
                make_pointer_release(
                    glm::vec2{
                        0.0f,
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
        << "=== Locus3D Editor LoopCutTool "
        << "Smoke Test ===\n";

    bool ok = true;

    ok &= test_fixture();
    ok &= test_activation_and_options();
    ok &= test_interactive_preview_is_non_destructive();
    ok &= test_single_cut_commit_and_history();
    ok &= test_multiple_even_cuts();
    ok &= test_single_edge_only();
    ok &= test_factor_clamping_and_direction();
    ok &= test_cancel_does_not_commit();
    ok &= test_stable_captured_target();
    ok &= test_missing_command_services();

    std::cout
        << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes do "
            << "LoopCutTool falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes do "
        << "LoopCutTool passaram.\n";

    return EXIT_SUCCESS;
}
