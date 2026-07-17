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
#include "kernel/geometry/primitives/BoxBuilder.h"

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

    ActionId subdivide_edges_action_id() {
        return make_action_id(
            topology_actions::SubdivideEdgesId);
    }

    ActionId subdivide_faces_action_id() {
        return make_action_id(
            topology_actions::SubdivideFacesId);
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

        bool build() {
            nodeId =
                editor.scene().create_mesh(
                    "Topology Action Test Box");

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

            editor.selection()
                .mesh()
                .set_active_mesh(nodeId);

            editor.clear_dirty();

            return true;
        }

        void select_edge(EdgeHandle edge) {
            editor.selection().set_granularity(
                SelectionGranularity::Edge);

            editor.selection()
                .mesh()
                .set_edge(edge);

            editor.selection().mark_dirty();
            editor.clear_dirty();
        }

        void select_face(FaceHandle face) {
            editor.selection().set_granularity(
                SelectionGranularity::Face);

            editor.selection()
                .mesh()
                .set_face(face);

            editor.selection().mark_dirty();
            editor.clear_dirty();
        }
    };

    bool test_registration() {
        std::cout
            << "\n=== Topology actions: registration ===\n";

        ActionRegistry registry{};

        const bool registered =
            register_topology_actions(registry);

        print_result(
            registered,
            "topology actions foram registradas");

        print_result(
            registry.size() == 2u,
            "duas topology actions foram registradas");

        print_result(
            registry.contains(
                subdivide_edges_action_id()),
            "registry contem Subdivide Edges");

        print_result(
            registry.contains(
                subdivide_faces_action_id()),
            "registry contem Subdivide Faces");

        const ActionDescriptor* edgeDescriptor =
            registry.descriptor(
                subdivide_edges_action_id());

        const ActionDescriptor* faceDescriptor =
            registry.descriptor(
                subdivide_faces_action_id());

        print_result(
            edgeDescriptor != nullptr
            && edgeDescriptor->is_valid(),
            "descritor de Subdivide Edges e valido");

        print_result(
            faceDescriptor != nullptr
            && faceDescriptor->is_valid(),
            "descritor de Subdivide Faces e valido");

        print_result(
            edgeDescriptor
            && edgeDescriptor->name
            == "Subdivide Edges",
            "descritor preserva nome de edges");

        print_result(
            faceDescriptor
            && faceDescriptor->name
            == "Subdivide Faces",
            "descritor preserva nome de faces");

        print_result(
            edgeDescriptor
            && edgeDescriptor->category
            == ActionCategory::Mesh,
            "Subdivide Edges pertence a Mesh");

        print_result(
            faceDescriptor
            && faceDescriptor->category
            == ActionCategory::Mesh,
            "Subdivide Faces pertence a Mesh");

        const bool registeredAgain =
            register_topology_actions(registry);

        print_result(
            !registeredAgain,
            "registro duplicado e rejeitado");

        print_result(
            registry.size() == 2u,
            "registro duplicado preserva registry");

        return registered
            && !registeredAgain
            && registry.size() == 2u
            && edgeDescriptor
            && faceDescriptor;
    }

    bool test_transactional_registration() {
        std::cout
            << "\n=== Topology actions: transactional registration ===\n";

        ActionRegistry registry{};

        const bool firstRegistration =
            register_topology_actions(registry);

        print_result(
            firstRegistration,
            "primeiro registro funcionou");

        const bool removedFace =
            registry.unregister_action(
                subdivide_faces_action_id());

        print_result(
            removedFace,
            "Subdivide Faces foi removida para preparar conflito");

        print_result(
            registry.contains(
                subdivide_edges_action_id()),
            "Subdivide Edges continua registrada");

        const std::size_t sizeBefore =
            registry.size();

        const bool secondRegistration =
            register_topology_actions(registry);

        print_result(
            !secondRegistration,
            "registro falha quando primeiro id ja existe");

        print_result(
            registry.size() == sizeBefore,
            "falha inicial nao altera registry");

        print_result(
            !registry.contains(
                subdivide_faces_action_id()),
            "falha nao registra action posterior");

        return firstRegistration
            && removedFace
            && !secondRegistration
            && registry.size() == sizeBefore;
    }

    bool test_availability() {
        std::cout
            << "\n=== Topology actions: availability ===\n";

        MeshFixture fixture{};

        if (!fixture.build()) {
            print_result(false, "fixture foi criada");
            return false;
        }

        ActionRegistry registry{};

        if (!register_topology_actions(registry)) {
            print_result(false, "actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        fixture.select_edge(fixture.edges.front());

        print_result(
            executor.can_execute(
                fixture.context,
                subdivide_edges_action_id()),
            "Subdivide Edges aceita edge selecionada");

        print_result(
            !executor.can_execute(
                fixture.context,
                subdivide_faces_action_id()),
            "Subdivide Faces rejeita granularidade Edge");

        fixture.select_face(fixture.faces.front());

        print_result(
            executor.can_execute(
                fixture.context,
                subdivide_faces_action_id()),
            "Subdivide Faces aceita face selecionada");

        print_result(
            !executor.can_execute(
                fixture.context,
                subdivide_edges_action_id()),
            "Subdivide Edges rejeita granularidade Face");

        fixture.editor.selection()
            .mesh()
            .clear_components();

        print_result(
            !executor.can_execute(
                fixture.context,
                subdivide_faces_action_id()),
            "Subdivide Faces rejeita selecao vazia");

        fixture.select_face(fixture.faces.front());
        fixture.editor.set_mode(EditorMode::Object);

        print_result(
            !executor.can_execute(
                fixture.context,
                subdivide_faces_action_id()),
            "topology action rejeita Object mode");

        return true;
    }

    bool test_subdivide_edge() {
        std::cout
            << "\n=== Subdivide Edges: execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build()) {
            print_result(false, "fixture foi criada");
            return false;
        }

        const EdgeHandle targetEdge =
            fixture.edges.front();

        fixture.select_edge(targetEdge);

        ActionRegistry registry{};

        if (!register_topology_actions(registry)) {
            print_result(false, "actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const std::size_t verticesBefore =
            fixture.node->mesh().vertex_count();

        const std::size_t edgesBefore =
            fixture.node->mesh().edge_count();

        const std::size_t facesBefore =
            fixture.node->mesh().face_count();

        const ActionResult result =
            executor.execute(
                fixture.context,
                subdivide_edges_action_id());

        print_action_result(
            "Subdivide Edges result",
            result);

        const std::size_t verticesAfter =
            fixture.node->mesh().vertex_count();

        print_result(
            result.succeeded(),
            "Subdivide Edges foi executada");

        print_result(
            verticesAfter == verticesBefore + 1u,
            "subdivisao criou um vertice central na edge");

        print_result(
            fixture.node->mesh().edge_count()
                > edgesBefore,
            "subdivisao aumentou armazenamento de edges");

        print_result(
            fixture.node->mesh().face_count()
            == facesBefore,
            "subdivisao de edge preservou slots de faces");

        print_result(
            fixture.history.undo_size() == 1u,
            "subdivisao criou uma entrada no historico");

        print_result(
            fixture.history.undo_name()
            == "Subdivide Edges",
            "historico usa label Subdivide Edges");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Mesh),
            "subdivisao marca Mesh como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Render),
            "subdivisao marca Render como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Picking),
            "subdivisao marca Picking como dirty");

        const CommandResult undoResult =
            fixture.history.undo(
                fixture.dispatcher);

        print_result(
            undoResult.success,
            "undo da subdivisao de edge funcionou");

        print_result(
            fixture.node->mesh().vertex_count()
            == verticesBefore,
            "undo restaurou quantidade de vertices");

        print_result(
            fixture.node->mesh().edge_count()
            == edgesBefore,
            "undo restaurou quantidade de edges");

        print_result(
            fixture.node->mesh().face_count()
            == facesBefore,
            "undo restaurou quantidade de faces");

        print_result(
            fixture.node->mesh().is_valid(targetEdge),
            "undo restaurou a edge original");

        print_result(
            fixture.editor.selection()
            .mesh()
            .edges()
            .contains(targetEdge),
            "undo restaurou selecao da edge");

        const CommandResult redoResult =
            fixture.history.redo(
                fixture.dispatcher);

        print_result(
            redoResult.success,
            "redo da subdivisao de edge funcionou");

        print_result(
            fixture.node->mesh().vertex_count()
            == verticesAfter,
            "redo restaurou o novo vertice");

        print_result(
            fixture.history.undo_size() == 1u
            && fixture.history.redo_size() == 0u,
            "redo restaurou estado do historico");

        return result.succeeded()
            && undoResult.success
            && redoResult.success
            && fixture.node->mesh().vertex_count()
            == verticesAfter;
    }

    bool test_subdivide_face() {
        std::cout
            << "\n=== Subdivide Faces: execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build()) {
            print_result(false, "fixture foi criada");
            return false;
        }

        const FaceHandle targetFace =
            fixture.faces.front();

        fixture.select_face(targetFace);

        ActionRegistry registry{};

        if (!register_topology_actions(registry)) {
            print_result(false, "actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const std::size_t verticesBefore =
            fixture.node->mesh().vertex_count();

        const std::size_t edgesBefore =
            fixture.node->mesh().edge_count();

        const std::size_t facesBefore =
            fixture.node->mesh().face_count();

        const ActionResult result =
            executor.execute(
                fixture.context,
                subdivide_faces_action_id());

        print_action_result(
            "Subdivide Faces result",
            result);

        const std::size_t verticesAfter =
            fixture.node->mesh().vertex_count();

        const std::size_t edgesAfter =
            fixture.node->mesh().edge_count();

        const std::size_t facesAfter =
            fixture.node->mesh().face_count();

        print_result(
            result.succeeded(),
            "Subdivide Faces foi executada");

        print_result(
            verticesAfter == verticesBefore + 1u,
            "subdivisao criou um vertice central");

        print_result(
            edgesAfter > edgesBefore,
            "subdivisao criou novas edges");

        print_result(
            facesAfter > facesBefore,
            "subdivisao criou novas faces");

        print_result(
            !fixture.node->mesh().is_valid(targetFace),
            "face original deixou de estar ativa");

        print_result(
            fixture.history.undo_size() == 1u,
            "subdivisao criou uma entrada no historico");

        print_result(
            fixture.history.undo_name()
            == "Subdivide Faces",
            "historico usa label Subdivide Faces");

        const CommandResult undoResult =
            fixture.history.undo(
                fixture.dispatcher);

        print_result(
            undoResult.success,
            "undo da subdivisao de face funcionou");

        print_result(
            fixture.node->mesh().vertex_count()
            == verticesBefore,
            "undo restaurou quantidade de vertices");

        print_result(
            fixture.node->mesh().edge_count()
            == edgesBefore,
            "undo restaurou quantidade de edges");

        print_result(
            fixture.node->mesh().face_count()
            == facesBefore,
            "undo restaurou quantidade de faces");

        print_result(
            fixture.node->mesh().is_valid(targetFace),
            "undo restaurou a face original");

        print_result(
            fixture.editor.selection()
            .mesh()
            .faces()
            .contains(targetFace),
            "undo restaurou selecao da face");

        const CommandResult redoResult =
            fixture.history.redo(
                fixture.dispatcher);

        print_result(
            redoResult.success,
            "redo da subdivisao de face funcionou");

        print_result(
            fixture.node->mesh().vertex_count()
            == verticesAfter,
            "redo restaurou quantidade subdividida de vertices");

        print_result(
            fixture.node->mesh().edge_count()
            == edgesAfter,
            "redo restaurou quantidade subdividida de edges");

        print_result(
            fixture.node->mesh().face_count()
            == facesAfter,
            "redo restaurou quantidade subdividida de faces");

        print_result(
            !fixture.node->mesh().is_valid(targetFace),
            "redo removeu novamente a face original");

        return result.succeeded()
            && undoResult.success
            && redoResult.success
            && !fixture.node->mesh().is_valid(targetFace);
    }

    bool test_unavailable_execution() {
        std::cout
            << "\n=== Topology actions: unavailable execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build()) {
            print_result(false, "fixture foi criada");
            return false;
        }

        ActionRegistry registry{};

        if (!register_topology_actions(registry)) {
            print_result(false, "actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        fixture.editor.selection().set_granularity(
            SelectionGranularity::Face);

        fixture.editor.selection()
            .mesh()
            .clear_components();

        const std::size_t verticesBefore =
            fixture.node->mesh().vertex_count();

        const ActionResult result =
            executor.execute(
                fixture.context,
                subdivide_faces_action_id());

        print_action_result(
            "Unavailable Subdivide Faces result",
            result);

        print_result(
            result.is_unavailable(),
            "selecao vazia retorna Unavailable");

        print_result(
            fixture.history.empty(),
            "action indisponivel nao entra no historico");

        print_result(
            fixture.node->mesh().vertex_count()
            == verticesBefore,
            "action indisponivel nao altera malha");

        return result.is_unavailable()
            && fixture.history.empty();
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor Topology Actions "
        "Smoke Test ===\n";

    bool passed = true;

    passed = test_registration() && passed;
    passed = test_transactional_registration() && passed;
    passed = test_availability() && passed;
    passed = test_subdivide_edge() && passed;
    passed = test_subdivide_face() && passed;
    passed = test_unavailable_execution() && passed;

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== All topology action smoke tests "
            "passed ===\n";
        return 0;
    }

    std::cout
        << "=== Topology action smoke test failed ===\n";
    return 1;
}