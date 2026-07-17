/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/actions/ActionExecutor.h"
#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionContext.h"
#include "editor/actions/mesh/edge/RegisterEdgeActions.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "kernel/geometry/primitives/BoxBuilder.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

    using namespace locus::editor;

    using EdgeHandle =
        locus::kernel::geometry::EdgeHandle;

    constexpr float CreaseEpsilon = 0.00001f;

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
        float first,
        float second,
        float epsilon = CreaseEpsilon) {
        return std::abs(first - second) <= epsilon;
    }

    ActionId make_action_id(
        std::string_view value) {
        return ActionId{
            std::string{ value }
        };
    }

    ActionId mark_sharp_action_id() {
        return make_action_id(
            edge_actions::MarkSharpId);
    }

    ActionId clear_sharp_action_id() {
        return make_action_id(
            edge_actions::ClearSharpId);
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

        std::vector<EdgeHandle> edges{};

        bool build_box() {
            nodeId =
                editor.scene().create_mesh(
                    "Edge Actions Test Box");

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
                || buildResult.edges.size() != 12u
                || buildResult.faces.size() != 6u
                || buildResult.vertices.size() != 8u) {
                return false;
            }

            edges = buildResult.edges;

            editor.set_mode(EditorMode::Mesh);

            editor.selection().set_granularity(
                SelectionGranularity::Edge);

            editor.selection()
                .mesh()
                .set_active_mesh(nodeId);

            editor.clear_dirty();

            return true;
        }

        void select_edges(
            const std::vector<EdgeHandle>& selectedEdges) {
            editor.selection().set_granularity(
                SelectionGranularity::Edge);

            editor.selection()
                .mesh()
                .clear_components();

            if (selectedEdges.empty()) {
                editor.selection().mark_dirty();
                editor.clear_dirty();
                return;
            }

            editor.selection()
                .mesh()
                .set_edge(selectedEdges.front());

            for (std::size_t index = 1u;
                index < selectedEdges.size();
                ++index) {
                editor.selection()
                    .mesh()
                    .add_edge(selectedEdges[index]);
            }

            editor.selection().mark_dirty();
            editor.clear_dirty();
        }

        bool selected_edges_have_crease(
            float expectedCrease) const {
            const auto& selectedEdges =
                editor.selection()
                .mesh()
                .edges()
                .items();

            if (selectedEdges.empty()) {
                return false;
            }

            for (const EdgeHandle edge : selectedEdges) {
                if (!node->mesh().is_valid(edge)) {
                    return false;
                }

                if (!approximately_equal(
                    node->mesh().edge(edge).crease,
                    expectedCrease)) {
                    return false;
                }
            }

            return true;
        }
    };

    bool test_registration() {
        std::cout
            << "\n=== Edge actions: registration ===\n";

        ActionRegistry registry{};

        const bool registered =
            register_edge_actions(registry);

        print_result(
            registered,
            "edge actions foram registradas");

        print_result(
            registry.size() == 2u,
            "duas edge actions foram registradas");

        print_result(
            registry.contains(
                mark_sharp_action_id()),
            "registry contem Mark Sharp");

        print_result(
            registry.contains(
                clear_sharp_action_id()),
            "registry contem Clear Sharp");

        const ActionDescriptor* markDescriptor =
            registry.descriptor(
                mark_sharp_action_id());

        const ActionDescriptor* clearDescriptor =
            registry.descriptor(
                clear_sharp_action_id());

        print_result(
            markDescriptor
            && markDescriptor->is_valid(),
            "descritor de Mark Sharp e valido");

        print_result(
            clearDescriptor
            && clearDescriptor->is_valid(),
            "descritor de Clear Sharp e valido");

        print_result(
            markDescriptor
            && markDescriptor->name
            == "Mark Sharp",
            "Mark Sharp preserva nome");

        print_result(
            clearDescriptor
            && clearDescriptor->name
            == "Clear Sharp",
            "Clear Sharp preserva nome");

        print_result(
            markDescriptor
            && markDescriptor->category
            == ActionCategory::Mesh,
            "Mark Sharp pertence a Mesh");

        print_result(
            clearDescriptor
            && clearDescriptor->category
            == ActionCategory::Mesh,
            "Clear Sharp pertence a Mesh");

        const bool registeredAgain =
            register_edge_actions(registry);

        print_result(
            !registeredAgain,
            "registro duplicado e rejeitado");

        print_result(
            registry.size() == 2u,
            "registro duplicado preserva registry");

        return registered
            && !registeredAgain
            && registry.size() == 2u
            && markDescriptor
            && clearDescriptor;
    }

    bool test_transactional_registration() {
        std::cout
            << "\n=== Edge actions: transactional registration ===\n";

        ActionRegistry registry{};

        const bool firstRegistration =
            register_edge_actions(registry);

        print_result(
            firstRegistration,
            "primeiro registro funcionou");

        const bool removedClear =
            registry.unregister_action(
                clear_sharp_action_id());

        print_result(
            removedClear,
            "Clear Sharp foi removida");

        print_result(
            registry.size() == 1u,
            "apenas Mark Sharp permaneceu");

        const bool secondRegistration =
            register_edge_actions(registry);

        print_result(
            !secondRegistration,
            "novo registro falha no primeiro ID duplicado");

        print_result(
            registry.size() == 1u,
            "falha inicial preserva registry");

        print_result(
            !registry.contains(
                clear_sharp_action_id()),
            "action posterior nao foi registrada");

        return firstRegistration
            && removedClear
            && !secondRegistration
            && registry.size() == 1u;
    }

    bool test_availability() {
        std::cout
            << "\n=== Edge actions: availability ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        ActionRegistry registry{};

        if (!register_edge_actions(registry)) {
            print_result(
                false,
                "edge actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        fixture.select_edges({
            fixture.edges[3],
            fixture.edges[0]
            });

        print_result(
            executor.can_execute(
                fixture.context,
                mark_sharp_action_id()),
            "Mark Sharp aceita edges selecionadas");

        print_result(
            executor.can_execute(
                fixture.context,
                clear_sharp_action_id()),
            "Clear Sharp aceita edges selecionadas");

        fixture.select_edges({});

        print_result(
            !executor.can_execute(
                fixture.context,
                mark_sharp_action_id()),
            "Mark Sharp rejeita selecao vazia");

        print_result(
            !executor.can_execute(
                fixture.context,
                clear_sharp_action_id()),
            "Clear Sharp rejeita selecao vazia");

        fixture.select_edges({
            fixture.edges[0]
            });

        fixture.editor.selection().set_granularity(
            SelectionGranularity::Face);

        print_result(
            !executor.can_execute(
                fixture.context,
                mark_sharp_action_id()),
            "Mark Sharp rejeita granularidade Face");

        print_result(
            !executor.can_execute(
                fixture.context,
                clear_sharp_action_id()),
            "Clear Sharp rejeita granularidade Face");

        fixture.editor.selection().set_granularity(
            SelectionGranularity::Edge);

        fixture.editor.set_mode(EditorMode::Object);

        print_result(
            !executor.can_execute(
                fixture.context,
                mark_sharp_action_id()),
            "Mark Sharp rejeita Object mode");

        fixture.editor.set_mode(EditorMode::Mesh);

        print_result(
            executor.can_execute(
                fixture.context,
                mark_sharp_action_id()),
            "Mark Sharp volta a ficar disponivel");

        return true;
    }

    bool test_mark_sharp() {
        std::cout
            << "\n=== Mark Sharp: execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        const std::vector<EdgeHandle> selectedEdges{
            fixture.edges[5],
            fixture.edges[1],
            fixture.edges[9]
        };

        fixture.select_edges(selectedEdges);

        for (const EdgeHandle edge : selectedEdges) {
            fixture.node->mesh()
                .edge(edge)
                .crease = 0.25f;
        }

        ActionRegistry registry{};

        if (!register_edge_actions(registry)) {
            print_result(
                false,
                "edge actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const std::size_t verticesBefore =
            fixture.node->mesh().vertex_count();

        const std::size_t edgesBefore =
            fixture.node->mesh().edge_count();

        const std::size_t loopsBefore =
            fixture.node->mesh().loop_count();

        const std::size_t facesBefore =
            fixture.node->mesh().face_count();

        const ActionResult result =
            executor.execute(
                fixture.context,
                mark_sharp_action_id());

        print_action_result(
            "Mark Sharp result",
            result);

        print_result(
            result.succeeded(),
            "Mark Sharp foi executada");

        print_result(
            fixture.selected_edges_have_crease(
                1.0f),
            "edges selecionadas receberam crease 1");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .edge(fixture.edges[0])
                .crease,
                0.0f),
            "edge nao selecionada foi preservada");

        print_result(
            fixture.node->mesh().vertex_count()
            == verticesBefore,
            "Mark Sharp preservou vertices");

        print_result(
            fixture.node->mesh().edge_count()
            == edgesBefore,
            "Mark Sharp preservou edges");

        print_result(
            fixture.node->mesh().loop_count()
            == loopsBefore,
            "Mark Sharp preservou loops");

        print_result(
            fixture.node->mesh().face_count()
            == facesBefore,
            "Mark Sharp preservou faces");

        print_result(
            fixture.history.undo_size() == 1u,
            "Mark Sharp criou uma entrada no historico");

        print_result(
            fixture.history.undo_name()
            == "Mark Edges Sharp",
            "historico usa label Mark Edges Sharp");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Mesh),
            "Mark Sharp marca Mesh como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Render),
            "Mark Sharp marca Render como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Picking),
            "Mark Sharp marca Picking como dirty");

        const CommandResult undoResult =
            fixture.history.undo(
                fixture.dispatcher);

        print_result(
            undoResult.success,
            "undo de Mark Sharp funcionou");

        bool undoRestoredCrease = true;

        for (const EdgeHandle edge : selectedEdges) {
            if (!approximately_equal(
                fixture.node->mesh()
                .edge(edge)
                .crease,
                0.25f)) {
                undoRestoredCrease = false;
                break;
            }
        }

        print_result(
            undoRestoredCrease,
            "undo restaurou crease anterior");

        print_result(
            fixture.editor.selection()
            .mesh()
            .edges()
            .items()
            == selectedEdges,
            "undo restaurou ordem da selecao");

        print_result(
            fixture.history.can_redo(),
            "redo ficou disponivel");

        print_result(
            fixture.history.redo_name()
            == "Mark Edges Sharp",
            "redo preserva label");

        const CommandResult redoResult =
            fixture.history.redo(
                fixture.dispatcher);

        print_result(
            redoResult.success,
            "redo de Mark Sharp funcionou");

        print_result(
            fixture.selected_edges_have_crease(
                1.0f),
            "redo restaurou crease 1");

        print_result(
            fixture.history.undo_size() == 1u
            && fixture.history.redo_size() == 0u,
            "redo restaurou estado do historico");

        return result.succeeded()
            && undoResult.success
            && redoResult.success
            && fixture.selected_edges_have_crease(
                1.0f);
    }

    bool test_clear_sharp() {
        std::cout
            << "\n=== Clear Sharp: execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        const std::vector<EdgeHandle> selectedEdges{
            fixture.edges[8],
            fixture.edges[2],
            fixture.edges[6]
        };

        fixture.select_edges(selectedEdges);

        for (const EdgeHandle edge : selectedEdges) {
            fixture.node->mesh()
                .edge(edge)
                .crease = 0.8f;
        }

        ActionRegistry registry{};

        if (!register_edge_actions(registry)) {
            print_result(
                false,
                "edge actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const ActionResult result =
            executor.execute(
                fixture.context,
                clear_sharp_action_id());

        print_action_result(
            "Clear Sharp result",
            result);

        print_result(
            result.succeeded(),
            "Clear Sharp foi executada");

        print_result(
            fixture.selected_edges_have_crease(
                0.0f),
            "edges selecionadas receberam crease 0");

        print_result(
            fixture.history.undo_size() == 1u,
            "Clear Sharp criou uma entrada no historico");

        print_result(
            fixture.history.undo_name()
            == "Clear Edge Sharpness",
            "historico usa label Clear Edge Sharpness");

        const CommandResult undoResult =
            fixture.history.undo(
                fixture.dispatcher);

        print_result(
            undoResult.success,
            "undo de Clear Sharp funcionou");

        bool undoRestoredCrease = true;

        for (const EdgeHandle edge : selectedEdges) {
            if (!approximately_equal(
                fixture.node->mesh()
                .edge(edge)
                .crease,
                0.8f)) {
                undoRestoredCrease = false;
                break;
            }
        }

        print_result(
            undoRestoredCrease,
            "undo restaurou crease 0.8");

        const CommandResult redoResult =
            fixture.history.redo(
                fixture.dispatcher);

        print_result(
            redoResult.success,
            "redo de Clear Sharp funcionou");

        print_result(
            fixture.selected_edges_have_crease(
                0.0f),
            "redo restaurou crease 0");

        return result.succeeded()
            && undoResult.success
            && redoResult.success
            && fixture.selected_edges_have_crease(
                0.0f);
    }

    bool test_unavailable_execution() {
        std::cout
            << "\n=== Edge actions: unavailable execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        fixture.select_edges({});

        ActionRegistry registry{};

        if (!register_edge_actions(registry)) {
            print_result(
                false,
                "edge actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const ActionResult result =
            executor.execute(
                fixture.context,
                mark_sharp_action_id());

        print_action_result(
            "Unavailable Mark Sharp result",
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
                .edge(fixture.edges.front())
                .crease,
                0.0f),
            "action indisponivel nao altera a malha");

        return result.is_unavailable()
            && fixture.history.empty();
    }

    bool test_already_marked_behavior() {
        std::cout
            << "\n=== Mark Sharp: already marked ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        const std::vector<EdgeHandle> selectedEdges{
            fixture.edges[0],
            fixture.edges[1]
        };

        fixture.select_edges(selectedEdges);

        for (const EdgeHandle edge : selectedEdges) {
            fixture.node->mesh()
                .edge(edge)
                .crease = 1.0f;
        }

        ActionRegistry registry{};

        if (!register_edge_actions(registry)) {
            print_result(
                false,
                "edge actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const ActionResult result =
            executor.execute(
                fixture.context,
                mark_sharp_action_id());

        print_action_result(
            "Already marked result",
            result);

        print_result(
            result.failed(),
            "operacao sem mudanca retorna Failed");

        print_result(
            fixture.history.empty(),
            "operacao sem mudanca nao entra no historico");

        print_result(
            fixture.selected_edges_have_crease(
                1.0f),
            "crease existente foi preservado");

        return result.failed()
            && fixture.history.empty()
            && fixture.selected_edges_have_crease(
                1.0f);
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor Final Edge Actions "
        "Smoke Test ===\n";

    bool passed = true;

    passed = test_registration() && passed;
    passed = test_transactional_registration() && passed;
    passed = test_availability() && passed;
    passed = test_mark_sharp() && passed;
    passed = test_clear_sharp() && passed;
    passed = test_unavailable_execution() && passed;
    passed = test_already_marked_behavior() && passed;

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== All final edge action smoke tests "
            "passed ===\n";
        return 0;
    }

    std::cout
        << "=== Final edge action smoke test failed ===\n";
    return 1;
}