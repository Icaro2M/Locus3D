/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/PickingRenderAdapter.h"
#include "editor/scene/EditorScene.h"
#include "editor/sync/PickingSync.h"
#include "graphics/picking/PickingId.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

    using locus::editor::EditorScene;
    using locus::editor::PickingRenderAdapter;
    using locus::editor::PickingRenderResult;
    using locus::editor::PickingSync;
    using locus::editor::SceneNodeId;

    using locus::graphics::PickingId;
    using locus::graphics::RenderObject;
    using locus::graphics::RenderScene;

    bool expect(
        const bool condition,
        const std::string& message
    ) {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return true;
        }

        std::cout << "[FAIL] " << message << '\n';
        return false;
    }

    RenderObject make_render_object(
        const SceneNodeId nodeId,
        const std::string& name
    ) {
        RenderObject object{};
        object.id = static_cast<RenderObject::Id>(nodeId.value);
        object.name = name;
        return object;
    }

    RenderObject* find_render_object(
        RenderScene& scene,
        const RenderObject::Id id
    ) {
        for (RenderObject& object : scene.objects()) {
            if (object.id == id) {
                return &object;
            }
        }

        return nullptr;
    }

    const RenderObject* find_render_object(
        const RenderScene& scene,
        const RenderObject::Id id
    ) {
        for (const RenderObject& object : scene.objects()) {
            if (object.id == id) {
                return &object;
            }
        }

        return nullptr;
    }

    bool test_apply_to_object() {
        std::cout << "\n=== Apply to one object ===\n";

        bool ok = true;

        EditorScene editorScene;

        const SceneNodeId nodeId =
            editorScene.create_empty("Mapped Node");

        PickingSync sync;

        ok &= expect(
            sync.sync(editorScene),
            "PickingSync foi sincronizado"
        );

        RenderObject object =
            make_render_object(nodeId, "Mapped Object");

        ok &= expect(
            !object.pickingId.is_valid(),
            "RenderObject inicia com PickingId invalido"
        );

        ok &= expect(
            PickingRenderAdapter::apply_to_object(
                object,
                sync
            ),
            "adapter aplica mapeamento ao objeto"
        );

        const PickingId expectedPickingId =
            sync.picking_id(nodeId);

        ok &= expect(
            object.pickingId.is_valid(),
            "objeto recebe PickingId valido"
        );

        ok &= expect(
            object.pickingId == expectedPickingId,
            "objeto recebe o PickingId mantido pelo sync"
        );

        ok &= expect(
            object.id
            == static_cast<RenderObject::Id>(
                nodeId.value
                ),
            "adapter preserva o ID estavel do RenderObject"
        );

        return ok;
    }

    bool test_invalid_object_id() {
        std::cout << "\n=== Invalid render object ID ===\n";

        bool ok = true;

        EditorScene editorScene;
        editorScene.create_empty("Node");

        PickingSync sync;
        sync.sync(editorScene);

        RenderObject object{};
        object.id = 0;
        object.pickingId = PickingId::from_u32(77);

        ok &= expect(
            !PickingRenderAdapter::apply_to_object(
                object,
                sync
            ),
            "objeto com ID zero nao recebe mapeamento"
        );

        ok &= expect(
            !object.pickingId.is_valid(),
            "PickingId anterior e limpo para objeto invalido"
        );

        ok &= expect(
            object.id == 0,
            "ID zero do objeto permanece inalterado"
        );

        return ok;
    }

    bool test_unmapped_object() {
        std::cout << "\n=== Unmapped render object ===\n";

        bool ok = true;

        EditorScene editorScene;
        editorScene.create_empty("Mapped Node");

        PickingSync sync;
        sync.sync(editorScene);

        const SceneNodeId missingNodeId{ 999999 };

        RenderObject object =
            make_render_object(
                missingNodeId,
                "Unmapped Object"
            );

        object.pickingId = PickingId::from_u32(88);

        ok &= expect(
            !PickingRenderAdapter::apply_to_object(
                object,
                sync
            ),
            "objeto sem entrada no sync nao recebe mapeamento"
        );

        ok &= expect(
            !object.pickingId.is_valid(),
            "PickingId obsoleto do objeto sem mapeamento e limpo"
        );

        return ok;
    }

    bool test_apply_to_scene() {
        std::cout << "\n=== Apply to render scene ===\n";

        bool ok = true;

        EditorScene editorScene;

        const SceneNodeId nodeA =
            editorScene.create_empty("Node A");

        const SceneNodeId nodeB =
            editorScene.create_empty("Node B");

        const SceneNodeId nodeC =
            editorScene.create_empty("Node C");

        PickingSync sync;

        ok &= expect(
            sync.sync(editorScene),
            "sync cria os mapeamentos da cena"
        );

        RenderScene renderScene;

        renderScene.add_object(
            make_render_object(nodeA, "Object A")
        );

        renderScene.add_object(
            make_render_object(nodeB, "Object B")
        );

        renderScene.add_object(
            make_render_object(nodeC, "Object C")
        );

        RenderObject invalidObject{};
        invalidObject.id = 0;
        invalidObject.name = "Invalid Object";
        invalidObject.pickingId =
            PickingId::from_u32(100);

        renderScene.add_object(
            std::move(invalidObject)
        );

        RenderObject unmappedObject =
            make_render_object(
                SceneNodeId{ 999999 },
                "Unmapped Object"
            );

        unmappedObject.pickingId =
            PickingId::from_u32(101);

        renderScene.add_object(
            std::move(unmappedObject)
        );

        PickingRenderResult result;

        PickingRenderAdapter::apply_to_scene(
            renderScene,
            sync,
            &result
        );

        ok &= expect(
            renderScene.object_count() == 5,
            "RenderScene preserva os cinco objetos"
        );

        ok &= expect(
            result.visitedObjectCount == 5,
            "resultado informa cinco objetos visitados"
        );

        ok &= expect(
            result.assignedObjectCount == 3,
            "resultado informa tres objetos mapeados"
        );

        ok &= expect(
            result.invalidObjectCount == 1,
            "resultado informa um objeto com ID zero"
        );

        ok &= expect(
            result.unmappedObjectCount == 1,
            "resultado informa um objeto sem mapeamento"
        );

        ok &= expect(
            result.has_assignments(),
            "resultado informa que houve atribuicoes"
        );

        ok &= expect(
            !result.message.empty(),
            "resultado produz mensagem de diagnostico"
        );

        const RenderObject* objectA =
            find_render_object(
                renderScene,
                static_cast<RenderObject::Id>(
                    nodeA.value
                    )
            );

        const RenderObject* objectB =
            find_render_object(
                renderScene,
                static_cast<RenderObject::Id>(
                    nodeB.value
                    )
            );

        const RenderObject* objectC =
            find_render_object(
                renderScene,
                static_cast<RenderObject::Id>(
                    nodeC.value
                    )
            );

        ok &= expect(
            objectA != nullptr
            && objectB != nullptr
            && objectC != nullptr,
            "objetos mapeados continuam na RenderScene"
        );

        if (objectA && objectB && objectC) {
            ok &= expect(
                objectA->pickingId
                == sync.picking_id(nodeA),
                "objeto A recebe PickingId correto"
            );

            ok &= expect(
                objectB->pickingId
                == sync.picking_id(nodeB),
                "objeto B recebe PickingId correto"
            );

            ok &= expect(
                objectC->pickingId
                == sync.picking_id(nodeC),
                "objeto C recebe PickingId correto"
            );

            ok &= expect(
                objectA->pickingId
                != objectB->pickingId
                && objectA->pickingId
                != objectC->pickingId
                && objectB->pickingId
                != objectC->pickingId,
                "objetos recebem PickingId distintos"
            );
        }

        const RenderObject* invalid =
            find_render_object(renderScene, 0);

        const RenderObject* unmapped =
            find_render_object(renderScene, 999999);

        ok &= expect(
            invalid != nullptr
            && !invalid->pickingId.is_valid(),
            "objeto com ID zero termina sem PickingId"
        );

        ok &= expect(
            unmapped != nullptr
            && !unmapped->pickingId.is_valid(),
            "objeto sem mapeamento termina sem PickingId"
        );

        return ok;
    }

    bool test_selectability_does_not_change_identity() {
        std::cout << "\n=== Selectability and identity ===\n";

        bool ok = true;

        EditorScene editorScene;

        const SceneNodeId nodeId =
            editorScene.create_empty("Node");

        PickingSync sync;
        sync.sync(editorScene);

        RenderObject object =
            make_render_object(nodeId, "Object");

        object.visibility.selectable = false;

        ok &= expect(
            PickingRenderAdapter::apply_to_object(
                object,
                sync
            ),
            "objeto nao selecionavel ainda recebe identidade de picking"
        );

        ok &= expect(
            object.pickingId
            == sync.picking_id(nodeId),
            "selectable nao altera o mapeamento de identidade"
        );

        ok &= expect(
            !object.visibility.selectable,
            "adapter nao altera a flag selectable"
        );

        return ok;
    }

    bool test_resynchronization_after_node_removal() {
        std::cout << "\n=== Resynchronization after removal ===\n";

        bool ok = true;

        EditorScene editorScene;

        const SceneNodeId nodeA =
            editorScene.create_empty("Node A");

        const SceneNodeId nodeB =
            editorScene.create_empty("Node B");

        PickingSync sync;
        sync.sync(editorScene);

        RenderScene renderScene;

        renderScene.add_object(
            make_render_object(nodeA, "Object A")
        );

        renderScene.add_object(
            make_render_object(nodeB, "Object B")
        );

        PickingRenderAdapter::apply_to_scene(
            renderScene,
            sync
        );

        RenderObject* objectA =
            find_render_object(
                renderScene,
                static_cast<RenderObject::Id>(
                    nodeA.value
                    )
            );

        RenderObject* objectB =
            find_render_object(
                renderScene,
                static_cast<RenderObject::Id>(
                    nodeB.value
                    )
            );

        const PickingId originalA =
            objectA
            ? objectA->pickingId
            : PickingId::invalid();

        const PickingId originalB =
            objectB
            ? objectB->pickingId
            : PickingId::invalid();

        ok &= expect(
            originalA.is_valid()
            && originalB.is_valid(),
            "ambos os objetos recebem IDs antes da remocao"
        );

        ok &= expect(
            editorScene.remove_node(nodeB),
            "node B e removido da EditorScene"
        );

        ok &= expect(
            sync.sync(editorScene),
            "PickingSync e atualizado depois da remocao"
        );

        PickingRenderResult result;

        PickingRenderAdapter::apply_to_scene(
            renderScene,
            sync,
            &result
        );

        objectA = find_render_object(
            renderScene,
            static_cast<RenderObject::Id>(
                nodeA.value
                )
        );

        objectB = find_render_object(
            renderScene,
            static_cast<RenderObject::Id>(
                nodeB.value
                )
        );

        ok &= expect(
            objectA != nullptr
            && objectA->pickingId == originalA,
            "objeto A preserva seu PickingId"
        );

        ok &= expect(
            objectB != nullptr
            && !objectB->pickingId.is_valid(),
            "objeto B perde PickingId apos sair da EditorScene"
        );

        ok &= expect(
            result.assignedObjectCount == 1,
            "somente objeto A permanece mapeado"
        );

        ok &= expect(
            result.unmappedObjectCount == 1,
            "objeto B e contabilizado como nao mapeado"
        );

        return ok;
    }

    bool test_empty_render_scene() {
        std::cout << "\n=== Empty render scene ===\n";

        bool ok = true;

        EditorScene editorScene;
        editorScene.create_empty("Node");

        PickingSync sync;
        sync.sync(editorScene);

        RenderScene renderScene;
        PickingRenderResult result;

        PickingRenderAdapter::apply_to_scene(
            renderScene,
            sync,
            &result
        );

        ok &= expect(
            renderScene.empty(),
            "RenderScene permanece vazia"
        );

        ok &= expect(
            result.visitedObjectCount == 0,
            "cena vazia visita zero objetos"
        );

        ok &= expect(
            result.assignedObjectCount == 0,
            "cena vazia atribui zero IDs"
        );

        ok &= expect(
            result.invalidObjectCount == 0,
            "cena vazia nao encontra objetos invalidos"
        );

        ok &= expect(
            result.unmappedObjectCount == 0,
            "cena vazia nao encontra objetos sem mapeamento"
        );

        ok &= expect(
            !result.has_assignments(),
            "cena vazia nao informa atribuicoes"
        );

        ok &= expect(
            !result.message.empty(),
            "cena vazia produz diagnostico"
        );

        return ok;
    }

    bool test_null_result() {
        std::cout << "\n=== Null diagnostic result ===\n";

        bool ok = true;

        EditorScene editorScene;

        const SceneNodeId nodeId =
            editorScene.create_empty("Node");

        PickingSync sync;
        sync.sync(editorScene);

        RenderScene renderScene;

        renderScene.add_object(
            make_render_object(nodeId, "Object")
        );

        PickingRenderAdapter::apply_to_scene(
            renderScene,
            sync,
            nullptr
        );

        const RenderObject* object =
            find_render_object(
                renderScene,
                static_cast<RenderObject::Id>(
                    nodeId.value
                    )
            );

        ok &= expect(
            object != nullptr
            && object->pickingId
            == sync.picking_id(nodeId),
            "adapter funciona sem resultado de diagnostico"
        );

        return ok;
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor PickingRenderAdapter "
        "Smoke Test ===\n";

    bool ok = true;

    ok &= test_apply_to_object();
    ok &= test_invalid_object_id();
    ok &= test_unmapped_object();
    ok &= test_apply_to_scene();
    ok &= test_selectability_does_not_change_identity();
    ok &= test_resynchronization_after_node_removal();
    ok &= test_empty_render_scene();
    ok &= test_null_result();

    std::cout << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes do "
            "PickingRenderAdapter falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes do "
        "PickingRenderAdapter passaram.\n";

    return EXIT_SUCCESS;
}