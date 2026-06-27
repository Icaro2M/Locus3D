#include "editor/Editor.h"
#include "editor/scene/MeshNode.h"
#include "kernel/geometry/mesh/LEMEditor.h"

#include <glm/vec3.hpp>

#include <iostream>

int main()
{
    using namespace locus::editor;
    using namespace locus::kernel::geometry;

    std::cout << "=== Locus3D Editor Scene Smoke Test ===\n\n";

    Editor editor;

    const SceneNodeId root = editor.scene().create_empty("Root");
    const SceneNodeId meshId = editor.scene().create_mesh("Quad Mesh");

    std::cout << "[INFO] root id: " << root.value << "\n";
    std::cout << "[INFO] mesh id: " << meshId.value << "\n";

    if (!root.is_valid() || !meshId.is_valid()) {
        std::cout << "[FAIL] ids validos\n";
        return 1;
    }

    if (!editor.scene().reparent(meshId, root)) {
        std::cout << "[FAIL] reparent mesh -> root\n";
        return 1;
    }

    const SceneNode* rootNode = editor.scene().find_node(root);
    const SceneNode* meshNodeBase = editor.scene().find_node(meshId);
    MeshNode* meshNode = editor.scene().find_mesh(meshId);

    if (!rootNode || !meshNodeBase || !meshNode) {
        std::cout << "[FAIL] busca de nodes\n";
        return 1;
    }

    if (rootNode->children().size() != 1 || rootNode->children()[0] != meshId) {
        std::cout << "[FAIL] hierarquia root -> mesh\n";
        return 1;
    }

    if (meshNodeBase->parent() != root) {
        std::cout << "[FAIL] parent do mesh\n";
        return 1;
    }

    meshNode->transform().set_position(glm::vec3{ 1.0f, 2.0f, 3.0f });
    meshNode->transform().set_scale(glm::vec3{ 2.0f, 2.0f, 2.0f });

    LEMEditor meshEditor(meshNode->mesh());

    const VertexHandle v0 = meshEditor.add_vertex(glm::vec3{ -1.0f, -1.0f, 0.0f });
    const VertexHandle v1 = meshEditor.add_vertex(glm::vec3{ 1.0f, -1.0f, 0.0f });
    const VertexHandle v2 = meshEditor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });
    const VertexHandle v3 = meshEditor.add_vertex(glm::vec3{ -1.0f, 1.0f, 0.0f });

    const FaceHandle face = meshEditor.add_face({ v0, v1, v2, v3 });

    if (!face.is_valid()) {
        std::cout << "[FAIL] face quad criada\n";
        return 1;
    }

    if (meshNode->mesh().vertex_count() != 4 ||
        meshNode->mesh().edge_count() != 4 ||
        meshNode->mesh().loop_count() != 4 ||
        meshNode->mesh().face_count() != 1) {
        std::cout << "[FAIL] contagem da LEM\n";
        return 1;
    }

    if (!editor.scene().tree().is_ancestor(root, meshId)) {
        std::cout << "[FAIL] root deveria ser ancestral do mesh\n";
        return 1;
    }

    if (editor.scene().tree().is_ancestor(meshId, root)) {
        std::cout << "[FAIL] mesh nao deveria ser ancestral do root\n";
        return 1;
    }

    std::cout << "[OK] ids criados\n";
    std::cout << "[OK] hierarquia criada\n";
    std::cout << "[OK] transform local aplicado\n";
    std::cout << "[OK] MeshNode contem LEM editavel\n";
    std::cout << "[OK] quad criado via LEMEditor\n";
    std::cout << "[OK] vertices: " << meshNode->mesh().vertex_count() << "\n";
    std::cout << "[OK] edges: " << meshNode->mesh().edge_count() << "\n";
    std::cout << "[OK] loops: " << meshNode->mesh().loop_count() << "\n";
    std::cout << "[OK] faces: " << meshNode->mesh().face_count() << "\n";

    std::cout << "\n=== Editor Scene Smoke Test PASSED ===\n";
    return 0;
}