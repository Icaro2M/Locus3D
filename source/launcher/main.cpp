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
#include "editor/tools/mesh/face/ExtrudeFaceTool.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/geometric.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

    using locus::editor::CommandDispatcher;
    using locus::editor::CommandResult;
    using locus::editor::Editor;
    using locus::editor::EditorDirtyFlags;
    using locus::editor::EditorMode;
    using locus::editor::ExtrudeFaceTool;
    using locus::editor::ExtrudeFaceToolOptions;
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
        const std::string& name = "Extrude Face Fixture")
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

        ExtrudeFaceTool tool{};

        ok &= expect(
            tool.state() ==
            ToolState::Inactive,
            "ExtrudeFaceTool comeca Inactive");

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
            "descriptor da extrusao e valido");

        ok &= expect(
            tool.descriptor().id.value ==
            std::string{
                ExtrudeFaceTool::Id
            },
            "descriptor usa id estavel da extrusao");

        ExtrudeFaceToolOptions options =
            tool.options();

        options.distancePerPixel =
            0.05f;

        options.distanceEpsilon =
            0.0001f;

        options.keepSourceFace =
            true;

        ok &= expect(
            tool.set_options(options),
            "opcoes podem mudar fora da interacao");

        ok &= expect(
            nearly_equal(
                tool.options().distancePerPixel,
                0.05f),
            "distancePerPixel foi atualizado");

        ok &= expect(
            tool.options().keepSourceFace,
            "keepSourceFace foi atualizado");

        tool.deactivate(
            services.context);

        return ok;
    }

    bool test_preview_is_non_destructive()
    {
        std::cout
            << "\n=== Non-destructive extrusion preview ===\n";

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
        ExtrudeFaceTool tool{};

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

        const glm::vec3 originalPosition =
            originalMesh
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
            "pointer press inicia extrusao");

        ok &= expect(
            tool.state() ==
            ToolState::Interacting,
            "tool entra em Interacting");

        ok &= expect(
            tool.mesh_session().is_active(),
            "extrusao inicia MeshOperationSession");

        ok &= expect(
            !tool.has_operation_preview(),
            "distancia inicial zero nao gera preview visivel");

        ok &= expect(
            tool.operation_preview().is_empty(),
            "preview inicial da extrusao e Empty");

        const ToolResult moveResult =
            tool.handle_event(
                services.context,
                make_pointer_move(
                    glm::vec2{
                        100.0f,
                        50.0f
                    },
                    glm::vec2{
                        0.0f,
                        -50.0f
                    }));

        ok &= expect(
            moveResult.code ==
            ToolResultCode::Updated,
            "pointer move atualiza extrusao");

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
            "OperationPreview da extrusao fica Ready");

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
                originalPosition),
            "preview nao altera posicoes autoritativas");

        ok &= expect(
            services.history.empty(),
            "preview nao cria entrada no historico");

        ok &= expect(
            !tool.set_options(
                ExtrudeFaceToolOptions{}),
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
        ExtrudeFaceTool tool{};

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
                tool.distance(),
                1.0f),
            "arrasto configura distancia final 1.0");

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
            "pointer release confirma extrusao");

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

        const LEM& extrudedMesh =
            node->mesh();

        ok &= expect(
            active_vertex_count(
                extrudedMesh) ==
            originalVertices + 4,
            "extrusao adiciona quatro vertices");

        ok &= expect(
            active_face_count(
                extrudedMesh) ==
            5,
            "quad extrudado possui uma tampa e quatro laterais");

        ok &= expect(
            active_face_count(
                extrudedMesh) >
            originalFaces,
            "extrusao aumenta quantidade de faces");

        ok &= expect(
            active_edge_count(
                extrudedMesh) >
            originalEdges,
            "extrusao aumenta quantidade de arestas");

        ok &= expect(
            active_loop_count(
                extrudedMesh) >
            originalLoops,
            "extrusao aumenta quantidade de loops");

        ok &= expect(
            !extrudedMesh.is_valid(
                fixture.face),
            "face original e removida por padrao");

        ok &= expect(
            services.history.can_undo(),
            "commit cria entrada de undo");

        ok &= expect(
            services.history.undo_size() == 1,
            "historico possui uma entrada");

        ok &= expect(
            !services.history.can_redo(),
            "redo nao esta disponivel antes do undo");

        ok &= expect(
            services.history.undo_name() ==
            "Extrude Faces",
            "entrada possui nome Extrude Faces");

        const CommandResult undoResult =
            services.history.undo(
                services.dispatcher);

        ok &= expect(
            undoResult.success,
            "undo da extrusao funciona");

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
            "Extrude Faces",
            "entrada de redo preserva nome");

        const CommandResult redoResult =
            services.history.redo(
                services.dispatcher);

        ok &= expect(
            redoResult.success,
            "redo da extrusao funciona");

        ok &= expect(
            active_vertex_count(
                node->mesh()) ==
            originalVertices + 4,
            "redo restaura vertices extrudados");

        ok &= expect(
            active_face_count(
                node->mesh()) ==
            5,
            "redo restaura topologia extrudada");

        ok &= expect(
            !node->mesh().is_valid(
                fixture.face),
            "redo remove novamente a face original");

        ok &= expect(
            services.history.can_undo(),
            "undo volta a ficar disponivel apos redo");

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
        ExtrudeFaceTool tool{};

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

        select_face(
            editor,
            fixture);

        ToolServices services{ editor };
        ExtrudeFaceTool tool{};

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
            "distancia zero nao cria vertices");

        ok &= expect(
            active_face_count(
                node->mesh()) ==
            originalFaces,
            "distancia zero nao muda faces");

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

        select_face(
            editor,
            fixture);

        ToolServices services{ editor };

        ExtrudeFaceToolOptions options{};

        options.distancePerPixel =
            0.01f;

        options.invertDragDirection =
            true;

        ExtrudeFaceTool tool{
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
                    50.0f
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

    bool test_keep_source_face()
    {
        std::cout
            << "\n=== Keep source face ===\n";

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

        ExtrudeFaceToolOptions options{};

        options.keepSourceFace =
            true;

        ExtrudeFaceTool tool{
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
                }));

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

        const MeshNode* node =
            editor.scene().find_mesh(
                fixture.nodeId);

        ok &= expect(
            result.code ==
            ToolResultCode::Confirmed,
            "extrusao com keepSourceFace confirma");

        ok &= expect(
            node != nullptr,
            "node continua disponivel");

        if (node) {
            ok &= expect(
                node->mesh().is_valid(
                    fixture.face),
                "keepSourceFace preserva face original");

            ok &= expect(
                active_face_count(
                    node->mesh()) ==
                6,
                "keepSourceFace produz original, tampa e quatro laterais");
        }

        ok &= expect(
            services.history.undo_size() == 1,
            "keepSourceFace ainda cria uma entrada de historico");

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

        ExtrudeFaceTool tool{};

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
        << "=== Locus3D Editor ExtrudeFaceTool "
        << "Smoke Test ===\n";

    bool ok = true;

    ok &= test_activation_and_options();
    ok &= test_preview_is_non_destructive();
    ok &= test_commit_and_history();
    ok &= test_cancel_does_not_commit();
    ok &= test_zero_distance_confirmation();
    ok &= test_visual_scale_and_direction();
    ok &= test_keep_source_face();
    ok &= test_missing_command_services();

    std::cout
        << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes do "
            << "ExtrudeFaceTool falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes do "
        << "ExtrudeFaceTool passaram.\n";

    return EXIT_SUCCESS;
}