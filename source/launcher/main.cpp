/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/actions/ActionExecutor.h"
#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionContext.h"
#include "editor/actions/mesh/topology/RegisterTopologyActions.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/primitives/BoxBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

    using namespace locus::editor;

    using EdgeHandle =
        locus::kernel::geometry::EdgeHandle;

    using FaceHandle =
        locus::kernel::geometry::FaceHandle;

    using LoopHandle =
        locus::kernel::geometry::LoopHandle;

    using TopologyTraversal =
        locus::kernel::geometry::TopologyTraversal;

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

    ActionId make_action_id(
        std::string_view value) {
        return ActionId{
            std::string{ value }
        };
    }

    ActionId fill_hole_action_id() {
        return make_action_id(
            topology_actions::FillHoleId);
    }

    std::size_t active_face_count(
        const locus::kernel::geometry::LEM& mesh) {
        return TopologyTraversal::faces(mesh).size();
    }

    std::size_t boundary_edge_count(
        const locus::kernel::geometry::LEM& mesh) {
        std::size_t count = 0u;

        for (const EdgeHandle edge
            : TopologyTraversal::edges(mesh)) {
            if (TopologyTraversal::is_boundary_edge(
                mesh,
                edge)) {
                ++count;
            }
        }

        return count;
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
        std::vector<FaceHandle> faces{};

        bool build_box() {
            nodeId =
                editor.scene().create_mesh(
                    "Fill Hole Test Box");

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
                || buildResult.faces.size() != 6u) {
                return false;
            }

            edges = buildResult.edges;
            faces = buildResult.faces;

            editor.set_mode(EditorMode::Mesh);

            editor.selection().set_granularity(
                SelectionGranularity::Edge);

            editor.selection()
                .mesh()
                .set_active_mesh(nodeId);

            editor.clear_dirty();

            return true;
        }

        bool remove_face_and_select_boundary(
            FaceHandle face) {
            if (!node
                || !node->mesh().is_valid(face)) {
                return false;
            }

            const std::vector<EdgeHandle> boundaryEdges =
                TopologyTraversal::face_edges(
                    node->mesh(),
                    face);

            if (boundaryEdges.size() < 3u) {
                return false;
            }

            locus::kernel::geometry::LEMEditor meshEditor{
                node->mesh()
            };

            if (!meshEditor.remove_face(face)) {
                return false;
            }

            for (const EdgeHandle edge : boundaryEdges) {
                if (!node->mesh().is_valid(edge)
                    || !TopologyTraversal::is_boundary_edge(
                        node->mesh(),
                        edge)) {
                    return false;
                }
            }

            editor.selection()
                .mesh()
                .set_edge(boundaryEdges.front());

            for (std::size_t index = 1u;
                index < boundaryEdges.size();
                ++index) {
                editor.selection()
                    .mesh()
                    .add_edge(boundaryEdges[index]);
            }

            editor.selection().mark_dirty();
            editor.clear_dirty();

            edges = boundaryEdges;
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
    };

    bool test_registration() {
        std::cout
            << "\n=== Fill Hole: registration ===\n";

        ActionRegistry registry{};

        const bool registered =
            register_topology_actions(registry);

        print_result(
            registered,
            "topology actions foram registradas");

        print_result(
            registry.size() == 3u,
            "tres topology actions foram registradas");

        print_result(
            registry.contains(
                fill_hole_action_id()),
            "registry contem mesh.topology.fill_hole");

        const ActionDescriptor* descriptor =
            registry.descriptor(
                fill_hole_action_id());

        print_result(
            descriptor != nullptr,
            "descritor de Fill Hole pode ser consultado");

        print_result(
            descriptor
            && descriptor->is_valid(),
            "descritor de Fill Hole e valido");

        print_result(
            descriptor
            && descriptor->name == "Fill Hole",
            "descritor preserva o nome Fill Hole");

        print_result(
            descriptor
            && descriptor->category
            == ActionCategory::Mesh,
            "Fill Hole pertence a categoria Mesh");

        print_result(
            descriptor
            && !descriptor->keywords.empty(),
            "Fill Hole possui termos de busca");

        const bool registeredAgain =
            register_topology_actions(registry);

        print_result(
            !registeredAgain,
            "registro duplicado e rejeitado");

        print_result(
            registry.size() == 3u,
            "registro duplicado preserva o registry");

        return registered
            && !registeredAgain
            && registry.size() == 3u
            && descriptor
            && descriptor->is_valid();
    }

    bool test_open_box_fixture() {
        std::cout
            << "\n=== Fill Hole: open box fixture ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()) {
            print_result(
                false,
                "caixa base foi criada");
            return false;
        }

        const FaceHandle removedFace =
            fixture.faces.front();

        const std::vector<EdgeHandle> faceEdges =
            TopologyTraversal::face_edges(
                fixture.node->mesh(),
                removedFace);

        print_result(
            faceEdges.size() == 4u,
            "face da caixa possui quatro edges");

        const bool removed =
            fixture.remove_face_and_select_boundary(
                removedFace);

        print_result(
            removed,
            "face foi removida");

        if (!removed) {
            return false;
        }

        print_result(
            !fixture.node->mesh().is_valid(
                removedFace),
            "face removida deixou de estar ativa");

        print_result(
            active_face_count(
                fixture.node->mesh()) == 5u,
            "caixa aberta possui cinco faces ativas");

        print_result(
            fixture.edges.size() == 4u,
            "quatro edges de contorno foram capturadas");

        print_result(
            boundary_edge_count(
                fixture.node->mesh()) == 4u,
            "caixa aberta possui quatro boundary edges");

        print_result(
            fixture.editor.selection()
            .mesh()
            .edges()
            .size()
            == 4u,
            "as quatro boundary edges estao selecionadas");

        bool allBoundary = true;

        for (const EdgeHandle edge : fixture.edges) {
            if (!TopologyTraversal::is_boundary_edge(
                fixture.node->mesh(),
                edge)) {
                allBoundary = false;
                break;
            }
        }

        print_result(
            allBoundary,
            "todas as edges selecionadas sao de fronteira");

        return removed
            && allBoundary
            && active_face_count(
                fixture.node->mesh()) == 5u
            && fixture.editor.selection()
            .mesh()
            .edges()
            .size()
            == 4u;
    }

    bool test_availability() {
        std::cout
            << "\n=== Fill Hole: availability ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()
            || !fixture.remove_face_and_select_boundary(
                fixture.faces.front())) {
            print_result(
                false,
                "fixture aberta foi criada");
            return false;
        }

        ActionRegistry registry{};

        if (!register_topology_actions(registry)) {
            print_result(
                false,
                "topology actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        print_result(
            executor.can_execute(
                fixture.context,
                fill_hole_action_id()),
            "Fill Hole aceita quatro boundary edges");

        fixture.editor.selection()
            .mesh()
            .set_edge(fixture.edges.front());

        print_result(
            !executor.can_execute(
                fixture.context,
                fill_hole_action_id()),
            "Fill Hole rejeita menos de tres edges");

        fixture.select_edges(fixture.edges);

        fixture.editor.selection()
            .set_granularity(
                SelectionGranularity::Face);

        print_result(
            !executor.can_execute(
                fixture.context,
                fill_hole_action_id()),
            "Fill Hole rejeita granularidade Face");

        fixture.editor.selection()
            .set_granularity(
                SelectionGranularity::Edge);

        fixture.editor.set_mode(EditorMode::Object);

        print_result(
            !executor.can_execute(
                fixture.context,
                fill_hole_action_id()),
            "Fill Hole rejeita Object mode");

        fixture.editor.set_mode(EditorMode::Mesh);

        print_result(
            executor.can_execute(
                fixture.context,
                fill_hole_action_id()),
            "Fill Hole volta a ficar disponivel");

        return true;
    }

    bool test_fill_hole_execution() {
        std::cout
            << "\n=== Fill Hole: execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()
            || !fixture.remove_face_and_select_boundary(
                fixture.faces.front())) {
            print_result(
                false,
                "fixture aberta foi criada");
            return false;
        }

        ActionRegistry registry{};

        if (!register_topology_actions(registry)) {
            print_result(
                false,
                "topology actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const std::size_t faceSlotsBefore =
            fixture.node->mesh().face_count();

        const std::size_t activeFacesBefore =
            active_face_count(
                fixture.node->mesh());

        const std::size_t boundaryEdgesBefore =
            boundary_edge_count(
                fixture.node->mesh());

        const ActionResult result =
            executor.execute(
                fixture.context,
                fill_hole_action_id());

        print_action_result(
            "Fill Hole result",
            result);

        const std::size_t faceSlotsAfter =
            fixture.node->mesh().face_count();

        const std::size_t activeFacesAfter =
            active_face_count(
                fixture.node->mesh());

        const std::size_t boundaryEdgesAfter =
            boundary_edge_count(
                fixture.node->mesh());

        print_result(
            result.succeeded(),
            "Fill Hole foi executada");

        print_result(
            activeFacesBefore == 5u,
            "fixture possuia cinco faces ativas");

        print_result(
            activeFacesAfter == 6u,
            "Fill Hole restaurou seis faces ativas");

        print_result(
            faceSlotsAfter == faceSlotsBefore + 1u,
            "nova face utilizou um novo slot");

        print_result(
            boundaryEdgesBefore == 4u,
            "quatro boundary edges existiam antes");

        print_result(
            boundaryEdgesAfter == 0u,
            "buraco foi completamente fechado");

        bool edgesRemainValid = true;

        for (const EdgeHandle edge : fixture.edges) {
            if (!fixture.node->mesh().is_valid(edge)) {
                edgesRemainValid = false;
                break;
            }
        }

        print_result(
            edgesRemainValid,
            "edges originais continuam validas");

        print_result(
            fixture.history.undo_size() == 1u,
            "Fill Hole criou uma entrada no historico");

        print_result(
            fixture.history.undo_name()
            == "Fill Hole",
            "historico usa label Fill Hole");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Mesh),
            "Fill Hole marca Mesh como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Render),
            "Fill Hole marca Render como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Picking),
            "Fill Hole marca Picking como dirty");

        const CommandResult undoResult =
            fixture.history.undo(
                fixture.dispatcher);

        print_result(
            undoResult.success,
            "undo de Fill Hole funcionou");

        print_result(
            active_face_count(
                fixture.node->mesh())
            == activeFacesBefore,
            "undo restaurou cinco faces ativas");

        print_result(
            fixture.node->mesh().face_count()
            == faceSlotsBefore,
            "undo restaurou os slots anteriores");

        print_result(
            boundary_edge_count(
                fixture.node->mesh())
            == boundaryEdgesBefore,
            "undo reabriu o contorno");

        print_result(
            fixture.editor.selection()
            .mesh()
            .edges()
            .size()
            == fixture.edges.size(),
            "undo restaurou a selecao de edges");

        bool selectionRestored = true;

        for (const EdgeHandle edge : fixture.edges) {
            if (!fixture.editor.selection()
                .mesh()
                .edges()
                .contains(edge)) {
                selectionRestored = false;
                break;
            }
        }

        print_result(
            selectionRestored,
            "undo preservou todas as boundary edges selecionadas");

        print_result(
            fixture.history.can_redo(),
            "redo ficou disponivel");

        print_result(
            fixture.history.redo_name()
            == "Fill Hole",
            "redo preserva label Fill Hole");

        const CommandResult redoResult =
            fixture.history.redo(
                fixture.dispatcher);

        print_result(
            redoResult.success,
            "redo de Fill Hole funcionou");

        print_result(
            active_face_count(
                fixture.node->mesh())
            == activeFacesAfter,
            "redo restaurou seis faces ativas");

        print_result(
            fixture.node->mesh().face_count()
            == faceSlotsAfter,
            "redo restaurou o novo slot de face");

        print_result(
            boundary_edge_count(
                fixture.node->mesh())
            == 0u,
            "redo fechou novamente o buraco");

        print_result(
            fixture.history.undo_size() == 1u
            && fixture.history.redo_size() == 0u,
            "redo restaurou estado do historico");

        return result.succeeded()
            && undoResult.success
            && redoResult.success
            && active_face_count(
                fixture.node->mesh()) == 6u
            && boundary_edge_count(
                fixture.node->mesh()) == 0u
            && fixture.history.undo_size() == 1u;
    }

    bool test_non_boundary_edges_failure() {
        std::cout
            << "\n=== Fill Hole: non-boundary edges ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()) {
            print_result(
                false,
                "caixa fechada foi criada");
            return false;
        }

        const std::vector<EdgeHandle> selectedEdges{
            fixture.edges[0],
            fixture.edges[1],
            fixture.edges[2]
        };

        fixture.select_edges(selectedEdges);

        ActionRegistry registry{};

        if (!register_topology_actions(registry)) {
            print_result(
                false,
                "topology actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        print_result(
            boundary_edge_count(
                fixture.node->mesh()) == 0u,
            "caixa fechada nao possui boundary edges");

        print_result(
            executor.can_execute(
                fixture.context,
                fill_hole_action_id()),
            "validacao superficial aceita tres handles validos");

        const std::size_t faceSlotsBefore =
            fixture.node->mesh().face_count();

        const std::size_t activeFacesBefore =
            active_face_count(
                fixture.node->mesh());

        const ActionResult result =
            executor.execute(
                fixture.context,
                fill_hole_action_id());

        print_action_result(
            "non-boundary Fill Hole result",
            result);

        print_result(
            result.failed(),
            "kernel rejeita edges que nao sao de fronteira");

        print_result(
            fixture.history.empty(),
            "operacao rejeitada nao entra no historico");

        print_result(
            fixture.node->mesh().face_count()
            == faceSlotsBefore,
            "falha nao altera slots de faces");

        print_result(
            active_face_count(
                fixture.node->mesh())
            == activeFacesBefore,
            "falha nao altera faces ativas");

        print_result(
            boundary_edge_count(
                fixture.node->mesh()) == 0u,
            "falha preserva a caixa fechada");

        return result.failed()
            && fixture.history.empty()
            && fixture.node->mesh().face_count()
            == faceSlotsBefore
            && active_face_count(
                fixture.node->mesh())
            == activeFacesBefore;
    }

    bool test_too_few_edges_unavailable() {
        std::cout
            << "\n=== Fill Hole: too few edges ===\n";

        MeshFixture fixture{};

        if (!fixture.build_box()
            || !fixture.remove_face_and_select_boundary(
                fixture.faces.front())) {
            print_result(
                false,
                "fixture aberta foi criada");
            return false;
        }

        fixture.editor.selection()
            .mesh()
            .set_edge(fixture.edges.front());

        ActionRegistry registry{};

        if (!register_topology_actions(registry)) {
            print_result(
                false,
                "topology actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const std::size_t activeFacesBefore =
            active_face_count(
                fixture.node->mesh());

        const ActionResult result =
            executor.execute(
                fixture.context,
                fill_hole_action_id());

        print_action_result(
            "too few edges result",
            result);

        print_result(
            result.is_unavailable(),
            "uma edge retorna Unavailable");

        print_result(
            fixture.history.empty(),
            "action indisponivel nao entra no historico");

        print_result(
            active_face_count(
                fixture.node->mesh())
            == activeFacesBefore,
            "action indisponivel nao altera a malha");

        return result.is_unavailable()
            && fixture.history.empty();
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor Fill Hole Action "
        "Smoke Test ===\n";

    bool passed = true;

    passed = test_registration() && passed;
    passed = test_open_box_fixture() && passed;
    passed = test_availability() && passed;
    passed = test_fill_hole_execution() && passed;
    passed = test_non_boundary_edges_failure() && passed;
    passed = test_too_few_edges_unavailable() && passed;

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== All Fill Hole action smoke tests "
            "passed ===\n";
        return 0;
    }

    std::cout
        << "=== Fill Hole action smoke test failed ===\n";
    return 1;
}