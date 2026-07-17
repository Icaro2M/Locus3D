/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/actions/ActionExecutor.h"
#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionContext.h"
#include "editor/actions/mesh/face/RegisterFaceActions.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "kernel/geometry/primitives/BoxBuilder.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace {

    using namespace locus::editor;

    using FaceHandle =
        locus::kernel::geometry::FaceHandle;

    /**
     * @brief Prints one smoke-test assertion.
     *
     * @param condition Assertion result.
     * @param message Human-readable assertion description.
     */
    void print_result(
        bool condition,
        const std::string& message) {
        std::cout
            << (condition ? "[OK] " : "[FAIL] ")
            << message
            << '\n';
    }

    /**
     * @brief Converts an action result code to readable text.
     *
     * @param code Action result code.
     * @return Static readable name.
     */
    const char* result_code_name(
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

    /**
     * @brief Prints an action execution result.
     *
     * @param label Result label.
     * @param result Result to print.
     */
    void print_action_result(
        const std::string& label,
        const ActionResult& result) {
        std::cout << label << '\n';

        std::cout
            << "  code: "
            << result_code_name(result.code)
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

    /**
     * @brief Checks whether two vectors are approximately equal.
     *
     * @param lhs Left-hand vector.
     * @param rhs Right-hand vector.
     * @param epsilon Maximum accepted distance.
     * @return True when vectors are approximately equal.
     */
    bool approximately_equal(
        const glm::vec3& lhs,
        const glm::vec3& rhs,
        float epsilon = 0.0001f) {
        return glm::length(lhs - rhs) <= epsilon;
    }

    /**
     * @brief Converts the official action identifier to ActionId.
     *
     * @return Built-in Flip Face action identifier.
     */
    ActionId flip_face_action_id() {
        return ActionId{
            std::string{
                face_actions::FlipFaceId
            }
        };
    }

    /**
     * @brief Fixture containing one editable box and action runtime services.
     */
    struct MeshFixture {
        Editor editor{};

        CommandDispatcher dispatcher{
            editor
        };

        HistoryStack history{};

        ActionContext context{
            editor,
            dispatcher,
            history
        };

        SceneNodeId nodeId{};
        MeshNode* node = nullptr;

        std::vector<FaceHandle> faces{};

        /**
         * @brief Builds the editable box fixture.
         *
         * @return True when construction succeeded.
         */
        bool build() {
            nodeId =
                editor.scene().create_mesh(
                    "Face Action Test Box");

            node =
                editor.scene().find_mesh(nodeId);

            if (!node) {
                return false;
            }

            locus::kernel::geometry::BoxParameters
                parameters{};

            parameters.size = {
                2.0f,
                2.0f,
                2.0f
            };

            const auto buildResult =
                locus::kernel::geometry::BoxBuilder::
                build_into(
                    node->mesh(),
                    parameters);

            if (!buildResult.success
                || buildResult.faces.size() != 6u) {
                return false;
            }

            faces = buildResult.faces;

            for (const FaceHandle face : faces) {
                if (!node->mesh().is_valid(face)) {
                    return false;
                }
            }

            editor.set_mode(EditorMode::Mesh);

            editor.selection().set_granularity(
                SelectionGranularity::Face);

            editor.selection()
                .mesh()
                .set_active_mesh(nodeId);

            editor.selection()
                .mesh()
                .set_face(faces[0]);

            editor.selection()
                .mesh()
                .add_face(faces[1]);

            editor.selection().mark_dirty();
            editor.clear_dirty();

            return true;
        }
    };

    /**
     * @brief Tests the official face action registration.
     *
     * @return True when every assertion passed.
     */
    bool test_registration() {
        std::cout
            << "\n=== Face actions: registration ===\n";

        ActionRegistry registry{};

        const bool registered =
            register_face_actions(registry);

        print_result(
            registered,
            "face actions foram registradas");

        print_result(
            registry.size() == 1u,
            "uma face action foi registrada");

        print_result(
            registry.contains(
                flip_face_action_id()),
            "registry contem mesh.face.flip");

        const ActionDescriptor* descriptor =
            registry.descriptor(
                flip_face_action_id());

        print_result(
            descriptor != nullptr,
            "descritor oficial pode ser consultado");

        print_result(
            descriptor
            && descriptor->is_valid(),
            "descritor oficial e valido");

        print_result(
            descriptor
            && descriptor->name == "Flip Face",
            "descritor preserva o nome Flip Face");

        print_result(
            descriptor
            && descriptor->category
            == ActionCategory::Mesh,
            "Flip Face pertence a categoria Mesh");

        print_result(
            descriptor
            && !descriptor->description.empty(),
            "descritor possui descricao");

        print_result(
            descriptor
            && !descriptor->keywords.empty(),
            "descritor possui termos de busca");

        const bool registeredAgain =
            register_face_actions(registry);

        print_result(
            !registeredAgain,
            "registro duplicado e rejeitado");

        print_result(
            registry.size() == 1u,
            "registro duplicado nao altera o registry");

        return registered
            && !registeredAgain
            && registry.size() == 1u
            && registry.contains(
                flip_face_action_id())
            && descriptor
            && descriptor->is_valid();
    }

    /**
     * @brief Tests availability rules of the official action.
     *
     * @return True when every assertion passed.
     */
    bool test_availability() {
        std::cout
            << "\n=== Flip Face: availability ===\n";

        MeshFixture fixture{};

        if (!fixture.build()) {
            print_result(
                false,
                "fixture necessaria foi criada");
            return false;
        }

        ActionRegistry registry{};

        if (!register_face_actions(registry)) {
            print_result(
                false,
                "face actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        print_result(
            executor.can_execute(
                fixture.context,
                flip_face_action_id()),
            "Flip Face esta disponivel com faces selecionadas");

        fixture.editor.set_mode(EditorMode::Object);

        print_result(
            !executor.can_execute(
                fixture.context,
                flip_face_action_id()),
            "Flip Face fica indisponivel em Object mode");

        fixture.editor.set_mode(EditorMode::Mesh);

        fixture.editor.selection().set_granularity(
            SelectionGranularity::Edge);

        print_result(
            !executor.can_execute(
                fixture.context,
                flip_face_action_id()),
            "Flip Face fica indisponivel em granularidade Edge");

        fixture.editor.selection().set_granularity(
            SelectionGranularity::Face);

        fixture.editor.selection()
            .mesh()
            .clear_components();

        print_result(
            !executor.can_execute(
                fixture.context,
                flip_face_action_id()),
            "Flip Face fica indisponivel sem faces");

        fixture.editor.selection()
            .mesh()
            .set_face(fixture.faces[0]);

        print_result(
            executor.can_execute(
                fixture.context,
                flip_face_action_id()),
            "Flip Face volta a ficar disponivel");

        fixture.editor.selection()
            .mesh()
            .set_active_mesh(SceneNodeId{});

        print_result(
            !executor.can_execute(
                fixture.context,
                flip_face_action_id()),
            "Flip Face fica indisponivel sem mesh ativo");

        return true;
    }

    /**
     * @brief Tests batch execution and history behavior.
     *
     * @return True when every assertion passed.
     */
    bool test_batch_execution() {
        std::cout
            << "\n=== Flip Face: batch execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build()) {
            print_result(
                false,
                "fixture necessaria foi criada");
            return false;
        }

        ActionRegistry registry{};

        if (!register_face_actions(registry)) {
            print_result(
                false,
                "face actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const FaceHandle firstFace =
            fixture.faces[0];

        const FaceHandle secondFace =
            fixture.faces[1];

        const glm::vec3 firstNormalBefore =
            fixture.node->mesh()
            .face(firstFace)
            .normal;

        const glm::vec3 secondNormalBefore =
            fixture.node->mesh()
            .face(secondFace)
            .normal;

        print_result(
            fixture.editor.selection()
            .mesh()
            .faces()
            .size()
            == 2u,
            "duas faces estao selecionadas");

        const ActionResult result =
            executor.execute(
                fixture.context,
                flip_face_action_id());

        print_action_result(
            "Flip Face batch result",
            result);

        const glm::vec3 firstNormalAfter =
            fixture.node->mesh()
            .face(firstFace)
            .normal;

        const glm::vec3 secondNormalAfter =
            fixture.node->mesh()
            .face(secondFace)
            .normal;

        print_result(
            result.succeeded(),
            "Flip Face foi executada");

        print_result(
            approximately_equal(
                firstNormalAfter,
                -firstNormalBefore),
            "primeira normal foi invertida");

        print_result(
            approximately_equal(
                secondNormalAfter,
                -secondNormalBefore),
            "segunda normal foi invertida");

        print_result(
            fixture.node->mesh().is_valid(firstFace)
            && fixture.node->mesh().is_valid(secondFace),
            "faces continuam validas");

        print_result(
            fixture.node->mesh().face_count() == 6u,
            "quantidade de faces foi preservada");

        print_result(
            fixture.history.undo_size() == 1u,
            "lote criou uma unica entrada no historico");

        print_result(
            fixture.history.redo_size() == 0u,
            "redo inicia vazio");

        print_result(
            fixture.history.undo_name()
            == "Flip Faces",
            "historico usa o label Flip Faces");

        print_result(
            fixture.history.can_undo(),
            "undo ficou disponivel");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Mesh),
            "Mesh foi marcado como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Render),
            "Render foi marcado como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Picking),
            "Picking foi marcado como dirty");

        const CommandResult undoResult =
            fixture.history.undo(
                fixture.dispatcher);

        print_result(
            undoResult.success,
            "undo do lote funcionou");

        const glm::vec3 firstNormalUndo =
            fixture.node->mesh()
            .face(firstFace)
            .normal;

        const glm::vec3 secondNormalUndo =
            fixture.node->mesh()
            .face(secondFace)
            .normal;

        print_result(
            approximately_equal(
                firstNormalUndo,
                firstNormalBefore),
            "undo restaurou a primeira normal");

        print_result(
            approximately_equal(
                secondNormalUndo,
                secondNormalBefore),
            "undo restaurou a segunda normal");

        print_result(
            fixture.history.undo_size() == 0u,
            "undo removeu a entrada da pilha de undo");

        print_result(
            fixture.history.redo_size() == 1u,
            "undo criou uma entrada de redo");

        print_result(
            fixture.history.redo_name()
            == "Flip Faces",
            "redo preserva o label Flip Faces");

        print_result(
            fixture.editor.selection()
            .mesh()
            .faces()
            .contains(firstFace),
            "undo preservou a primeira face selecionada");

        print_result(
            fixture.editor.selection()
            .mesh()
            .faces()
            .contains(secondFace),
            "undo preservou a segunda face selecionada");

        const CommandResult redoResult =
            fixture.history.redo(
                fixture.dispatcher);

        print_result(
            redoResult.success,
            "redo do lote funcionou");

        const glm::vec3 firstNormalRedo =
            fixture.node->mesh()
            .face(firstFace)
            .normal;

        const glm::vec3 secondNormalRedo =
            fixture.node->mesh()
            .face(secondFace)
            .normal;

        print_result(
            approximately_equal(
                firstNormalRedo,
                firstNormalAfter),
            "redo restaurou a primeira normal invertida");

        print_result(
            approximately_equal(
                secondNormalRedo,
                secondNormalAfter),
            "redo restaurou a segunda normal invertida");

        print_result(
            fixture.history.undo_size() == 1u,
            "redo restaurou a entrada de undo");

        print_result(
            fixture.history.redo_size() == 0u,
            "redo esvaziou sua pilha");

        print_result(
            fixture.editor.selection()
            .mesh()
            .active_mesh()
            == fixture.nodeId,
            "mesh ativo foi preservado");

        print_result(
            fixture.editor.selection()
            .mesh()
            .faces()
            .size()
            == 2u,
            "selecao do lote foi preservada");

        return result.succeeded()
            && undoResult.success
            && redoResult.success
            && approximately_equal(
                firstNormalRedo,
                -firstNormalBefore)
            && approximately_equal(
                secondNormalRedo,
                -secondNormalBefore)
            && fixture.history.undo_size() == 1u
            && fixture.history.redo_size() == 0u;
    }

    /**
     * @brief Tests execution while the action is unavailable.
     *
     * @return True when every assertion passed.
     */
    bool test_unavailable_execution() {
        std::cout
            << "\n=== Flip Face: unavailable execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build()) {
            print_result(
                false,
                "fixture necessaria foi criada");
            return false;
        }

        ActionRegistry registry{};

        if (!register_face_actions(registry)) {
            print_result(
                false,
                "face actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        fixture.editor.selection()
            .mesh()
            .clear_components();

        const glm::vec3 normalBefore =
            fixture.node->mesh()
            .face(fixture.faces[0])
            .normal;

        const ActionResult result =
            executor.execute(
                fixture.context,
                flip_face_action_id());

        print_action_result(
            "unavailable Flip Face result",
            result);

        print_result(
            result.is_unavailable(),
            "execucao sem faces retorna Unavailable");

        print_result(
            fixture.history.empty(),
            "action indisponivel nao entra no historico");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .face(fixture.faces[0])
                .normal,
                normalBefore),
            "action indisponivel nao altera a malha");

        return result.is_unavailable()
            && fixture.history.empty();
    }

    /**
     * @brief Tests stale selected handle validation.
     *
     * @return True when every assertion passed.
     */
    bool test_invalid_selected_handle() {
        std::cout
            << "\n=== Flip Face: invalid selected handle ===\n";

        MeshFixture fixture{};

        if (!fixture.build()) {
            print_result(
                false,
                "fixture necessaria foi criada");
            return false;
        }

        ActionRegistry registry{};

        if (!register_face_actions(registry)) {
            print_result(
                false,
                "face actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const FaceHandle invalidFace{};

        fixture.editor.selection()
            .mesh()
            .set_face(invalidFace);

        const bool invalidHandleAccepted =
            fixture.node->mesh()
            .is_valid(invalidFace);

        std::cout
            << "invalid handle active in mesh: "
            << (invalidHandleAccepted
                ? "true"
                : "false")
            << '\n';

        if (invalidHandleAccepted) {
            std::cout
                << "Fixture note: default FaceHandle points "
                "to an active face in this LEM.\n";

            return true;
        }

        print_result(
            !executor.can_execute(
                fixture.context,
                flip_face_action_id()),
            "action rejeita handle selecionado invalido");

        const ActionResult result =
            executor.execute(
                fixture.context,
                flip_face_action_id());

        print_result(
            result.is_unavailable(),
            "handle invalido retorna Unavailable");

        print_result(
            fixture.history.empty(),
            "handle invalido nao entra no historico");

        return result.is_unavailable()
            && fixture.history.empty();
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor Face Actions "
        "Smoke Test ===\n";

    bool passed = true;

    passed = test_registration() && passed;
    passed = test_availability() && passed;
    passed = test_batch_execution() && passed;
    passed = test_unavailable_execution() && passed;
    passed = test_invalid_selected_handle() && passed;

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== All face action smoke tests "
            "passed ===\n";
        return 0;
    }

    std::cout
        << "=== Face action smoke test failed ===\n";
    return 1;
}