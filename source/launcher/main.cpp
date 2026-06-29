#include "editor/Editor.h"
#include "editor/selection/SelectionSerializer.h"
#include "kernel/geometry/mesh/LEMEditor.h"

#include <glm/vec3.hpp>

#include <iostream>

int main()
{
    using namespace locus::editor;
    using namespace locus::kernel::geometry;

    std::cout << "=== Locus3D Editor Selection Smoke Test ===\n\n";

    Editor editor;

    const SceneNodeId root = editor.scene().create_empty("Root");
    const SceneNodeId meshId = editor.scene().create_mesh("Quad Mesh");
    const SceneNodeId emptyId = editor.scene().create_empty("Empty Helper");

    if (!root.is_valid() || !meshId.is_valid() || !emptyId.is_valid()) {
        std::cout << "[FAIL] ids validos\n";
        return 1;
    }

    if (!editor.scene().reparent(meshId, root)) {
        std::cout << "[FAIL] reparent mesh -> root\n";
        return 1;
    }

    MeshNode* meshNode = editor.scene().find_mesh(meshId);
    if (!meshNode) {
        std::cout << "[FAIL] mesh node encontrado\n";
        return 1;
    }

    LEMEditor meshEditor(meshNode->mesh());

    const VertexHandle v0 = meshEditor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
    const VertexHandle v1 = meshEditor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
    const VertexHandle v2 = meshEditor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });
    const VertexHandle v3 = meshEditor.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });

    const FaceHandle face = meshEditor.add_face({ v0, v1, v2, v3 });

    if (!face.is_valid()) {
        std::cout << "[FAIL] face criada\n";
        return 1;
    }

    SelectionController& selection = editor.selection_controller();

    if (!selection.select_object(meshId)) {
        std::cout << "[FAIL] select object mesh\n";
        return 1;
    }

    if (editor.selection().objects().active() != meshId) {
        std::cout << "[FAIL] active object mesh\n";
        return 1;
    }

    if (!selection.add_object(emptyId)) {
        std::cout << "[FAIL] add object empty\n";
        return 1;
    }

    if (editor.selection().objects().size() != 2) {
        std::cout << "[FAIL] object selection size 2\n";
        return 1;
    }

    const bool emptySelectedAfterToggle = selection.toggle_object(emptyId);
    if (emptySelectedAfterToggle) {
        std::cout << "[FAIL] empty deveria ser removido no toggle\n";
        return 1;
    }

    if (editor.selection().objects().contains(emptyId)) {
        std::cout << "[FAIL] empty removido da selecao\n";
        return 1;
    }

    const bool emptySelectedAfterSecondToggle = selection.toggle_object(emptyId);
    if (!emptySelectedAfterSecondToggle) {
        std::cout << "[FAIL] empty deveria ser selecionado no segundo toggle\n";
        return 1;
    }

    if (!editor.selection().objects().contains(emptyId)) {
        std::cout << "[FAIL] empty selecionado novamente\n";
        return 1;
    }

    if (!selection.set_active_mesh(meshId)) {
        std::cout << "[FAIL] set active mesh\n";
        return 1;
    }

    if (!selection.select_vertex(v0)) {
        std::cout << "[FAIL] select vertex v0\n";
        return 1;
    }

    if (!selection.toggle_vertex(v1)) {
        std::cout << "[FAIL] toggle vertex v1\n";
        return 1;
    }

    if (editor.selection().mesh().vertices().size() != 2) {
        std::cout << "[FAIL] vertex selection size 2\n";
        return 1;
    }

    if (!selection.select_face(face)) {
        std::cout << "[FAIL] select face\n";
        return 1;
    }

    if (editor.selection().granularity() != SelectionGranularity::Face) {
        std::cout << "[FAIL] granularity face\n";
        return 1;
    }

    if (editor.selection().scope() != SelectionScope::ActiveMesh) {
        std::cout << "[FAIL] scope active mesh\n";
        return 1;
    }

    const SelectionSnapshot snapshot =
        SelectionSerializer::capture(editor.selection());

    editor.selection().clear();

    if (!editor.selection().objects().empty() ||
        !editor.selection().mesh().empty()) {
        std::cout << "[FAIL] selection clear\n";
        return 1;
    }

    SelectionSerializer::restore(snapshot, editor.selection());

    if (editor.selection().mesh().faces().size() != 1) {
        std::cout << "[FAIL] snapshot restore face\n";
        return 1;
    }

    if (editor.selection().mesh().active_mesh() != meshId) {
        std::cout << "[FAIL] snapshot restore active mesh\n";
        return 1;
    }

    if (!editor.selection().objects().contains(emptyId)) {
        std::cout << "[FAIL] snapshot restore object empty\n";
        return 1;
    }

    std::cout << "[OK] object selection\n";
    std::cout << "[OK] active object\n";
    std::cout << "[OK] object toggle remove/add\n";
    std::cout << "[OK] active mesh\n";
    std::cout << "[OK] vertex selection\n";
    std::cout << "[OK] face selection\n";
    std::cout << "[OK] selection granularity/scope\n";
    std::cout << "[OK] selection snapshot restore\n";

    std::cout << "\n=== Editor Selection Smoke Test PASSED ===\n";
    return 0;
}