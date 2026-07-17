/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/actions/ActionExecutor.h"
#include "editor/actions/ActionRegistry.h"
#include "editor/actions/core/ActionContext.h"
#include "editor/actions/mesh/vertex/RegisterVertexActions.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "kernel/geometry/mesh/LEMEditor.h"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

    using namespace locus::editor;

    using VertexHandle =
        locus::kernel::geometry::VertexHandle;

    constexpr float PositionEpsilon = 0.00001f;

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
        float epsilon = PositionEpsilon) {
        return glm::length(first - second) <= epsilon;
    }

    ActionId make_action_id(
        std::string_view value) {
        return ActionId{
            std::string{ value }
        };
    }

    ActionId merge_at_center_action_id() {
        return make_action_id(
            vertex_actions::MergeAtCenterId);
    }

    ActionId merge_at_first_action_id() {
        return make_action_id(
            vertex_actions::MergeAtFirstId);
    }

    ActionId merge_at_last_action_id() {
        return make_action_id(
            vertex_actions::MergeAtLastId);
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

        std::vector<VertexHandle> vertices{};

        bool build_vertices() {
            nodeId =
                editor.scene().create_mesh(
                    "Vertex Actions Test Mesh");

            node =
                editor.scene().find_mesh(nodeId);

            if (!node) {
                return false;
            }

            locus::kernel::geometry::LEMEditor meshEditor{
                node->mesh()
            };

            const VertexHandle vertexA =
                meshEditor.add_vertex({
                    0.0f,
                    0.0f,
                    0.0f
                    });

            const VertexHandle vertexB =
                meshEditor.add_vertex({
                    3.0f,
                    0.0f,
                    0.0f
                    });

            const VertexHandle vertexC =
                meshEditor.add_vertex({
                    0.0f,
                    6.0f,
                    0.0f
                    });

            if (!node->mesh().is_valid(vertexA)
                || !node->mesh().is_valid(vertexB)
                || !node->mesh().is_valid(vertexC)) {
                return false;
            }

            vertices = {
                vertexA,
                vertexB,
                vertexC
            };

            editor.set_mode(EditorMode::Mesh);

            editor.selection().set_granularity(
                SelectionGranularity::Vertex);

            editor.selection()
                .mesh()
                .set_active_mesh(nodeId);

            editor.clear_dirty();

            return true;
        }

        void select_vertices(
            const std::vector<VertexHandle>& selectedVertices) {
            editor.selection().set_granularity(
                SelectionGranularity::Vertex);

            editor.selection()
                .mesh()
                .clear_components();

            if (selectedVertices.empty()) {
                editor.selection().mark_dirty();
                editor.clear_dirty();
                return;
            }

            editor.selection()
                .mesh()
                .set_vertex(selectedVertices.front());

            for (std::size_t index = 1u;
                index < selectedVertices.size();
                ++index) {
                editor.selection()
                    .mesh()
                    .add_vertex(selectedVertices[index]);
            }

            editor.selection().mark_dirty();
            editor.clear_dirty();
        }

        std::size_t active_original_vertices() const {
            std::size_t count = 0u;

            for (const VertexHandle vertex : vertices) {
                if (node->mesh().is_valid(vertex)) {
                    ++count;
                }
            }

            return count;
        }
    };

    bool test_registration() {
        std::cout
            << "\n=== Vertex actions: registration ===\n";

        ActionRegistry registry{};

        const bool registered =
            register_vertex_actions(registry);

        print_result(
            registered,
            "vertex actions foram registradas");

        print_result(
            registry.size() == 3u,
            "tres vertex actions foram registradas");

        print_result(
            registry.contains(
                merge_at_center_action_id()),
            "registry contem Merge at Center");

        print_result(
            registry.contains(
                merge_at_first_action_id()),
            "registry contem Merge at First");

        print_result(
            registry.contains(
                merge_at_last_action_id()),
            "registry contem Merge at Last");

        const ActionDescriptor* centerDescriptor =
            registry.descriptor(
                merge_at_center_action_id());

        const ActionDescriptor* firstDescriptor =
            registry.descriptor(
                merge_at_first_action_id());

        const ActionDescriptor* lastDescriptor =
            registry.descriptor(
                merge_at_last_action_id());

        print_result(
            centerDescriptor
            && centerDescriptor->is_valid(),
            "descritor de Merge at Center e valido");

        print_result(
            firstDescriptor
            && firstDescriptor->is_valid(),
            "descritor de Merge at First e valido");

        print_result(
            lastDescriptor
            && lastDescriptor->is_valid(),
            "descritor de Merge at Last e valido");

        print_result(
            centerDescriptor
            && centerDescriptor->name
            == "Merge at Center",
            "nome de Merge at Center foi preservado");

        print_result(
            firstDescriptor
            && firstDescriptor->name
            == "Merge at First",
            "nome de Merge at First foi preservado");

        print_result(
            lastDescriptor
            && lastDescriptor->name
            == "Merge at Last",
            "nome de Merge at Last foi preservado");

        print_result(
            centerDescriptor
            && centerDescriptor->category
            == ActionCategory::Mesh,
            "Merge at Center pertence a Mesh");

        const bool registeredAgain =
            register_vertex_actions(registry);

        print_result(
            !registeredAgain,
            "registro duplicado e rejeitado");

        print_result(
            registry.size() == 3u,
            "registro duplicado preserva registry");

        return registered
            && !registeredAgain
            && registry.size() == 3u
            && centerDescriptor
            && firstDescriptor
            && lastDescriptor;
    }

    bool test_transactional_registration() {
        std::cout
            << "\n=== Vertex actions: transactional registration ===\n";

        ActionRegistry registry{};

        const bool firstRegistration =
            register_vertex_actions(registry);

        print_result(
            firstRegistration,
            "primeiro registro funcionou");

        const bool removedFirst =
            registry.unregister_action(
                merge_at_first_action_id());

        const bool removedLast =
            registry.unregister_action(
                merge_at_last_action_id());

        print_result(
            removedFirst && removedLast,
            "actions posteriores foram removidas");

        print_result(
            registry.size() == 1u,
            "apenas Merge at Center permaneceu");

        const bool secondRegistration =
            register_vertex_actions(registry);

        print_result(
            !secondRegistration,
            "registro falha no primeiro ID duplicado");

        print_result(
            registry.size() == 1u,
            "falha inicial nao altera registry");

        print_result(
            !registry.contains(
                merge_at_first_action_id())
            && !registry.contains(
                merge_at_last_action_id()),
            "actions posteriores nao foram registradas");

        return firstRegistration
            && removedFirst
            && removedLast
            && !secondRegistration
            && registry.size() == 1u;
    }

    bool test_availability() {
        std::cout
            << "\n=== Vertex actions: availability ===\n";

        MeshFixture fixture{};

        if (!fixture.build_vertices()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        ActionRegistry registry{};

        if (!register_vertex_actions(registry)) {
            print_result(
                false,
                "vertex actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        fixture.select_vertices({
            fixture.vertices[2],
            fixture.vertices[0]
            });

        print_result(
            executor.can_execute(
                fixture.context,
                merge_at_center_action_id()),
            "Merge at Center aceita dois vertices");

        print_result(
            executor.can_execute(
                fixture.context,
                merge_at_first_action_id()),
            "Merge at First aceita dois vertices");

        print_result(
            executor.can_execute(
                fixture.context,
                merge_at_last_action_id()),
            "Merge at Last aceita dois vertices");

        fixture.select_vertices({
            fixture.vertices[0]
            });

        print_result(
            !executor.can_execute(
                fixture.context,
                merge_at_center_action_id()),
            "Merge at Center rejeita um vertice");

        print_result(
            !executor.can_execute(
                fixture.context,
                merge_at_first_action_id()),
            "Merge at First rejeita um vertice");

        print_result(
            !executor.can_execute(
                fixture.context,
                merge_at_last_action_id()),
            "Merge at Last rejeita um vertice");

        fixture.select_vertices({
            fixture.vertices[0],
            fixture.vertices[1]
            });

        fixture.editor.selection().set_granularity(
            SelectionGranularity::Edge);

        print_result(
            !executor.can_execute(
                fixture.context,
                merge_at_center_action_id()),
            "merge rejeita granularidade Edge");

        fixture.editor.selection().set_granularity(
            SelectionGranularity::Vertex);

        fixture.editor.set_mode(EditorMode::Object);

        print_result(
            !executor.can_execute(
                fixture.context,
                merge_at_center_action_id()),
            "merge rejeita Object mode");

        fixture.editor.set_mode(EditorMode::Mesh);

        print_result(
            executor.can_execute(
                fixture.context,
                merge_at_center_action_id()),
            "merge volta a ficar disponivel em Mesh mode");

        return true;
    }

    bool test_merge_at_center() {
        std::cout
            << "\n=== Merge at Center: execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build_vertices()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        const VertexHandle firstSelected =
            fixture.vertices[2];

        const VertexHandle secondSelected =
            fixture.vertices[0];

        const VertexHandle lastSelected =
            fixture.vertices[1];

        fixture.select_vertices({
            firstSelected,
            secondSelected,
            lastSelected
            });

        const glm::vec3 expectedCenter{
            1.0f,
            2.0f,
            0.0f
        };

        ActionRegistry registry{};

        if (!register_vertex_actions(registry)) {
            print_result(
                false,
                "vertex actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const ActionResult result =
            executor.execute(
                fixture.context,
                merge_at_center_action_id());

        print_action_result(
            "Merge at Center result",
            result);

        print_result(
            result.succeeded(),
            "Merge at Center foi executada");

        print_result(
            fixture.node->mesh().is_valid(
                firstSelected),
            "primeiro vertice selecionado sobreviveu");

        print_result(
            !fixture.node->mesh().is_valid(
                secondSelected),
            "segundo vertice foi absorvido");

        print_result(
            !fixture.node->mesh().is_valid(
                lastSelected),
            "ultimo vertice foi absorvido");

        print_result(
            fixture.active_original_vertices() == 1u,
            "restou um vertice original ativo");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .vertex(firstSelected)
                .position,
                expectedCenter),
            "vertice sobrevivente foi movido para o centro");

        print_result(
            fixture.history.undo_size() == 1u,
            "merge criou uma entrada no historico");

        print_result(
            fixture.history.undo_name()
            == "Merge Vertices at Center",
            "historico usa label de center");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Mesh),
            "merge marca Mesh como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Render),
            "merge marca Render como dirty");

        print_result(
            has_flag(
                fixture.editor.dirty_flags(),
                EditorDirtyFlags::Picking),
            "merge marca Picking como dirty");

        const CommandResult undoResult =
            fixture.history.undo(
                fixture.dispatcher);

        print_result(
            undoResult.success,
            "undo de Merge at Center funcionou");

        print_result(
            fixture.active_original_vertices() == 3u,
            "undo restaurou os tres vertices");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .vertex(firstSelected)
                .position,
                glm::vec3{
                    0.0f,
                    6.0f,
                    0.0f
                }),
            "undo restaurou posicao do primeiro selecionado");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .vertex(secondSelected)
                .position,
                glm::vec3{
                    0.0f,
                    0.0f,
                    0.0f
                }),
            "undo restaurou posicao do segundo selecionado");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .vertex(lastSelected)
                .position,
                glm::vec3{
                    3.0f,
                    0.0f,
                    0.0f
                }),
            "undo restaurou posicao do ultimo selecionado");

        print_result(
            fixture.editor.selection()
            .mesh()
            .vertices()
            .items()
            == std::vector<VertexHandle>{
            firstSelected,
                secondSelected,
                lastSelected
        },
            "undo restaurou ordem da selecao");

        const CommandResult redoResult =
            fixture.history.redo(
                fixture.dispatcher);

        print_result(
            redoResult.success,
            "redo de Merge at Center funcionou");

        print_result(
            fixture.active_original_vertices() == 1u,
            "redo restaurou um unico vertice");

        print_result(
            fixture.node->mesh().is_valid(
                firstSelected)
            && approximately_equal(
                fixture.node->mesh()
                .vertex(firstSelected)
                .position,
                expectedCenter),
            "redo restaurou o resultado centralizado");

        return result.succeeded()
            && undoResult.success
            && redoResult.success
            && fixture.active_original_vertices() == 1u;
    }

    bool test_merge_at_first() {
        std::cout
            << "\n=== Merge at First: execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build_vertices()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        const VertexHandle firstSelected =
            fixture.vertices[2];

        const VertexHandle secondSelected =
            fixture.vertices[0];

        const VertexHandle lastSelected =
            fixture.vertices[1];

        fixture.select_vertices({
            firstSelected,
            secondSelected,
            lastSelected
            });

        const glm::vec3 expectedPosition =
            fixture.node->mesh()
            .vertex(firstSelected)
            .position;

        ActionRegistry registry{};

        if (!register_vertex_actions(registry)) {
            print_result(
                false,
                "vertex actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const ActionResult result =
            executor.execute(
                fixture.context,
                merge_at_first_action_id());

        print_action_result(
            "Merge at First result",
            result);

        print_result(
            result.succeeded(),
            "Merge at First foi executada");

        print_result(
            fixture.node->mesh().is_valid(
                firstSelected),
            "primeiro selecionado sobreviveu");

        print_result(
            !fixture.node->mesh().is_valid(
                secondSelected)
            && !fixture.node->mesh().is_valid(
                lastSelected),
            "demais vertices foram absorvidos");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .vertex(firstSelected)
                .position,
                expectedPosition),
            "posicao do primeiro selecionado foi preservada");

        print_result(
            fixture.history.undo_name()
            == "Merge Vertices at First",
            "historico usa label de first");

        const CommandResult undoResult =
            fixture.history.undo(
                fixture.dispatcher);

        print_result(
            undoResult.success,
            "undo de Merge at First funcionou");

        print_result(
            fixture.active_original_vertices() == 3u,
            "undo restaurou os tres vertices");

        const CommandResult redoResult =
            fixture.history.redo(
                fixture.dispatcher);

        print_result(
            redoResult.success,
            "redo de Merge at First funcionou");

        print_result(
            fixture.node->mesh().is_valid(
                firstSelected)
            && fixture.active_original_vertices() == 1u,
            "redo preservou o primeiro selecionado");

        return result.succeeded()
            && undoResult.success
            && redoResult.success
            && fixture.node->mesh().is_valid(
                firstSelected)
            && fixture.active_original_vertices() == 1u;
    }

    bool test_merge_at_last() {
        std::cout
            << "\n=== Merge at Last: execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build_vertices()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        const VertexHandle firstSelected =
            fixture.vertices[2];

        const VertexHandle secondSelected =
            fixture.vertices[0];

        const VertexHandle lastSelected =
            fixture.vertices[1];

        fixture.select_vertices({
            firstSelected,
            secondSelected,
            lastSelected
            });

        const glm::vec3 expectedPosition =
            fixture.node->mesh()
            .vertex(lastSelected)
            .position;

        ActionRegistry registry{};

        if (!register_vertex_actions(registry)) {
            print_result(
                false,
                "vertex actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const ActionResult result =
            executor.execute(
                fixture.context,
                merge_at_last_action_id());

        print_action_result(
            "Merge at Last result",
            result);

        print_result(
            result.succeeded(),
            "Merge at Last foi executada");

        print_result(
            fixture.node->mesh().is_valid(
                lastSelected),
            "ultimo selecionado sobreviveu");

        print_result(
            !fixture.node->mesh().is_valid(
                firstSelected)
            && !fixture.node->mesh().is_valid(
                secondSelected),
            "vertices anteriores foram absorvidos");

        print_result(
            approximately_equal(
                fixture.node->mesh()
                .vertex(lastSelected)
                .position,
                expectedPosition),
            "posicao do ultimo selecionado foi preservada");

        print_result(
            fixture.history.undo_name()
            == "Merge Vertices at Last",
            "historico usa label de last");

        const CommandResult undoResult =
            fixture.history.undo(
                fixture.dispatcher);

        print_result(
            undoResult.success,
            "undo de Merge at Last funcionou");

        print_result(
            fixture.active_original_vertices() == 3u,
            "undo restaurou os tres vertices");

        const CommandResult redoResult =
            fixture.history.redo(
                fixture.dispatcher);

        print_result(
            redoResult.success,
            "redo de Merge at Last funcionou");

        print_result(
            fixture.node->mesh().is_valid(
                lastSelected)
            && fixture.active_original_vertices() == 1u,
            "redo preservou o ultimo selecionado");

        return result.succeeded()
            && undoResult.success
            && redoResult.success
            && fixture.node->mesh().is_valid(
                lastSelected)
            && fixture.active_original_vertices() == 1u;
    }

    bool test_unavailable_execution() {
        std::cout
            << "\n=== Vertex actions: unavailable execution ===\n";

        MeshFixture fixture{};

        if (!fixture.build_vertices()) {
            print_result(
                false,
                "fixture foi criada");
            return false;
        }

        fixture.select_vertices({
            fixture.vertices.front()
            });

        ActionRegistry registry{};

        if (!register_vertex_actions(registry)) {
            print_result(
                false,
                "vertex actions foram registradas");
            return false;
        }

        ActionExecutor executor{ registry };

        const ActionResult result =
            executor.execute(
                fixture.context,
                merge_at_center_action_id());

        print_action_result(
            "Unavailable Merge at Center result",
            result);

        print_result(
            result.is_unavailable(),
            "um vertice retorna Unavailable");

        print_result(
            fixture.history.empty(),
            "action indisponivel nao entra no historico");

        print_result(
            fixture.active_original_vertices() == 3u,
            "action indisponivel nao altera a malha");

        return result.is_unavailable()
            && fixture.history.empty()
            && fixture.active_original_vertices() == 3u;
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor Final Vertex Actions "
        "Smoke Test ===\n";

    bool passed = true;

    passed = test_registration() && passed;
    passed = test_transactional_registration() && passed;
    passed = test_availability() && passed;
    passed = test_merge_at_center() && passed;
    passed = test_merge_at_first() && passed;
    passed = test_merge_at_last() && passed;
    passed = test_unavailable_execution() && passed;

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== All final vertex action smoke tests "
            "passed ===\n";
        return 0;
    }

    std::cout
        << "=== Final vertex action smoke test failed ===\n";
    return 1;
}