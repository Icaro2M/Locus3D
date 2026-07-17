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

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

    using namespace locus::editor;

    using FaceHandle =
        locus::kernel::geometry::FaceHandle;

    constexpr float NormalEpsilon = 0.00001f;

    void print_result(
        bool condition,
        const std::string& message) {
        std::cout
            << (condition ? "[OK] " : "[FAIL] ")
            << message
            << '\n';
    }

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

    bool approximately_equal(
        const glm::vec3& first,
        const glm::vec3& second,
        float epsilon = NormalEpsilon) {
        return glm::length(first - second) <= epsilon;
    }

    bool approximately_opposite(
        const glm::vec3& first,
        const glm::vec3& second,
        float epsilon = NormalEpsilon) {
        return approximately_equal(
            first,
            -second,
            epsilon);
    }

    ActionId make_action_id(
        std::string_view value) {
        return ActionId{
            std::string{ value }
        };
    }

    ActionId flip_face_action_id() {
        return make_action_id(
            face_actions::FlipFaceId);
    }

    ActionId recalculate_normals_action_id() {
        return make_action_id(
            face_actions::RecalculateNormalsId);
    }

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

        bool build_box() {
            nodeId =
                editor.scene().create_mesh(
                    "Face Actions Test Box");

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

            editor.set_mode(EditorMode::Mesh);

            editor.selection().set_granularity(
                SelectionGranularity::Face);

            editor.selection()
                .mesh()
                .set_active_mesh(nodeId);

            editor.clear_dirty();

            return true;
        }

        void select_faces(
            const std::vector<FaceHandle>& selectedFaces) {
            editor.selection().set_granularity(
                SelectionGranularity::Face);

            editor.selection()
                .mesh()
                .clear_components();

            if (selectedFaces.empty()) {
                editor.selection().mark_dirty();
                editor.clear_dirty();
                return;
            }

            editor.selection()
                .mesh()
                .set_face(selectedFaces.front());

            for (std::size_t index = 1u;
                index < selectedFaces.size();
                ++index) {
                editor.selection()
                    .mesh()
                    .add_face(selectedFaces[index]);
            }

            editor.selection().mark_dirty();
            editor.clear_dirty();
        }
    };

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
            registry.size() == 2u,
            "duas face actions foram registradas");

        print_result(
            registry.contains(
                flip_face_action_id()),
            "registry contem Flip Face");

        print_result(
            registry.contains(
                recalculate_normals_action_id()),
            "registry contem Recalculate Normals");

        const ActionDescriptor* flipDescriptor =
            registry.descriptor(
                flip_face_action_id());

        const ActionDescriptor* normalsDescriptor =
            registry.descriptor(
                recalculate_normals_action_id());

        print_result(
            flipDescriptor != nullptr
            && flipDescriptor->is_valid(),
            "descritor de Flip Face e valido");

        print_result(
            normalsDescriptor != nullptr
            && normalsDescriptor->is_valid(),
            "descritor de Recalculate Normals e valido");

        print_result(
            flipDescriptor
            && flipDescriptor->name == "Flip Face",
            "Flip Face preserva nome");

        print_result(
            normalsDescriptor
            && normalsDescriptor->name
            == "Recalculate Normals",
            "Recalculate Normals preserva nome");

        print_result(
            flipDescriptor
            && flipDescriptor->category
            == ActionCategory::Mesh,
            "Flip Face pertence a Mesh");

        print_result(
            normalsDescriptor
            && normalsDescriptor->category
            == ActionCategory::Mesh,
            "Recalculate Normals pertence a Mesh");

        const bool registeredAgain =
            register_face_actions(registry);

        print_result(
            !registeredAgain,
            "registro duplicado e rejeitado");

        print_result(
            registry.size() == 2u,
            "registro duplicado preserva registry");

        return registered
            && !registeredAgain
            && registry.size() == 2u
            && flipDescriptor
            && normalsDescriptor;
    }

    bool test_transactional_registration() {
        std::cout
            << "\n=== Face actions: transactional registration ===\n";

        ActionRegistry registry{};

        const bool firstRegistration =
            register_face_actions(registry);

        print_result(
            firstRegistration,
            "primeiro registro funcionou");

        const bool removedNormals =
            registry.unregister_action(
                recalculate_normals_action_id());

        print_result(
            removedNormals,
            "Recalculate Normals foi removida");

        print_result(
            registry.size() == 1u,
            "apenas Flip Face permaneceu");

        const bool secondRegistration =
            register_face_actions(registry);

        print_result(
            !secondRegistration,
            "novo registro falha no primeiro ID duplicado");

        print_result(
            registry.size() == 1u,
            "falha inicial nao altera registry");

        print_result(
            !registry.contains(
                recalculate_normals_action_id()),
            "action posterior nao foi registrada");

        return firstRegistration
            && removedNormals
            && !secondRegistration
            && registry.size() == 1u;
    }

    bool test_availability() {
        std::cout
            << "\n=== Face actions: availability ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()) {
            print_result(
                false,
                "fixture foi criada");
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

        fixture.select_faces({
            fixture.faces.front()
            });

        print_result(
            executor.can_execute(
                fixture.context,
                flip_face_action_id()),
            "Flip Face aceita face selecionada");

        print_result(
            executor.can_execute(
                fixture.context,
                recalculate_normals_action_id()),
            "Recalculate Normals aceita face selecionada");

        fixture.editor.selection()
            .mesh()
            .clear_components();

        print_result(
            !executor.can_execute(
                fixture.context,
                flip_face_action_id()),
            "Flip Face rejeita selecao vazia");

        print_result(
            !executor.can_execute(
                fixture.context,
                recalculate_normals_action_id()),
            "Recalculate Normals rejeita selecao vazia");

        fixture.select_faces({
            fixture.faces.front()
            });

        fixture.editor.selection()
            .set_granularity(
                SelectionGranularity::Edge);

        print_result(
            !executor.can_execute(
                fixture.context,
                flip_face_action_id()),
            "Flip Face rejeita granularidade Edge");

        print_result(
            !executor.can_execute(
                fixture.context,
                recalculate_normals_action_id()),
            "Recalculate Normals rejeita granularidade Edge");

        fixture.editor.selection()
            .set_granularity(
                SelectionGranularity::Face);

        fixture.editor.set_mode(EditorMode::Object);

        print_result(
            !executor.can_execute(
                fixture.context,
                flip_face_action_id()),
            "Flip Face rejeita Object mode");

        print_result(
            !executor.can_execute(
                fixture.context,
                recalculate_normals_action_id()),
            "Recalculate Normals rejeita Object mode");

        return true;
    }

    bool test_flip_faces() {
        std::cout
            << "\n=== Flip Face: execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        const FaceHandle firstFace =
            fixture.faces[0];

        const FaceHandle secondFace =
            fixture.faces[1];

        fixture.select_faces({
            firstFace,
            secondFace
            });

        const glm::vec3 firstNormalBefore =
            fixture.node->mesh()
            .face(firstFace)
            .normal;

        const glm::vec3 secondNormalBefore =
            fixture.node->mesh()
            .face(secondFace)
            .normal;

        const std::size_t verticesBefore =
            fixture.node->mesh().vertex_count();

        const std::size_t edgesBefore =
            fixture.node->mesh().edge_count();

        const std::size_t loopsBefore =
            fixture.node->mesh().loop_count();

        const std::size_t facesBefore =
            fixture.node->mesh().face_count();

        ActionRegistry registry{};

        if (!register_face_actions(registry)) {
            print_result(
                false,
                "face actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const ActionResult result =
            executor.execute(
                fixture.context,
                flip_face_action_id());

        print_action_result(
            "Flip Face result",
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
            approximately_opposite(
                firstNormalAfter,
                firstNormalBefore),
            "primeira normal foi invertida");

        print_result(
            approximately_opposite(
                secondNormalAfter,
                secondNormalBefore),
            "segunda normal foi invertida");

        print_result(
            fixture.node->mesh().vertex_count()
            == verticesBefore,
            "Flip Face preservou vertices");

        print_result(
            fixture.node->mesh().edge_count()
            == edgesBefore,
            "Flip Face preservou edges");

        print_result(
            fixture.node->mesh().loop_count()
            == loopsBefore,
            "Flip Face preservou loops");

        print_result(
            fixture.node->mesh().face_count()
            == facesBefore,
            "Flip Face preservou faces");

        print_result(
            fixture.history.undo_size() == 1u,
            "Flip Face criou uma entrada no historico");

        print_result(
            fixture.history.undo_name()
            == "Flip Faces",
            "historico usa label Flip Faces");

        const CommandResult undoResult =
            fixture.history.undo(
                fixture.dispatcher);

        print_result(
            undoResult.success,
            "undo de Flip Face funcionou");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .face(firstFace)
                .normal,
                firstNormalBefore),
            "undo restaurou primeira normal");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .face(secondFace)
                .normal,
                secondNormalBefore),
            "undo restaurou segunda normal");

        print_result(
            fixture.editor.selection()
            .mesh()
            .faces()
            .contains(firstFace)
            && fixture.editor.selection()
            .mesh()
            .faces()
            .contains(secondFace),
            "undo preservou selecao das faces");

        const CommandResult redoResult =
            fixture.history.redo(
                fixture.dispatcher);

        print_result(
            redoResult.success,
            "redo de Flip Face funcionou");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .face(firstFace)
                .normal,
                firstNormalAfter),
            "redo restaurou primeira normal invertida");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .face(secondFace)
                .normal,
                secondNormalAfter),
            "redo restaurou segunda normal invertida");

        return result.succeeded()
            && undoResult.success
            && redoResult.success;
    }

    bool test_recalculate_normals() {
        std::cout
            << "\n=== Recalculate Normals: execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        const FaceHandle firstFace =
            fixture.faces[0];

        const FaceHandle secondFace =
            fixture.faces[1];

        const glm::vec3 expectedFirstNormal =
            fixture.node->mesh()
            .face(firstFace)
            .normal;

        const glm::vec3 expectedSecondNormal =
            fixture.node->mesh()
            .face(secondFace)
            .normal;

        const glm::vec3 corruptedFirstNormal{
            0.25f,
            0.5f,
            0.75f
        };

        const glm::vec3 corruptedSecondNormal{
            -0.4f,
            0.3f,
            0.2f
        };

        fixture.node->mesh()
            .face(firstFace)
            .normal = corruptedFirstNormal;

        fixture.node->mesh()
            .face(secondFace)
            .normal = corruptedSecondNormal;

        fixture.select_faces({
            firstFace,
            secondFace
            });

        const std::size_t verticesBefore =
            fixture.node->mesh().vertex_count();

        const std::size_t edgesBefore =
            fixture.node->mesh().edge_count();

        const std::size_t loopsBefore =
            fixture.node->mesh().loop_count();

        const std::size_t facesBefore =
            fixture.node->mesh().face_count();

        ActionRegistry registry{};

        if (!register_face_actions(registry)) {
            print_result(
                false,
                "face actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const ActionResult result =
            executor.execute(
                fixture.context,
                recalculate_normals_action_id());

        print_action_result(
            "Recalculate Normals result",
            result);

        const glm::vec3 rebuiltFirstNormal =
            fixture.node->mesh()
            .face(firstFace)
            .normal;

        const glm::vec3 rebuiltSecondNormal =
            fixture.node->mesh()
            .face(secondFace)
            .normal;

        print_result(
            result.succeeded(),
            "Recalculate Normals foi executada");

        print_result(
            approximately_equal(
                rebuiltFirstNormal,
                expectedFirstNormal),
            "primeira normal foi reconstruida");

        print_result(
            approximately_equal(
                rebuiltSecondNormal,
                expectedSecondNormal),
            "segunda normal foi reconstruida");

        print_result(
            !approximately_equal(
                rebuiltFirstNormal,
                corruptedFirstNormal),
            "primeira normal corrompida foi substituida");

        print_result(
            !approximately_equal(
                rebuiltSecondNormal,
                corruptedSecondNormal),
            "segunda normal corrompida foi substituida");

        print_result(
            fixture.node->mesh().vertex_count()
            == verticesBefore,
            "recalculo preservou vertices");

        print_result(
            fixture.node->mesh().edge_count()
            == edgesBefore,
            "recalculo preservou edges");

        print_result(
            fixture.node->mesh().loop_count()
            == loopsBefore,
            "recalculo preservou loops");

        print_result(
            fixture.node->mesh().face_count()
            == facesBefore,
            "recalculo preservou faces");

        print_result(
            fixture.history.undo_size() == 1u,
            "recalculo criou uma entrada no historico");

        print_result(
            fixture.history.undo_name()
            == "Recalculate Normals",
            "historico usa label Recalculate Normals");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Mesh),
            "recalculo marca Mesh como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Render),
            "recalculo marca Render como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Picking),
            "recalculo marca Picking como dirty");

        const CommandResult undoResult =
            fixture.history.undo(
                fixture.dispatcher);

        print_result(
            undoResult.success,
            "undo de Recalculate Normals funcionou");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .face(firstFace)
                .normal,
                corruptedFirstNormal),
            "undo restaurou primeira normal corrompida");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .face(secondFace)
                .normal,
                corruptedSecondNormal),
            "undo restaurou segunda normal corrompida");

        print_result(
            fixture.editor.selection()
            .mesh()
            .faces()
            .contains(firstFace)
            && fixture.editor.selection()
            .mesh()
            .faces()
            .contains(secondFace),
            "undo preservou selecao das faces");

        const CommandResult redoResult =
            fixture.history.redo(
                fixture.dispatcher);

        print_result(
            redoResult.success,
            "redo de Recalculate Normals funcionou");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .face(firstFace)
                .normal,
                rebuiltFirstNormal),
            "redo restaurou primeira normal reconstruida");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .face(secondFace)
                .normal,
                rebuiltSecondNormal),
            "redo restaurou segunda normal reconstruida");

        print_result(
            fixture.history.undo_size() == 1u
            && fixture.history.redo_size() == 0u,
            "redo restaurou estado do historico");

        return result.succeeded()
            && undoResult.success
            && redoResult.success
            && approximately_equal(
                fixture.node->mesh()
                .face(firstFace)
                .normal,
                expectedFirstNormal)
            && approximately_equal(
                fixture.node->mesh()
                .face(secondFace)
                .normal,
                expectedSecondNormal);
    }

    bool test_unavailable_execution() {
        std::cout
            << "\n=== Face actions: unavailable execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        fixture.editor.selection()
            .mesh()
            .clear_components();

        ActionRegistry registry{};

        if (!register_face_actions(registry)) {
            print_result(
                false,
                "face actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const glm::vec3 normalBefore =
            fixture.node->mesh()
            .face(fixture.faces.front())
            .normal;

        const ActionResult result =
            executor.execute(
                fixture.context,
                recalculate_normals_action_id());

        print_action_result(
            "Unavailable Recalculate Normals result",
            result);

        print_result(
            result.is_unavailable(),
            "selecao vazia retorna Unavailable");

        print_result(
            fixture.history.empty(),
            "action indisponivel nao entra no historico");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .face(fixture.faces.front())
                .normal,
                normalBefore),
            "action indisponivel nao altera normal");

        return result.is_unavailable()
            && fixture.history.empty();
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor Final Face Actions "
        "Smoke Test ===\n";

    bool passed = true;

    passed = test_registration() && passed;
    passed = test_transactional_registration() && passed;
    passed = test_availability() && passed;
    passed = test_flip_faces() && passed;
    passed = test_recalculate_normals() && passed;
    passed = test_unavailable_execution() && passed;

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== All final face action smoke tests "
            "passed ===\n";
        return 0;
    }

    std::cout
        << "=== Final face action smoke test failed ===\n";
    return 1;
}