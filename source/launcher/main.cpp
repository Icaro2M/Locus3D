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
#include "editor/tools/mesh/edge/BevelTool.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {

    using locus::editor::BevelTool;
    using locus::editor::BevelToolOptions;
    using locus::editor::CommandDispatcher;
    using locus::editor::CommandResult;
    using locus::editor::Editor;
    using locus::editor::EditorDirtyFlags;
    using locus::editor::EditorMode;
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
        const std::string& name = "Bevel Fixture")
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
         * bottomEdge is the bevel target.
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

    bool contains_vertex_position(
        const LEM& mesh,
        const glm::vec3& expectedPosition)
    {
        const std::vector<VertexHandle> vertices =
            TopologyTraversal::vertices(mesh);

        for (const VertexHandle vertex : vertices) {
            if (!mesh.is_valid(vertex)) {
                continue;
            }

            if (nearly_equal(
                mesh.vertex(vertex).position,
                expectedPosition)) {
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

        BevelTool tool{};

        ok &= expect(
            tool.state() ==
            ToolState::Inactive,
            "BevelTool comeca Inactive");

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
            "descriptor do bevel e valido");

        ok &= expect(
            tool.descriptor().id.value ==
            std::string{
                BevelTool::Id
            },
            "descriptor usa id estavel do bevel");

        BevelToolOptions options =
            tool.options();

        options.widthPerPixel =
            0.02f;

        options.widthEpsilon =
            0.0001f;

        options.maximumWidth =
            0.75f;

        options.invertDragDirection =
            true;

        ok &= expect(
            tool.set_options(options),
            "opcoes podem mudar fora da interacao");

        ok &= expect(
            nearly_equal(
                tool.options().widthPerPixel,
                0.02f),
            "widthPerPixel foi atualizado");

        ok &= expect(
            nearly_equal(
                tool.options().maximumWidth,
                0.75f),
            "maximumWidth foi atualizado");

        ok &= expect(
            tool.options().invertDragDirection,
            "invertDragDirection foi atualizado");

        BevelToolOptions invalidOptions{};

        invalidOptions.widthPerPixel =
            -1.0f;

        invalidOptions.widthEpsilon =
            -0.5f;

        invalidOptions.maximumWidth =
            -2.0f;

        ok &= expect(
            tool.set_options(
                invalidOptions),
            "opcoes invalidas sao sanitizadas");

        ok &= expect(
            nearly_equal(
                tool.options().widthPerPixel,
                0.0f),
            "widthPerPixel negativo e limitado a zero");

        ok &= expect(
            nearly_equal(
                tool.options().widthEpsilon,
                0.0f),
            "widthEpsilon negativo e limitado a zero");

        ok &= expect(
            nearly_equal(
                tool.options().maximumWidth,
                0.0f),
            "maximumWidth negativo e limitado a zero");

        tool.deactivate(
            services.context);

        return ok;
    }

    bool test_preview_is_non_destructive()
    {
        std::cout
            << "\n=== Non-destructive bevel preview ===\n";

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
        BevelTool tool{};

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

        const glm::vec3 originalPosition0 =
            node->mesh()
            .vertex(fixture.vertex0)
            .position;

        const glm::vec3 originalPosition1 =
            node->mesh()
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
            "pointer press inicia bevel");

        ok &= expect(
            tool.state() ==
            ToolState::Interacting,
            "tool entra em Interacting");

        ok &= expect(
            tool.mesh_session().is_active(),
            "bevel inicia MeshOperationSession");

        ok &= expect(
            nearly_equal(
                tool.width(),
                0.0f),
            "largura inicial e zero");

        ok &= expect(
            !tool.has_operation_preview(),
            "largura inicial zero nao gera preview pronto");

        ok &= expect(
            tool.operation_preview().is_empty(),
            "preview inicial do bevel e Empty");

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
            "pointer move atualiza bevel");

        ok &= expect(
            nearly_equal(
                tool.width(),
                0.5f),
            "arrasto de 50 pixels gera largura 0.5");

        ok &= expect(
            tool.has_operation_preview(),
            "largura valida gera preview pronto");

        ok &= expect(
            tool.operation_preview().is_ready(),
            "OperationPreview do bevel fica Ready");

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
            "preview nao altera posicoes autoritativas");

        ok &= expect(
            services.history.empty(),
            "preview nao cria entrada no historico");

        ok &= expect(
            !tool.set_options(
                BevelToolOptions{}),
            "opcoes nao mudam durante interacao");

        tool.cancel(
            services.context);

        return ok;
    }

    bool test_commit_and_history()
    {
        std::cout
            << "\n=== Commit, undo and redo ===\n";

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
        BevelTool tool{};

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
                tool.width(),
                0.5f),
            "arrasto configura largura final 0.5");

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
            "pointer release confirma bevel");

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

        const LEM& beveledMesh =
            node->mesh();

        ok &= expect(
            active_vertex_count(
                beveledMesh) ==
            originalVertices + 4,
            "bevel de uma aresta adiciona quatro vertices de corte");

        ok &= expect(
            active_face_count(
                beveledMesh) == 3,
            "bevel produz face reconstruida e duas faces de chanfro");

        ok &= expect(
            active_edge_count(
                beveledMesh) >
            originalEdges,
            "bevel aumenta quantidade de arestas");

        ok &= expect(
            active_loop_count(
                beveledMesh) >
            originalLoops,
            "bevel aumenta quantidade de loops");

        ok &= expect(
            !beveledMesh.is_valid(
                fixture.face),
            "face original e substituida");

        ok &= expect(
            beveledMesh.is_valid(
                fixture.vertex0) &&
            beveledMesh.is_valid(
                fixture.vertex1) &&
            beveledMesh.is_valid(
                fixture.vertex2) &&
            beveledMesh.is_valid(
                fixture.vertex3),
            "vertices originais permanecem validos");

        ok &= expect(
            contains_vertex_position(
                beveledMesh,
                glm::vec3{
                    -1.0f,
                    -0.5f,
                    0.0f
                }),
            "bevel cria corte de vertex0 em direcao a vertex3");

        ok &= expect(
            contains_vertex_position(
                beveledMesh,
                glm::vec3{
                    -0.5f,
                    -1.0f,
                    0.0f
                }),
            "bevel cria corte de vertex0 em direcao a vertex1");

        ok &= expect(
            contains_vertex_position(
                beveledMesh,
                glm::vec3{
                    0.5f,
                    -1.0f,
                    0.0f
                }),
            "bevel cria corte de vertex1 em direcao a vertex0");

        ok &= expect(
            contains_vertex_position(
                beveledMesh,
                glm::vec3{
                    1.0f,
                    -0.5f,
                    0.0f
                }),
            "bevel cria corte de vertex1 em direcao a vertex2");

        ok &= expect(
            services.history.can_undo(),
            "commit cria entrada de undo");

        ok &= expect(
            services.history.undo_size() == 1,
            "historico possui uma entrada");

        ok &= expect(
            services.history.undo_name() ==
            "Bevel Edges",
            "entrada possui nome Bevel Edges");

        ok &= expect(
            !services.history.can_redo(),
            "redo nao esta disponivel antes do undo");

        const CommandResult undoResult =
            services.history.undo(
                services.dispatcher);

        ok &= expect(
            undoResult.success,
            "undo do bevel funciona");

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
            "Bevel Edges",
            "entrada de redo preserva nome");

        const CommandResult redoResult =
            services.history.redo(
                services.dispatcher);

        ok &= expect(
            redoResult.success,
            "redo do bevel funciona");

        ok &= expect(
            active_vertex_count(
                node->mesh()) ==
            originalVertices + 4,
            "redo restaura vertices de corte");

        ok &= expect(
            active_face_count(
                node->mesh()) == 3,
            "redo restaura topologia chanfrada");

        ok &= expect(
            !node->mesh().is_valid(
                fixture.face),
            "redo substitui novamente a face original");

        return ok;
    }

    bool test_kernel_width_limit()
    {
        std::cout
            << "\n=== Kernel width limit ===\n";

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

        BevelToolOptions options{};

        options.widthPerPixel =
            0.1f;

        /*
         * No explicit tool-side maximum. A very large requested width should
         * still be limited locally by BevelOp.
         */
        options.maximumWidth =
            0.0f;

        BevelTool tool{
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
                }));

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    100.0f,
                    0.0f
                }));

        ok &= expect(
            nearly_equal(
                tool.width(),
                10.0f),
            "tool preserva largura solicitada sem maximumWidth");

        const ToolResult result =
            tool.handle_event(
                services.context,
                make_pointer_release(
                    glm::vec2{
                        100.0f,
                        0.0f
                    }));

        const MeshNode* node =
            editor.scene().find_mesh(
                fixture.nodeId);

        ok &= expect(
            result.code ==
            ToolResultCode::Confirmed,
            "largura grande ainda confirma");

        ok &= expect(
            node != nullptr,
            "node continua disponivel");

        if (node) {
            /*
             * Each adjacent source edge has length 2.0, so BevelOp limits each
             * local cut to 0.9.
             */
            ok &= expect(
                contains_vertex_position(
                    node->mesh(),
                    glm::vec3{
                        -1.0f,
                        -0.1f,
                        0.0f
                    }),
                "kernel limita corte lateral a 45 por cento");

            ok &= expect(
                contains_vertex_position(
                    node->mesh(),
                    glm::vec3{
                        -0.1f,
                        -1.0f,
                        0.0f
                    }),
                "kernel limita corte horizontal a 45 por cento");
        }

        return ok;
    }

    bool test_tool_width_clamp()
    {
        std::cout
            << "\n=== Tool width clamp ===\n";

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

        BevelToolOptions options{};

        options.widthPerPixel =
            0.01f;

        options.maximumWidth =
            0.25f;

        BevelTool tool{
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
                }));

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    100.0f,
                    0.0f
                }));

        ok &= expect(
            nearly_equal(
                tool.width(),
                0.25f),
            "maximumWidth limita largura interativa");

        ok &= expect(
            tool.has_operation_preview(),
            "largura limitada produz preview pronto");

        tool.cancel(
            services.context);

        return ok;
    }

    bool test_opposite_direction_returns_to_zero()
    {
        std::cout
            << "\n=== Opposite drag direction ===\n";

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
        BevelTool tool{};

        tool.activate(
            services.context);

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    100.0f,
                    0.0f
                }));

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    50.0f,
                    0.0f
                }));

        ok &= expect(
            nearly_equal(
                tool.width(),
                0.0f),
            "arrasto oposto e limitado a largura zero");

        ok &= expect(
            tool.operation_preview().is_empty(),
            "largura zero produz preview Empty");

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
            "largura zero conclui como no-op");

        ok &= expect(
            services.history.empty(),
            "largura zero nao cria historico");

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

        BevelToolOptions options{};

        options.widthPerPixel =
            0.01f;

        options.invertDragDirection =
            true;

        BevelTool tool{
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
                tool.width(),
                1.0f),
            "visualScale e invertDragDirection afetam largura");

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

        select_bottom_edge(
            editor,
            fixture);

        ToolServices services{ editor };
        BevelTool tool{};

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

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    50.0f,
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
            active_face_count(
                node->mesh()) ==
            originalFaces,
            "cancel nao altera faces");

        ok &= expect(
            node->mesh().is_valid(
                fixture.face),
            "cancel preserva face original");

        ok &= expect(
            services.history.empty(),
            "cancel nao cria historico");

        return ok;
    }

    bool test_zero_width_confirmation()
    {
        std::cout
            << "\n=== Zero-width confirmation ===\n";

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
        BevelTool tool{};

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
            active_vertex_count(
                node->mesh()) ==
            originalVertices,
            "largura zero nao cria vertices");

        ok &= expect(
            active_face_count(
                node->mesh()) ==
            originalFaces,
            "largura zero nao muda faces");

        ok &= expect(
            services.history.empty(),
            "largura zero nao cria entrada no historico");

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
        BevelTool tool{};

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
            "bevel confirma apos mudanca de selecao");

        ok &= expect(
            firstNode != nullptr &&
            active_face_count(
                firstNode->mesh()) == 3,
            "bevel modifica node capturado");

        ok &= expect(
            secondNode != nullptr &&
            active_face_count(
                secondNode->mesh()) == 1,
            "bevel nao modifica node selecionado depois");

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

        BevelTool tool{};

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
        << "=== Locus3D Editor BevelTool "
        << "Smoke Test ===\n";

    bool ok = true;

    ok &= test_fixture();
    ok &= test_activation_and_options();
    ok &= test_preview_is_non_destructive();
    ok &= test_commit_and_history();
    ok &= test_kernel_width_limit();
    ok &= test_tool_width_clamp();
    ok &= test_opposite_direction_returns_to_zero();
    ok &= test_visual_scale_and_direction();
    ok &= test_cancel_does_not_commit();
    ok &= test_zero_width_confirmation();
    ok &= test_stable_captured_target();
    ok &= test_missing_command_services();

    std::cout
        << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes do "
            << "BevelTool falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes do "
        << "BevelTool passaram.\n";

    return EXIT_SUCCESS;
}