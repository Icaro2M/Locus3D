// SPDX-FileCopyrightText: 2026 Icaro2M
// SPDX-License-Identifier: Apache-2.0

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/mesh/ApplyMeshOperationCommand.h"
#include "editor/command/mesh/EditMeshSelectionCommand.h"
#include "editor/command/mesh/ReplaceMeshCommand.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "kernel/common/Id.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/vec3.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

    using namespace locus::editor;
    using namespace locus::kernel;
    using namespace locus::kernel::geometry;

    int g_failures = 0;

    void check(bool condition, const std::string& message)
    {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return;
        }

        std::cout << "[FAIL] " << message << '\n';
        ++g_failures;
    }

    void print_result(const std::string& label, const CommandResult& result)
    {
        std::cout << label << '\n';
        std::cout << "  success: " << (result.success ? "true" : "false") << '\n';
        std::cout << "  message: " << result.message << '\n';
    }

    void print_mesh_counts(const std::string& label, const LEM& mesh)
    {
        std::cout << label << '\n';
        std::cout << "  vertices: " << mesh.vertex_count() << '\n';
        std::cout << "  edges: " << mesh.edge_count() << '\n';
        std::cout << "  loops: " << mesh.loop_count() << '\n';
        std::cout << "  faces: " << mesh.face_count() << '\n';
    }

    LEM make_quad_mesh()
    {
        LEM mesh{};
        LEMEditor editor(mesh);

        const VertexHandle v0 = editor.add_vertex(glm::vec3{ -1.0f, 0.0f, -1.0f });
        const VertexHandle v1 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f, -1.0f });
        const VertexHandle v2 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f,  1.0f });
        const VertexHandle v3 = editor.add_vertex(glm::vec3{ -1.0f, 0.0f,  1.0f });

        editor.add_face(std::vector<VertexHandle>{ v0, v1, v2, v3 });
        return mesh;
    }

    VertexHandle first_valid_vertex(const LEM& mesh)
    {
        for (std::size_t i = 1; i <= mesh.vertex_count(); ++i) {
            const VertexHandle handle(static_cast<IdValue>(i));
            if (mesh.is_valid(handle)) {
                return handle;
            }
        }

        return {};
    }

    MeshNode* find_mesh(Editor& editor, SceneNodeId id)
    {
        return editor.scene().find_mesh(id);
    }

    const MeshNode* find_mesh(const Editor& editor, SceneNodeId id)
    {
        return editor.scene().find_mesh(id);
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor Mesh Commands Smoke Test ===\n\n";

    Editor editor;
    CommandDispatcher dispatcher(editor);
    HistoryStack history;

    const SceneNodeId meshNodeId = editor.scene().create_mesh("Mesh Command Test");
    check(meshNodeId.is_valid(), "mesh node criado com id valido");

    MeshNode* meshNode = find_mesh(editor, meshNodeId);
    check(meshNode != nullptr, "mesh node pode ser encontrado como MeshNode");

    if (!meshNode) {
        std::cout << "\nTeste interrompido: mesh node nao encontrado.\n";
        return 1;
    }

    check(meshNode->mesh().empty(), "mesh inicial comeca vazia");
    check(history.empty(), "historico comeca vazio");

    std::cout << "\n=== ReplaceMeshCommand ===\n";

    {
        auto command = std::make_unique<ReplaceMeshCommand>(
            meshNodeId,
            make_quad_mesh(),
            true);

        const CommandResult result = history.execute(dispatcher, std::move(command));
        print_result("replace mesh", result);

        check(result.success, "replace mesh executou com sucesso");
        check(history.undo_size() == 1u, "replace mesh entrou no historico");
        check(history.redo_size() == 0u, "redo continua vazio apos replace");

        const MeshNode* node = find_mesh(editor, meshNodeId);
        check(node != nullptr, "mesh node ainda existe apos replace");

        if (node) {
            print_mesh_counts("malha apos replace", node->mesh());
            check(node->mesh().vertex_count() == 4u, "replace criou 4 vertices");
            check(node->mesh().edge_count() == 4u, "replace criou 4 edges");
            check(node->mesh().loop_count() == 4u, "replace criou 4 loops");
            check(node->mesh().face_count() == 1u, "replace criou 1 face");
        }
    }

    std::cout << "\n=== Undo/Redo ReplaceMeshCommand ===\n";

    {
        const CommandResult undoResult = history.undo(dispatcher);
        print_result("undo replace", undoResult);
        check(undoResult.success, "undo replace executou com sucesso");
        check(history.undo_size() == 0u, "undo removeu comando da pilha de undo");
        check(history.redo_size() == 1u, "undo colocou comando na pilha de redo");

        const MeshNode* node = find_mesh(editor, meshNodeId);
        check(node != nullptr, "mesh node ainda existe apos undo replace");

        if (node) {
            print_mesh_counts("malha apos undo replace", node->mesh());
            check(node->mesh().empty(), "undo replace restaurou malha vazia");
        }

        const CommandResult redoResult = history.redo(dispatcher);
        print_result("redo replace", redoResult);
        check(redoResult.success, "redo replace executou com sucesso");
        check(history.undo_size() == 1u, "redo devolveu comando para pilha de undo");
        check(history.redo_size() == 0u, "redo limpou pilha de redo");

        node = find_mesh(editor, meshNodeId);
        check(node != nullptr, "mesh node ainda existe apos redo replace");

        if (node) {
            print_mesh_counts("malha apos redo replace", node->mesh());
            check(node->mesh().vertex_count() == 4u, "redo replace restaurou 4 vertices");
            check(node->mesh().edge_count() == 4u, "redo replace restaurou 4 edges");
            check(node->mesh().loop_count() == 4u, "redo replace restaurou 4 loops");
            check(node->mesh().face_count() == 1u, "redo replace restaurou 1 face");
        }
    }

    std::cout << "\n=== ApplyMeshOperationCommand ===\n";

    {
        auto command = std::make_unique<ApplyMeshOperationCommand>(
            meshNodeId,
            [](LEMEditor& meshEditor) {
                const VertexHandle created = meshEditor.add_vertex(glm::vec3{ 0.0f, 2.0f, 0.0f });
                return created.is_valid();
            },
            "Add Loose Vertex");

        const CommandResult result = history.execute(dispatcher, std::move(command));
        print_result("apply mesh operation", result);

        check(result.success, "apply mesh operation executou com sucesso");
        check(history.undo_size() == 2u, "apply mesh operation entrou no historico");

        const MeshNode* node = find_mesh(editor, meshNodeId);
        check(node != nullptr, "mesh node ainda existe apos apply operation");

        if (node) {
            print_mesh_counts("malha apos apply operation", node->mesh());
            check(node->mesh().vertex_count() == 5u, "apply operation adicionou 1 vertice");
            check(node->mesh().edge_count() == 4u, "apply operation preservou edges");
            check(node->mesh().face_count() == 1u, "apply operation preservou faces");
        }
    }

    std::cout << "\n=== Undo/Redo ApplyMeshOperationCommand ===\n";

    {
        const CommandResult undoResult = history.undo(dispatcher);
        print_result("undo apply operation", undoResult);
        check(undoResult.success, "undo apply operation executou com sucesso");

        const MeshNode* node = find_mesh(editor, meshNodeId);
        check(node != nullptr, "mesh node ainda existe apos undo operation");

        if (node) {
            print_mesh_counts("malha apos undo operation", node->mesh());
            check(node->mesh().vertex_count() == 4u, "undo operation voltou para 4 vertices");
            check(node->mesh().edge_count() == 4u, "undo operation preservou 4 edges");
            check(node->mesh().face_count() == 1u, "undo operation preservou 1 face");
        }

        const CommandResult redoResult = history.redo(dispatcher);
        print_result("redo apply operation", redoResult);
        check(redoResult.success, "redo apply operation executou com sucesso");

        node = find_mesh(editor, meshNodeId);
        check(node != nullptr, "mesh node ainda existe apos redo operation");

        if (node) {
            print_mesh_counts("malha apos redo operation", node->mesh());
            check(node->mesh().vertex_count() == 5u, "redo operation restaurou 5 vertices");
            check(node->mesh().edge_count() == 4u, "redo operation preservou 4 edges");
            check(node->mesh().face_count() == 1u, "redo operation preservou 1 face");
        }
    }

    std::cout << "\n=== ApplyMeshOperationCommand rollback em falha ===\n";

    {
        const std::size_t undoBefore = history.undo_size();

        auto command = std::make_unique<ApplyMeshOperationCommand>(
            meshNodeId,
            [](LEMEditor& meshEditor) {
                meshEditor.add_vertex(glm::vec3{ 9.0f, 9.0f, 9.0f });
                return false;
            },
            "Failing Mesh Operation");

        const CommandResult result = history.execute(dispatcher, std::move(command));
        print_result("failing mesh operation", result);

        check(!result.success, "operacao com retorno false falhou");
        check(history.undo_size() == undoBefore, "operacao falha nao entrou no historico");

        const MeshNode* node = find_mesh(editor, meshNodeId);
        check(node != nullptr, "mesh node ainda existe apos falha");

        if (node) {
            print_mesh_counts("malha apos falha", node->mesh());
            check(node->mesh().vertex_count() == 5u, "rollback removeu vertice criado pela operacao falha");
            check(node->mesh().edge_count() == 4u, "rollback preservou edges anteriores");
            check(node->mesh().face_count() == 1u, "rollback preservou faces anteriores");
        }
    }

    std::cout << "\n=== EditMeshSelectionCommand ===\n";

    {
        const MeshNode* node = find_mesh(editor, meshNodeId);
        check(node != nullptr, "mesh node existe antes de editar selecao");

        const VertexHandle firstVertex = node ? first_valid_vertex(node->mesh()) : VertexHandle{};
        check(firstVertex.is_valid(), "primeiro vertice valido encontrado para selecao");

        auto command = std::make_unique<EditMeshSelectionCommand>(
            meshNodeId,
            [meshNodeId, firstVertex](LEM& mesh, SelectionState& selection) {
                if (!firstVertex.is_valid() || !mesh.is_valid(firstVertex)) {
                    return false;
                }

                selection.set_granularity(SelectionGranularity::Vertex);
                selection.set_scope(SelectionScope::ActiveMesh);
                selection.mesh().set_active_mesh(meshNodeId);
                selection.mesh().add_vertex(firstVertex);

                mesh.vertex(firstVertex).selected = true;
                return true;
            },
            "Select First Mesh Vertex");

        const CommandResult result = history.execute(dispatcher, std::move(command));
        print_result("edit mesh selection", result);

        check(result.success, "edit mesh selection executou com sucesso");
        check(history.undo_size() == 3u, "edit mesh selection entrou no historico");
        check(editor.selection().granularity() == SelectionGranularity::Vertex, "granularidade virou Vertex");
        check(editor.selection().scope() == SelectionScope::ActiveMesh, "scope virou ActiveMesh");
        check(editor.selection().mesh().active_mesh() == meshNodeId, "active mesh aponta para o mesh node");
        check(editor.selection().mesh().vertices().size() == 1u, "selecao de vertices tem 1 item");

        node = find_mesh(editor, meshNodeId);
        if (node && firstVertex.is_valid()) {
            check(node->mesh().vertex(firstVertex).selected, "flag selected do vertice foi marcada na LEM");
        }
    }

    std::cout << "\n=== Undo/Redo EditMeshSelectionCommand ===\n";

    {
        const MeshNode* node = find_mesh(editor, meshNodeId);
        const VertexHandle firstVertex = node ? first_valid_vertex(node->mesh()) : VertexHandle{};

        const CommandResult undoResult = history.undo(dispatcher);
        print_result("undo edit mesh selection", undoResult);
        check(undoResult.success, "undo edit mesh selection executou com sucesso");
        check(editor.selection().mesh().vertices().empty(), "undo limpou selecao de vertices");

        node = find_mesh(editor, meshNodeId);
        if (node && firstVertex.is_valid()) {
            check(!node->mesh().vertex(firstVertex).selected, "undo restaurou flag selected do vertice");
        }

        const CommandResult redoResult = history.redo(dispatcher);
        print_result("redo edit mesh selection", redoResult);
        check(redoResult.success, "redo edit mesh selection executou com sucesso");
        check(editor.selection().mesh().vertices().size() == 1u, "redo restaurou selecao de vertices");

        node = find_mesh(editor, meshNodeId);
        if (node && firstVertex.is_valid()) {
            check(node->mesh().vertex(firstVertex).selected, "redo restaurou flag selected do vertice");
        }
    }

    std::cout << "\n=== Resultado Final ===\n";

    if (g_failures == 0) {
        std::cout << "Todos os testes passaram.\n";
        return 0;
    }

    std::cout << g_failures << " teste(s) falharam.\n";
    return 1;
}