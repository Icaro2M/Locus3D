/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/scene/EditorScene.h"
#include "editor/sync/PickingSync.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

    using locus::editor::EditorScene;
    using locus::editor::PickingSync;
    using locus::editor::PickingSyncResult;
    using locus::editor::SceneNodeId;

    using locus::graphics::PickingId;

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

    bool test_empty_scene() {
        std::cout << "\n=== Empty scene ===\n";

        bool ok = true;

        EditorScene scene;
        PickingSync sync;

        ok &= expect(
            sync.empty(),
            "PickingSync inicia vazio"
        );

        ok &= expect(
            sync.size() == 0,
            "PickingSync inicia com zero mapeamentos"
        );

        ok &= expect(
            sync.sync(scene),
            "sync de cena vazia e concluido"
        );

        const PickingSyncResult& result = sync.last_result();

        ok &= expect(
            result.synchronized,
            "resultado informa sincronizacao concluida"
        );

        ok &= expect(
            result.sceneNodeCount == 0,
            "cena vazia informa zero nodes"
        );

        ok &= expect(
            result.mappingCount == 0,
            "cena vazia produz zero mapeamentos"
        );

        ok &= expect(
            result.createdMappingCount == 0,
            "cena vazia nao cria mapeamentos"
        );

        ok &= expect(
            result.preservedMappingCount == 0,
            "cena vazia nao preserva mapeamentos"
        );

        ok &= expect(
            result.removedMappingCount == 0,
            "cena vazia nao remove mapeamentos"
        );

        ok &= expect(
            !result.exhausted,
            "cena vazia nao esgota IDs"
        );

        ok &= expect(
            !result.message.empty(),
            "sync de cena vazia produz diagnostico"
        );

        return ok;
    }

    bool test_initial_mapping() {
        std::cout << "\n=== Initial picking mappings ===\n";

        bool ok = true;

        EditorScene scene;

        const SceneNodeId nodeA =
            scene.create_empty("Node A");

        const SceneNodeId nodeB =
            scene.create_empty("Node B");

        const SceneNodeId nodeC =
            scene.create_empty("Node C");

        ok &= expect(
            nodeA.is_valid()
            && nodeB.is_valid()
            && nodeC.is_valid(),
            "nodes foram criados com IDs validos"
        );

        PickingSync sync;

        ok &= expect(
            sync.sync(scene),
            "primeiro sync foi concluido"
        );

        const PickingSyncResult& result = sync.last_result();

        ok &= expect(
            sync.size() == 3,
            "primeiro sync cria tres mapeamentos"
        );

        ok &= expect(
            result.sceneNodeCount == 3,
            "resultado informa tres nodes"
        );

        ok &= expect(
            result.createdMappingCount == 3,
            "resultado informa tres mapeamentos criados"
        );

        ok &= expect(
            result.preservedMappingCount == 0,
            "primeiro sync nao preserva mapeamentos anteriores"
        );

        ok &= expect(
            result.removedMappingCount == 0,
            "primeiro sync nao remove mapeamentos"
        );

        ok &= expect(
            result.mappingCount == 3,
            "resultado informa tres mapeamentos ativos"
        );

        const PickingId pickingA =
            sync.picking_id(nodeA);

        const PickingId pickingB =
            sync.picking_id(nodeB);

        const PickingId pickingC =
            sync.picking_id(nodeC);

        ok &= expect(
            pickingA.is_valid()
            && pickingB.is_valid()
            && pickingC.is_valid(),
            "todos os nodes recebem PickingId valido"
        );

        ok &= expect(
            pickingA != pickingB
            && pickingA != pickingC
            && pickingB != pickingC,
            "cada node recebe PickingId unico"
        );

        ok &= expect(
            sync.contains(nodeA)
            && sync.contains(nodeB)
            && sync.contains(nodeC),
            "contains reconhece todos os SceneNodeId"
        );

        ok &= expect(
            sync.contains(pickingA)
            && sync.contains(pickingB)
            && sync.contains(pickingC),
            "contains reconhece todos os PickingId"
        );

        ok &= expect(
            sync.scene_node_id(pickingA) == nodeA,
            "PickingId de A resolve para node A"
        );

        ok &= expect(
            sync.scene_node_id(pickingB) == nodeB,
            "PickingId de B resolve para node B"
        );

        ok &= expect(
            sync.scene_node_id(pickingC) == nodeC,
            "PickingId de C resolve para node C"
        );

        return ok;
    }

    bool test_mapping_preservation() {
        std::cout << "\n=== Mapping preservation ===\n";

        bool ok = true;

        EditorScene scene;

        const SceneNodeId nodeA =
            scene.create_empty("Node A");

        const SceneNodeId nodeB =
            scene.create_empty("Node B");

        const SceneNodeId nodeC =
            scene.create_empty("Node C");

        PickingSync sync;

        sync.sync(scene);

        const PickingId originalA =
            sync.picking_id(nodeA);

        const PickingId originalB =
            sync.picking_id(nodeB);

        const PickingId originalC =
            sync.picking_id(nodeC);

        ok &= expect(
            sync.sync(scene),
            "segundo sync sem alteracoes foi concluido"
        );

        const PickingSyncResult& result = sync.last_result();

        ok &= expect(
            result.createdMappingCount == 0,
            "segundo sync nao cria novos mapeamentos"
        );

        ok &= expect(
            result.preservedMappingCount == 3,
            "segundo sync preserva tres mapeamentos"
        );

        ok &= expect(
            result.removedMappingCount == 0,
            "segundo sync nao remove mapeamentos"
        );

        ok &= expect(
            sync.picking_id(nodeA) == originalA,
            "node A preserva PickingId"
        );

        ok &= expect(
            sync.picking_id(nodeB) == originalB,
            "node B preserva PickingId"
        );

        ok &= expect(
            sync.picking_id(nodeC) == originalC,
            "node C preserva PickingId"
        );

        return ok;
    }

    bool test_node_removal() {
        std::cout << "\n=== Node removal ===\n";

        bool ok = true;

        EditorScene scene;

        const SceneNodeId nodeA =
            scene.create_empty("Node A");

        const SceneNodeId nodeB =
            scene.create_empty("Node B");

        const SceneNodeId nodeC =
            scene.create_empty("Node C");

        PickingSync sync;
        sync.sync(scene);

        const PickingId pickingA =
            sync.picking_id(nodeA);

        const PickingId removedPickingId =
            sync.picking_id(nodeB);

        const PickingId pickingC =
            sync.picking_id(nodeC);

        ok &= expect(
            scene.remove_node(nodeB),
            "node B foi removido da cena"
        );

        ok &= expect(
            sync.sync(scene),
            "sync depois da remocao foi concluido"
        );

        const PickingSyncResult& result = sync.last_result();

        ok &= expect(
            sync.size() == 2,
            "remocao deixa dois mapeamentos ativos"
        );

        ok &= expect(
            result.sceneNodeCount == 2,
            "resultado informa dois nodes restantes"
        );

        ok &= expect(
            result.preservedMappingCount == 2,
            "mapeamentos de A e C foram preservados"
        );

        ok &= expect(
            result.createdMappingCount == 0,
            "remocao nao cria mapeamentos"
        );

        ok &= expect(
            result.removedMappingCount == 1,
            "resultado informa um mapeamento removido"
        );

        ok &= expect(
            !sync.contains(nodeB),
            "SceneNodeId removido nao permanece na tabela"
        );

        ok &= expect(
            !sync.contains(removedPickingId),
            "PickingId removido nao permanece na tabela reversa"
        );

        ok &= expect(
            !sync.picking_id(nodeB).is_valid(),
            "consulta do node removido retorna PickingId invalido"
        );

        ok &= expect(
            sync.scene_node_id(
                removedPickingId
            ).is_invalid(),
            "consulta do PickingId removido retorna SceneNodeId invalido"
        );

        ok &= expect(
            sync.picking_id(nodeA) == pickingA,
            "node A preserva ID depois da remocao"
        );

        ok &= expect(
            sync.picking_id(nodeC) == pickingC,
            "node C preserva ID depois da remocao"
        );

        return ok;
    }

    bool test_released_id_reuse() {
        std::cout << "\n=== Released ID reuse ===\n";

        bool ok = true;

        EditorScene scene;

        const SceneNodeId nodeA =
            scene.create_empty("Node A");

        const SceneNodeId nodeB =
            scene.create_empty("Node B");

        const SceneNodeId nodeC =
            scene.create_empty("Node C");

        PickingSync sync;
        sync.sync(scene);

        const PickingId removedPickingId =
            sync.picking_id(nodeB);

        scene.remove_node(nodeB);
        sync.sync(scene);

        const SceneNodeId nodeD =
            scene.create_empty("Node D");

        ok &= expect(
            nodeD.is_valid(),
            "novo node D possui ID valido"
        );

        ok &= expect(
            sync.sync(scene),
            "sync depois de criar node D foi concluido"
        );

        const PickingSyncResult& result = sync.last_result();

        const PickingId pickingD =
            sync.picking_id(nodeD);

        ok &= expect(
            pickingD.is_valid(),
            "node D recebe PickingId valido"
        );

        ok &= expect(
            pickingD == removedPickingId,
            "node D reutiliza PickingId liberado por B"
        );

        ok &= expect(
            result.createdMappingCount == 1,
            "sync cria somente o mapeamento de D"
        );

        ok &= expect(
            result.preservedMappingCount == 2,
            "sync preserva os mapeamentos de A e C"
        );

        ok &= expect(
            result.removedMappingCount == 0,
            "sync de D nao remove outro mapeamento"
        );

        ok &= expect(
            sync.scene_node_id(pickingD) == nodeD,
            "ID reutilizado agora resolve para node D"
        );

        ok &= expect(
            sync.contains(nodeA)
            && sync.contains(nodeC)
            && sync.contains(nodeD),
            "tres nodes atuais estao mapeados"
        );

        return ok;
    }

    bool test_scene_clear_synchronization() {
        std::cout << "\n=== Scene clear synchronization ===\n";

        bool ok = true;

        EditorScene scene;

        scene.create_empty("Node A");
        scene.create_empty("Node B");
        scene.create_empty("Node C");

        PickingSync sync;
        sync.sync(scene);

        ok &= expect(
            sync.size() == 3,
            "tres mapeamentos existem antes de scene.clear"
        );

        scene.clear();

        ok &= expect(
            scene.tree().empty(),
            "EditorScene fica vazia depois de clear"
        );

        ok &= expect(
            sync.sync(scene),
            "sync com cena limpa foi concluido"
        );

        const PickingSyncResult& result = sync.last_result();

        ok &= expect(
            sync.empty(),
            "sync remove todos os mapeamentos da cena limpa"
        );

        ok &= expect(
            result.sceneNodeCount == 0,
            "resultado informa zero nodes depois de clear"
        );

        ok &= expect(
            result.removedMappingCount == 3,
            "resultado informa tres mapeamentos removidos"
        );

        ok &= expect(
            result.mappingCount == 0,
            "resultado informa zero mapeamentos ativos"
        );

        return ok;
    }

    bool test_sync_clear() {
        std::cout << "\n=== PickingSync clear ===\n";

        bool ok = true;

        EditorScene scene;

        const SceneNodeId node =
            scene.create_empty("Node");

        PickingSync sync;
        sync.sync(scene);

        ok &= expect(
            !sync.empty(),
            "sync possui mapeamento antes de clear"
        );

        sync.clear();

        ok &= expect(
            sync.empty(),
            "clear remove todos os mapeamentos"
        );

        ok &= expect(
            sync.size() == 0,
            "clear zera quantidade de mapeamentos"
        );

        ok &= expect(
            !sync.contains(node),
            "clear remove consulta por SceneNodeId"
        );

        ok &= expect(
            sync.last_result().mappingCount == 0,
            "clear reinicia o ultimo resultado"
        );

        ok &= expect(
            sync.sync(scene),
            "sync pode ser usado novamente depois de clear"
        );

        const PickingId pickingId =
            sync.picking_id(node);

        ok &= expect(
            pickingId.is_valid(),
            "node recebe novo PickingId depois de clear"
        );

        ok &= expect(
            pickingId.value == 1,
            "clear reinicia alocacao no PickingId 1"
        );

        return ok;
    }

    bool test_invalid_queries() {
        std::cout << "\n=== Invalid queries ===\n";

        bool ok = true;

        EditorScene scene;
        PickingSync sync;

        const SceneNodeId node =
            scene.create_empty("Node");

        sync.sync(scene);

        const SceneNodeId invalidNode{};
        const PickingId invalidPicking =
            PickingId::invalid();

        const SceneNodeId missingNode{
            999999
        };

        const PickingId missingPicking =
            PickingId::from_u32(999999);

        ok &= expect(
            !sync.contains(invalidNode),
            "contains rejeita SceneNodeId invalido"
        );

        ok &= expect(
            !sync.contains(invalidPicking),
            "contains rejeita PickingId invalido"
        );

        ok &= expect(
            !sync.picking_id(
                invalidNode
            ).is_valid(),
            "SceneNodeId invalido resolve para PickingId invalido"
        );

        ok &= expect(
            sync.scene_node_id(
                invalidPicking
            ).is_invalid(),
            "PickingId invalido resolve para SceneNodeId invalido"
        );

        ok &= expect(
            !sync.contains(missingNode),
            "SceneNodeId valido mas ausente nao e encontrado"
        );

        ok &= expect(
            !sync.contains(missingPicking),
            "PickingId valido mas ausente nao e encontrado"
        );

        ok &= expect(
            !sync.picking_id(
                missingNode
            ).is_valid(),
            "node ausente resolve para PickingId invalido"
        );

        ok &= expect(
            sync.scene_node_id(
                missingPicking
            ).is_invalid(),
            "PickingId ausente resolve para SceneNodeId invalido"
        );

        ok &= expect(
            sync.contains(node),
            "consultas invalidas nao afetam mapeamento existente"
        );

        return ok;
    }

    bool test_bidirectional_consistency() {
        std::cout << "\n=== Bidirectional consistency ===\n";

        bool ok = true;

        EditorScene scene;

        const SceneNodeId nodeA =
            scene.create_empty("Node A");

        const SceneNodeId nodeB =
            scene.create_empty("Node B");

        const SceneNodeId nodeC =
            scene.create_empty("Node C");

        PickingSync sync;
        sync.sync(scene);

        const SceneNodeId nodes[] = {
            nodeA,
            nodeB,
            nodeC
        };

        for (const SceneNodeId nodeId : nodes) {
            const PickingId pickingId =
                sync.picking_id(nodeId);

            ok &= expect(
                pickingId.is_valid(),
                "ida produz PickingId valido"
            );

            ok &= expect(
                sync.scene_node_id(pickingId) == nodeId,
                "ida e volta preservam SceneNodeId"
            );
        }

        return ok;
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor PickingSync Smoke Test ===\n";

    bool ok = true;

    ok &= test_empty_scene();
    ok &= test_initial_mapping();
    ok &= test_mapping_preservation();
    ok &= test_node_removal();
    ok &= test_released_id_reuse();
    ok &= test_scene_clear_synchronization();
    ok &= test_sync_clear();
    ok &= test_invalid_queries();
    ok &= test_bidirectional_consistency();

    std::cout << "\n=== Resultado final ===\n";

    if (!ok) {
        std::cout
            << "[FAIL] Um ou mais testes do PickingSync falharam.\n";

        return EXIT_FAILURE;
    }

    std::cout
        << "[OK] Todos os testes do PickingSync passaram.\n";

    return EXIT_SUCCESS;
}