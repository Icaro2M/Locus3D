/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/actions/ActionExecutor.h"
#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionContext.h"
#include "editor/actions/mesh/MeshOperationAction.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "kernel/geometry/primitives/BoxBuilder.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/face/FlipFaceOp.h"

#include <glm/geometric.hpp>

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

    using namespace locus::editor;

    constexpr const char* FlipFaceActionId =
        "mesh.face.flip.test";

    constexpr const char* EmptyOperationActionId =
        "mesh.face.empty_operation.test";

    void print_result(
        bool condition,
        const std::string& message) {
        std::cout
            << (condition ? "[OK] " : "[FAIL] ")
            << message
            << '\n';
    }

    const char* action_result_code_name(
        ActionResultCode code) {
        switch (code) {
        case ActionResultCode::Executed:
            return "Executed";

        case ActionResultCode::Unavailable:
            return "Unavailable";

        case ActionResultCode::Failed:
            return "Failed";
        }

        return "Unknown";
    }

    void print_action_result(
        const std::string& label,
        const ActionResult& result) {
        std::cout << label << '\n';
        std::cout
            << "  code: "
            << action_result_code_name(result.code)
            << '\n';
        std::cout
            << "  succeeded: "
            << (result.succeeded() ? "true" : "false")
            << '\n';
        std::cout
            << "  unavailable: "
            << (result.is_unavailable() ? "true" : "false")
            << '\n';
        std::cout
            << "  failed: "
            << (result.failed() ? "true" : "false")
            << '\n';
        std::cout
            << "  message: "
            << result.message
            << '\n';
    }

    bool approximately_equal(
        const glm::vec3& lhs,
        const glm::vec3& rhs,
        float epsilon = 0.0001f) {
        return glm::length(lhs - rhs) <= epsilon;
    }

    ActionDescriptor make_flip_face_descriptor() {
        return ActionDescriptor{
            ActionId{ FlipFaceActionId },
            "Flip Face",
            "Reverses the winding of the selected mesh face.",
            ActionCategory::Mesh,
            {
                "flip",
                "face",
                "normal",
                "winding",
                "orientation"
            }
        };
    }

    std::unique_ptr<MeshOperationAction>
        make_flip_face_action() {
        MeshOperationAction::OperationFactory operationFactory =
            [](
                const MeshToolTarget& target)
            -> ApplyMeshOperationCommand::MeshOperation {
            const std::vector<
                locus::kernel::geometry::FaceHandle>
                faces = target.faces;

            return [faces](
                locus::kernel::geometry::LEMEditor& editor) {
                    bool changed = false;

                    for (const auto face : faces) {
                        locus::kernel::modeling::FlipFaceOp operation{
                            face
                        };

                        locus::kernel::modeling::OperationContext
                            operationContext{};

                        operationContext.mesh = &editor.mesh();
                        operationContext.validateAfterExecute = true;
                        operationContext.rebuildNormals = true;
                        operationContext.allowNonManifold = true;

                        const locus::kernel::modeling::OperationResult
                            result =
                            operation.execute(operationContext);

                        if (!result.is_success()
                            || !result.changed()) {
                            return false;
                        }

                        changed = true;
                    }

                    return changed;
                };
            };

        return std::make_unique<MeshOperationAction>(
            make_flip_face_descriptor(),
            SelectionGranularity::Face,
            1u,
            std::move(operationFactory),
            "Flip Face");
    }

    std::unique_ptr<MeshOperationAction>
        make_empty_operation_action() {
        MeshOperationAction::OperationFactory operationFactory =
            [](
                const MeshToolTarget& target)
            -> ApplyMeshOperationCommand::MeshOperation {
            (void)target;
            return {};
            };

        return std::make_unique<MeshOperationAction>(
            ActionDescriptor{
                ActionId{ EmptyOperationActionId },
                "Empty Mesh Operation",
                "Creates no operation callback.",
                ActionCategory::Mesh
            },
            SelectionGranularity::Face,
            1u,
            std::move(operationFactory),
            "Empty Mesh Operation");
    }

    struct MeshFixture {
        Editor editor{};
        CommandDispatcher dispatcher{ editor };
        HistoryStack history{};
        ActionContext context{
            editor,
            dispatcher,
            history
        };

        SceneNodeId nodeId{};
        MeshNode* node = nullptr;

        locus::kernel::geometry::FaceHandle face{};
        glm::vec3 originalNormal{ 0.0f };

        bool build() {
            nodeId =
                editor.scene().create_mesh("Action Test Box");

            node =
                editor.scene().find_mesh(nodeId);

            if (!node) {
                return false;
            }

            locus::kernel::geometry::BoxParameters parameters{};
            parameters.size = {
                2.0f,
                2.0f,
                2.0f
            };

            const locus::kernel::geometry::PrimitiveBuildResult
                buildResult =
                locus::kernel::geometry::BoxBuilder::build_into(
                    node->mesh(),
                    parameters);

            if (!buildResult.success
                || buildResult.faces.empty()) {
                return false;
            }

            face = buildResult.faces.front();

            if (!node->mesh().is_valid(face)) {
                return false;
            }

            originalNormal =
                node->mesh().face(face).normal;

            editor.set_mode(EditorMode::Mesh);

            editor.selection().set_granularity(
                SelectionGranularity::Face);

            editor.selection().mesh().set_active_mesh(
                nodeId);

            editor.selection().mesh().set_face(face);
            editor.selection().mark_dirty();

            editor.clear_dirty();

            return true;
        }
    };

    bool test_fixture() {
        std::cout << "\n=== Mesh fixture ===\n";

        MeshFixture fixture{};
        const bool built = fixture.build();

        print_result(
            built,
            "fixture de cubo foi criada");

        if (!built) {
            return false;
        }

        print_result(
            fixture.nodeId.is_valid(),
            "mesh node possui id valido");

        print_result(
            fixture.node != nullptr,
            "mesh node pode ser encontrado");

        print_result(
            fixture.node->mesh().vertex_count() == 8u,
            "cubo possui oito vertices");

        print_result(
            fixture.node->mesh().edge_count() == 12u,
            "cubo possui doze arestas");

        print_result(
            fixture.node->mesh().face_count() == 6u,
            "cubo possui seis faces");

        print_result(
            fixture.node->mesh().is_valid(fixture.face),
            "face alvo e valida");

        print_result(
            fixture.editor.mode() == EditorMode::Mesh,
            "editor esta em mesh mode");

        print_result(
            fixture.editor.selection().granularity()
            == SelectionGranularity::Face,
            "granularidade atual e Face");

        print_result(
            fixture.editor.selection()
            .mesh()
            .active_mesh()
            == fixture.nodeId,
            "selecao referencia o mesh node");

        print_result(
            fixture.editor.selection()
            .mesh()
            .faces()
            .size()
            == 1u,
            "uma face esta selecionada");

        return built
            && fixture.node->mesh().vertex_count() == 8u
            && fixture.node->mesh().edge_count() == 12u
            && fixture.node->mesh().face_count() == 6u
            && fixture.node->mesh().is_valid(fixture.face);
    }

    bool test_action_metadata() {
        std::cout
            << "\n=== MeshOperationAction: metadata ===\n";

        const std::unique_ptr<MeshOperationAction> action =
            make_flip_face_action();

        print_result(
            action != nullptr,
            "action foi criada");

        if (!action) {
            return false;
        }

        print_result(
            action->descriptor().is_valid(),
            "descritor da action e valido");

        print_result(
            action->descriptor().id
            == ActionId{ FlipFaceActionId },
            "descritor preserva o identificador");

        print_result(
            action->descriptor().category
            == ActionCategory::Mesh,
            "action pertence a categoria Mesh");

        print_result(
            action->required_granularity()
            == SelectionGranularity::Face,
            "action exige granularidade Face");

        print_result(
            action->minimum_selection_count() == 1u,
            "action exige pelo menos uma face");

        print_result(
            action->command_label() == "Flip Face",
            "action preserva label do command");

        return action->descriptor().is_valid()
            && action->required_granularity()
            == SelectionGranularity::Face
            && action->minimum_selection_count() == 1u
            && action->command_label() == "Flip Face";
    }

    bool test_availability() {
        std::cout
            << "\n=== MeshOperationAction: availability ===\n";

        MeshFixture fixture{};

        if (!fixture.build()) {
            print_result(
                false,
                "fixture necessaria para disponibilidade");
            return false;
        }

        std::unique_ptr<MeshOperationAction> action =
            make_flip_face_action();

        print_result(
            action->can_execute(fixture.context),
            "action esta disponivel com face selecionada");

        fixture.editor.set_mode(EditorMode::Object);

        print_result(
            !action->can_execute(fixture.context),
            "action fica indisponivel em Object mode");

        fixture.editor.set_mode(EditorMode::Mesh);
        fixture.editor.selection().set_granularity(
            SelectionGranularity::Edge);

        print_result(
            !action->can_execute(fixture.context),
            "action fica indisponivel com granularidade Edge");

        fixture.editor.selection().set_granularity(
            SelectionGranularity::Face);

        fixture.editor.selection().mesh().clear_components();

        print_result(
            !action->can_execute(fixture.context),
            "action fica indisponivel sem face selecionada");

        fixture.editor.selection().mesh().set_face(
            fixture.face);

        print_result(
            action->can_execute(fixture.context),
            "action volta a ficar disponivel apos selecionar face");

        fixture.editor.selection().mesh().set_active_mesh(
            SceneNodeId{});

        print_result(
            !action->can_execute(fixture.context),
            "action fica indisponivel sem mesh ativo");

        return true;
    }

    bool test_execution_and_history() {
        std::cout
            << "\n=== MeshOperationAction: execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build()) {
            print_result(
                false,
                "fixture necessaria para execucao");
            return false;
        }

        ActionRegistry registry{};

        const bool registered =
            registry.register_action(
                make_flip_face_action());

        print_result(
            registered,
            "Flip Face action foi registrada");

        ActionExecutor executor{ registry };

        print_result(
            executor.can_execute(
                fixture.context,
                ActionId{ FlipFaceActionId }),
            "executor reconhece action disponivel");

        const glm::vec3 beforeNormal =
            fixture.node->mesh().face(fixture.face).normal;

        const ActionResult executionResult =
            executor.execute(
                fixture.context,
                ActionId{ FlipFaceActionId });

        print_action_result(
            "flip face execution",
            executionResult);

        const glm::vec3 afterNormal =
            fixture.node->mesh().face(fixture.face).normal;

        print_result(
            executionResult.succeeded(),
            "Flip Face action foi executada");

        print_result(
            approximately_equal(
                afterNormal,
                -beforeNormal),
            "normal da face foi invertida");

        print_result(
            fixture.node->mesh().is_valid(fixture.face),
            "face continua valida apos flip");

        print_result(
            fixture.node->mesh().face_count() == 6u,
            "flip nao altera quantidade de faces");

        print_result(
            fixture.history.undo_size() == 1u,
            "command da action entrou no historico");

        print_result(
            fixture.history.undo_name() == "Flip Face",
            "historico preserva label da action");

        print_result(
            fixture.history.can_undo(),
            "undo ficou disponivel");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Mesh),
            "execucao marca Mesh como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Render),
            "execucao marca Render como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Picking),
            "execucao marca Picking como dirty");

        const CommandResult undoResult =
            fixture.history.undo(fixture.dispatcher);

        print_result(
            undoResult.success,
            "undo da action funcionou");

        const glm::vec3 undoNormal =
            fixture.node->mesh().face(fixture.face).normal;

        print_result(
            approximately_equal(
                undoNormal,
                beforeNormal),
            "undo restaurou a normal original");

        print_result(
            fixture.history.can_redo(),
            "redo ficou disponivel");

        print_result(
            fixture.history.redo_name() == "Flip Face",
            "redo preserva label da action");

        const CommandResult redoResult =
            fixture.history.redo(fixture.dispatcher);

        print_result(
            redoResult.success,
            "redo da action funcionou");

        const glm::vec3 redoNormal =
            fixture.node->mesh().face(fixture.face).normal;

        print_result(
            approximately_equal(
                redoNormal,
                afterNormal),
            "redo restaurou o resultado do flip");

        print_result(
            fixture.editor.selection()
            .mesh()
            .active_mesh()
            == fixture.nodeId,
            "undo e redo preservam mesh ativo");

        print_result(
            fixture.editor.selection()
            .mesh()
            .faces()
            .contains(fixture.face),
            "undo e redo preservam selecao da face");

        return registered
            && executionResult.succeeded()
            && undoResult.success
            && redoResult.success
            && approximately_equal(
                redoNormal,
                -fixture.originalNormal)
            && fixture.history.undo_size() == 1u;
    }

    bool test_empty_operation_callback() {
        std::cout
            << "\n=== MeshOperationAction: empty callback ===\n";

        MeshFixture fixture{};

        if (!fixture.build()) {
            print_result(
                false,
                "fixture necessaria para callback vazio");
            return false;
        }

        ActionRegistry registry{};

        const bool registered =
            registry.register_action(
                make_empty_operation_action());

        print_result(
            registered,
            "action com factory valida foi registrada");

        ActionExecutor executor{ registry };

        print_result(
            executor.can_execute(
                fixture.context,
                ActionId{ EmptyOperationActionId }),
            "action passa pela validacao de contexto");

        const glm::vec3 normalBefore =
            fixture.node->mesh().face(fixture.face).normal;

        const ActionResult result =
            executor.execute(
                fixture.context,
                ActionId{ EmptyOperationActionId });

        print_action_result(
            "empty callback result",
            result);

        print_result(
            result.failed(),
            "callback vazio gera falha");

        print_result(
            fixture.history.empty(),
            "callback vazio nao cria entrada no historico");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .face(fixture.face)
                .normal,
                normalBefore),
            "callback vazio nao altera a malha");

        return registered
            && result.failed()
            && fixture.history.empty();
    }

    bool test_executor_unavailable() {
        std::cout
            << "\n=== MeshOperationAction: unavailable ===\n";

        MeshFixture fixture{};

        if (!fixture.build()) {
            print_result(
                false,
                "fixture necessaria para indisponibilidade");
            return false;
        }

        ActionRegistry registry{};
        registry.register_action(
            make_flip_face_action());

        ActionExecutor executor{ registry };

        fixture.editor.set_mode(EditorMode::Object);

        const ActionResult result =
            executor.execute(
                fixture.context,
                ActionId{ FlipFaceActionId });

        print_action_result(
            "unavailable flip result",
            result);

        print_result(
            result.is_unavailable(),
            "executor retorna Unavailable fora de Mesh mode");

        print_result(
            fixture.history.empty(),
            "action indisponivel nao entra no historico");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .face(fixture.face)
                .normal,
                fixture.originalNormal),
            "action indisponivel nao altera a malha");

        return result.is_unavailable()
            && fixture.history.empty();
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor Mesh Operation Action "
        "Smoke Test ===\n";

    bool passed = true;

    passed = test_fixture() && passed;
    passed = test_action_metadata() && passed;
    passed = test_availability() && passed;
    passed = test_execution_and_history() && passed;
    passed = test_empty_operation_callback() && passed;
    passed = test_executor_unavailable() && passed;

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== All mesh operation action smoke "
            "tests passed ===\n";
        return 0;
    }

    std::cout
        << "=== Mesh operation action smoke test "
        "failed ===\n";
    return 1;
}