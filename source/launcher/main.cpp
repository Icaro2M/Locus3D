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
#include "editor/tools/mesh/face/InsetFaceTool.h"
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
    using locus::editor::HistoryStack;
    using locus::editor::InsetFaceTool;
    using locus::editor::InsetFaceToolOptions;
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
        const std::string& name = "Inset Face Fixture")
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

        return fixture;
    }

    void select_face(
        Editor& editor,
        const QuadFixture& fixture)
    {
        auto& selection =
            editor.selection().mesh();

        selection.set_active_mesh(
            fixture.nodeId);

        selection.set_face(
            fixture.face);
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

    bool test_activation_and_options()
    {
        std::cout
            << "\n=== Activation and options ===\n";

        bool ok = true;

        Editor editor{};
        ToolServices services{ editor };

        InsetFaceTool tool{};

        ok &= expect(
            tool.state() ==
            ToolState::Inactive,
            "InsetFaceTool comeca Inactive");

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
            "descriptor do inset e valido");

        ok &= expect(
            tool.descriptor().id.value ==
            std::string{
                InsetFaceTool::Id
            },
            "descriptor usa id estavel do inset");

        InsetFaceToolOptions options =
            tool.options();

        options.factorPerPixel =
            0.01f;

        options.factorEpsilon =
            0.0001f;

        options.maximumFactor =
            0.8f;

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
                tool.options().maximumFactor,
                0.8f),
            "maximumFactor foi atualizado");

        InsetFaceToolOptions invalidOptions{};

        invalidOptions.factorPerPixel =
            -1.0f;

        invalidOptions.factorEpsilon =
            -0.5f;

        invalidOptions.maximumFactor =
            2.0f;

        ok &= expect(
            tool.set_options(
                invalidOptions),
            "opcoes invalidas sao aceitas e sanitizadas");

        ok &= expect(
            nearly_equal(
                tool.options().factorPerPixel,
                0.0f),
            "factorPerPixel negativo e limitado a zero");

        ok &= expect(
            tool.options().factorEpsilon >= 0.0f,
            "factorEpsilon e mantido nao negativo");

        ok &= expect(
            tool.options().maximumFactor < 1.0f,
            "maximumFactor e mantido abaixo de um");

        tool.deactivate(
            services.context);

        return ok;
    }

    bool test_preview_is_non_destructive()
    {
        std::cout
            << "\n=== Non-destructive inset preview ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_face(
            editor,
            fixture);

        ToolServices services{ editor };
        InsetFaceTool tool{};

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

        const glm::vec3 originalPosition =
            node->mesh()
            .vertex(fixture.vertex0)
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
            "pointer press inicia inset");

        ok &= expect(
            tool.state() ==
            ToolState::Interacting,
            "tool entra em Interacting");

        ok &= expect(
            tool.mesh_session().is_active(),
            "inset inicia MeshOperationSession");

        ok &= expect(
            nearly_equal(
                tool.factor(),
                0.0f),
            "fator inicial e zero");

        ok &= expect(
            !tool.has_operation_preview(),
            "fator inicial zero nao gera preview visivel");

        ok &= expect(
            tool.operation_preview().is_empty(),
            "preview inicial do inset e Empty");

        const ToolResult moveResult =
            tool.handle_event(
                services.context,
                make_pointer_move(
                    glm::vec2{
                        100.0f,
                        0.0f
                    },
                    glm::vec2{
                        0.0f,
                        -100.0f
                    }));

        ok &= expect(
            moveResult.code ==
            ToolResultCode::Updated,
            "pointer move atualiza inset");

        ok &= expect(
            nearly_equal(
                tool.factor(),
                0.5f),
            "arrasto de 100 pixels gera fator 0.5");

        ok &= expect(
            tool.has_operation_preview(),
            "fator valido gera preview pronto");

        ok &= expect(
            tool.operation_preview().is_ready(),
            "OperationPreview do inset fica Ready");

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
                originalPosition),
            "preview nao altera posicoes autoritativas");

        ok &= expect(
            services.history.empty(),
            "preview nao cria entrada no historico");

        ok &= expect(
            !tool.set_options(
                InsetFaceToolOptions{}),
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

        select_face(
            editor,
            fixture);

        ToolServices services{ editor };
        InsetFaceTool tool{};

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

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    100.0f,
                    0.0f
                }));

        ok &= expect(
            nearly_equal(
                tool.factor(),
                0.5f),
            "arrasto configura fator final 0.5");

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
            "pointer release confirma inset");

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

        const LEM& insetMesh =
            node->mesh();

        ok &= expect(
            active_vertex_count(
                insetMesh) ==
            originalVertices + 4,
            "inset adiciona quatro vertices internos");

        ok &= expect(
            active_face_count(
                insetMesh) == 5,
            "quad insetado possui face interna e quatro faces externas");

        ok &= expect(
            active_face_count(
                insetMesh) >
            originalFaces,
            "inset aumenta quantidade de faces");

        ok &= expect(
            active_edge_count(
                insetMesh) >
            originalEdges,
            "inset aumenta quantidade de arestas");

        ok &= expect(
            active_loop_count(
                insetMesh) >
            originalLoops,
            "inset aumenta quantidade de loops");

        ok &= expect(
            !insetMesh.is_valid(
                fixture.face),
            "face original e substituida pelo inset");

        ok &= expect(
            insetMesh.is_valid(
                fixture.vertex0) &&
            insetMesh.is_valid(
                fixture.vertex1) &&
            insetMesh.is_valid(
                fixture.vertex2) &&
            insetMesh.is_valid(
                fixture.vertex3),
            "vertices externos originais permanecem validos");

        ok &= expect(
            services.history.can_undo(),
            "commit cria entrada de undo");

        ok &= expect(
            services.history.undo_size() == 1,
            "historico possui uma entrada");

        ok &= expect(
            services.history.undo_name() ==
            "Inset Faces",
            "entrada possui nome Inset Faces");

        ok &= expect(
            !services.history.can_redo(),
            "redo nao esta disponivel antes do undo");

        const CommandResult undoResult =
            services.history.undo(
                services.dispatcher);

        ok &= expect(
            undoResult.success,
            "undo do inset funciona");

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
            "Inset Faces",
            "entrada de redo preserva nome");

        const CommandResult redoResult =
            services.history.redo(
                services.dispatcher);

        ok &= expect(
            redoResult.success,
            "redo do inset funciona");

        ok &= expect(
            active_vertex_count(
                node->mesh()) ==
            originalVertices + 4,
            "redo restaura vertices internos");

        ok &= expect(
            active_face_count(
                node->mesh()) == 5,
            "redo restaura topologia insetada");

        ok &= expect(
            !node->mesh().is_valid(
                fixture.face),
            "redo substitui novamente a face original");

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

        select_face(
            editor,
            fixture);

        ToolServices services{ editor };
        InsetFaceTool tool{};

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
                    100.0f
                }));

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    0.0f,
                    20.0f
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

    bool test_zero_factor_confirmation()
    {
        std::cout
            << "\n=== Zero-factor confirmation ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_face(
            editor,
            fixture);

        ToolServices services{ editor };
        InsetFaceTool tool{};

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
            "fator zero nao cria vertices");

        ok &= expect(
            active_face_count(
                node->mesh()) ==
            originalFaces,
            "fator zero nao muda faces");

        ok &= expect(
            services.history.empty(),
            "fator zero nao cria entrada no historico");

        return ok;
    }

    bool test_factor_clamping()
    {
        std::cout
            << "\n=== Factor clamping ===\n";

        bool ok = true;

        Editor editor{};

        editor.set_mode(
            EditorMode::Mesh);

        const QuadFixture fixture =
            create_quad(editor);

        select_face(
            editor,
            fixture);

        ToolServices services{ editor };

        InsetFaceToolOptions options{};

        options.factorPerPixel =
            0.01f;

        options.maximumFactor =
            0.75f;

        InsetFaceTool tool{
            options
        };

        tool.activate(
            services.context);

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    200.0f
                }));

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        ok &= expect(
            nearly_equal(
                tool.factor(),
                0.75f),
            "fator e limitado por maximumFactor");

        ok &= expect(
            tool.has_operation_preview(),
            "fator limitado ainda produz preview valido");

        tool.cancel(
            services.context);

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

        select_face(
            editor,
            fixture);

        ToolServices services{ editor };

        InsetFaceToolOptions options{};

        options.factorPerPixel =
            0.005f;

        options.maximumFactor =
            0.95f;

        options.invertDragDirection =
            true;

        InsetFaceTool tool{
            options
        };

        tool.activate(
            services.context);

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    100.0f
                },
                2.0f));

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    0.0f,
                    150.0f
                }));

        ok &= expect(
            nearly_equal(
                tool.factor(),
                0.5f),
            "visualScale e invertDragDirection afetam fator");

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    0.0f,
                    50.0f
                }));

        ok &= expect(
            nearly_equal(
                tool.factor(),
                0.0f),
            "arrasto no sentido oposto e limitado a zero");

        ok &= expect(
            tool.operation_preview().is_empty(),
            "fator retornando a zero produz preview Empty");

        tool.cancel(
            services.context);

        return ok;
    }

    bool test_multiple_faces()
    {
        std::cout
            << "\n=== Multiple selected faces ===\n";

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

        /*
         * MeshToolTarget supports only one active mesh node. The second fixture
         * exists only to verify that selection from another node does not become
         * part of the captured target.
         */
        auto& selection =
            editor.selection().mesh();

        selection.set_active_mesh(
            firstFixture.nodeId);

        selection.set_face(
            firstFixture.face);

        ToolServices services{ editor };
        InsetFaceTool tool{};

        tool.activate(
            services.context);

        tool.handle_event(
            services.context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    100.0f
                }));

        selection.set_active_mesh(
            secondFixture.nodeId);

        selection.set_face(
            secondFixture.face);

        tool.handle_event(
            services.context,
            make_pointer_move(
                glm::vec2{
                    0.0f,
                    0.0f
                }));

        const ToolResult result =
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
            result.code ==
            ToolResultCode::Confirmed,
            "inset confirma apos mudanca de selecao");

        ok &= expect(
            firstNode != nullptr &&
            active_face_count(
                firstNode->mesh()) == 5,
            "inset modifica o node capturado");

        ok &= expect(
            secondNode != nullptr &&
            active_face_count(
                secondNode->mesh()) == 1,
            "inset nao modifica o node selecionado depois");

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

        select_face(
            editor,
            fixture);

        ToolContext context{
            editor
        };

        InsetFaceTool tool{};

        tool.activate(
            context);

        tool.handle_event(
            context,
            make_pointer_press(
                glm::vec2{
                    0.0f,
                    100.0f
                }));

        tool.handle_event(
            context,
            make_pointer_move(
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
        << "=== Locus3D Editor InsetFaceTool "
        << "Smoke Test ===\n";

    bool ok = true;

    ok &= test_activation_and_options();
    ok &= test_preview_is_non_destructive();
    ok &= test_commit_and_history();
    ok &= test_cancel_does_not_commit();
    ok &= test_zero_factor_confirmation();
    ok &= test_factor_clamping();
    ok &= test_visual_scale_and_direction();
    ok &= test_multiple_faces();
    ok &= test_missing_command_services();

    std::cout
        << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes do "
            << "InsetFaceTool falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes do "
        << "InsetFaceTool passaram.\n";

    return EXIT_SUCCESS;
}