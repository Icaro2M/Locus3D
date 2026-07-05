/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/SelectionRenderAdapter.h"
#include "editor/selection/SelectionState.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

    bool expect(bool condition, const std::string& message)
    {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return true;
        }

        std::cout << "[FAIL] " << message << '\n';
        return false;
    }

    bool expect_size(
        std::size_t actual,
        std::size_t expected,
        const std::string& message)
    {
        if (actual == expected) {
            std::cout << "[OK] " << message << " = " << actual << '\n';
            return true;
        }

        std::cout
            << "[FAIL] " << message
            << " | actual=" << actual
            << " expected=" << expected
            << '\n';

        return false;
    }

    bool expect_u64(
        std::uint64_t actual,
        std::uint64_t expected,
        const std::string& message)
    {
        if (actual == expected) {
            std::cout << "[OK] " << message << " = " << actual << '\n';
            return true;
        }

        std::cout
            << "[FAIL] " << message
            << " | actual=" << actual
            << " expected=" << expected
            << '\n';

        return false;
    }

    locus::graphics::RenderObject make_object(
        locus::graphics::RenderObject::Id id,
        const std::string& name)
    {
        locus::graphics::RenderObject object{};
        object.id = id;
        object.name = name;
        object.visibility.visible = true;
        object.visibility.selectable = true;
        return object;
    }

    locus::graphics::RenderScene make_three_object_scene()
    {
        locus::graphics::RenderScene scene;
        scene.add_object(make_object(10, "Object A"));
        scene.add_object(make_object(20, "Object B"));
        scene.add_object(make_object(30, "Object C"));
        return scene;
    }

    void print_result(const locus::editor::SelectionRenderResult& result)
    {
        std::cout
            << "SelectionRenderResult"
            << " | visited: " << result.visitedObjectCount
            << " | selected: " << result.selectedObjectCount
            << " | hovered: " << result.hoveredObjectCount
            << " | changed: " << result.changedObjectCount
            << " | activeObjectApplied: " << (result.activeObjectApplied ? "true" : "false")
            << " | hoveredObjectApplied: " << (result.hoveredObjectApplied ? "true" : "false")
            << " | activeMeshApplied: " << (result.activeMeshApplied ? "true" : "false")
            << " | activeMeshHoverApplied: " << (result.activeMeshHoverApplied ? "true" : "false")
            << '\n';

        for (const locus::editor::SelectionRenderObjectResult& objectResult : result.objects) {
            std::cout
                << "  object " << objectResult.objectId
                << " | wasSelected: " << (objectResult.wasSelected ? "true" : "false")
                << " | wasHovered: " << (objectResult.wasHovered ? "true" : "false")
                << " | selected: " << (objectResult.selected ? "true" : "false")
                << " | hovered: " << (objectResult.hovered ? "true" : "false")
                << " | changed: " << (objectResult.changed ? "true" : "false");

            if (!objectResult.message.empty()) {
                std::cout << " | " << objectResult.message;
            }

            std::cout << '\n';
        }
    }

    bool test_empty_scene()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: cena vazia ===\n";

        bool ok = true;

        graphics::RenderScene scene;
        editor::SelectionState selection;

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, {}, &result);

        print_result(result);

        ok &= expect(output.empty(), "output vazio");
        ok &= expect_size(output.object_count(), 0, "object_count");
        ok &= expect_size(result.visitedObjectCount, 0, "visitedObjectCount");
        ok &= expect_size(result.selectedObjectCount, 0, "selectedObjectCount");
        ok &= expect_size(result.hoveredObjectCount, 0, "hoveredObjectCount");
        ok &= expect_size(result.changedObjectCount, 0, "changedObjectCount");
        ok &= expect_size(result.objects.size(), 0, "objects result count");

        return ok;
    }

    bool test_no_selection_clears_existing_flags()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: sem selecao limpa flags antigas ===\n";

        bool ok = true;

        graphics::RenderScene scene;

        graphics::RenderObject objectA = make_object(10, "Object A");
        objectA.selected = true;
        objectA.hovered = true;

        scene.add_object(objectA);
        scene.add_object(make_object(20, "Object B"));

        editor::SelectionState selection;

        editor::SelectionRenderOptions options{};
        options.clearExistingFlags = true;

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, options, &result);

        print_result(result);

        ok &= expect_size(output.object_count(), 2, "object_count");
        ok &= expect(!output.objects()[0].selected, "object A selected limpo");
        ok &= expect(!output.objects()[0].hovered, "object A hovered limpo");
        ok &= expect(!output.objects()[1].selected, "object B selected false");
        ok &= expect(!output.objects()[1].hovered, "object B hovered false");

        ok &= expect_size(result.visitedObjectCount, 2, "visitedObjectCount");
        ok &= expect_size(result.selectedObjectCount, 0, "selectedObjectCount");
        ok &= expect_size(result.hoveredObjectCount, 0, "hoveredObjectCount");
        ok &= expect_size(result.changedObjectCount, 1, "changedObjectCount");
        ok &= expect_size(result.objects.size(), 2, "objects result count");
        ok &= expect(result.objects[0].wasSelected, "object A wasSelected true");
        ok &= expect(result.objects[0].wasHovered, "object A wasHovered true");
        ok &= expect(result.objects[0].changed, "object A changed true");

        return ok;
    }

    bool test_object_selection()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: selecao de objetos ===\n";

        bool ok = true;

        graphics::RenderScene scene = make_three_object_scene();

        editor::SelectionState selection;
        selection.objects().add(editor::SceneNodeId{ 10 });
        selection.objects().add(editor::SceneNodeId{ 30 });
        selection.objects().set_active(editor::SceneNodeId{ 30 });

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, {}, &result);

        print_result(result);

        ok &= expect_size(output.object_count(), 3, "object_count");

        ok &= expect(output.objects()[0].selected, "object 10 selected");
        ok &= expect(!output.objects()[0].hovered, "object 10 hovered false");

        ok &= expect(!output.objects()[1].selected, "object 20 selected false");
        ok &= expect(!output.objects()[1].hovered, "object 20 hovered false");

        ok &= expect(output.objects()[2].selected, "object 30 selected");
        ok &= expect(!output.objects()[2].hovered, "object 30 hovered false");

        ok &= expect_size(result.visitedObjectCount, 3, "visitedObjectCount");
        ok &= expect_size(result.selectedObjectCount, 2, "selectedObjectCount");
        ok &= expect_size(result.hoveredObjectCount, 0, "hoveredObjectCount");
        ok &= expect_size(result.changedObjectCount, 2, "changedObjectCount");
        ok &= expect(result.activeObjectApplied, "activeObjectApplied true");
        ok &= expect(!result.hoveredObjectApplied, "hoveredObjectApplied false");

        return ok;
    }

    bool test_active_object_selected_even_if_not_in_selected_set()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: active object selecionado mesmo fora do set ===\n";

        bool ok = true;

        graphics::RenderScene scene = make_three_object_scene();

        editor::SelectionState selection;
        selection.objects().set_active(editor::SceneNodeId{ 20 });

        editor::SelectionRenderOptions options{};
        options.applyObjectSelection = true;
        options.applyActiveObject = true;

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, options, &result);

        print_result(result);

        ok &= expect(!output.objects()[0].selected, "object 10 selected false");
        ok &= expect(output.objects()[1].selected, "object 20 selected por active");
        ok &= expect(!output.objects()[2].selected, "object 30 selected false");

        ok &= expect_size(result.selectedObjectCount, 1, "selectedObjectCount");
        ok &= expect_size(result.changedObjectCount, 1, "changedObjectCount");
        ok &= expect(result.activeObjectApplied, "activeObjectApplied true");

        return ok;
    }

    bool test_disable_active_object_selection()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: active object desativado por option ===\n";

        bool ok = true;

        graphics::RenderScene scene = make_three_object_scene();

        editor::SelectionState selection;
        selection.objects().set_active(editor::SceneNodeId{ 20 });

        editor::SelectionRenderOptions options{};
        options.applyObjectSelection = false;
        options.applyActiveObject = false;

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, options, &result);

        print_result(result);

        ok &= expect(!output.objects()[0].selected, "object 10 selected false");
        ok &= expect(!output.objects()[1].selected, "object 20 selected false");
        ok &= expect(!output.objects()[2].selected, "object 30 selected false");

        ok &= expect_size(result.selectedObjectCount, 0, "selectedObjectCount");
        ok &= expect_size(result.changedObjectCount, 0, "changedObjectCount");
        ok &= expect(!result.activeObjectApplied, "activeObjectApplied false");

        return ok;
    }

    bool test_hovered_object()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: objeto hovered ===\n";

        bool ok = true;

        graphics::RenderScene scene = make_three_object_scene();

        editor::SelectionState selection;
        selection.objects().set_hovered(editor::SceneNodeId{ 20 });

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, {}, &result);

        print_result(result);

        ok &= expect(!output.objects()[0].hovered, "object 10 hovered false");
        ok &= expect(output.objects()[1].hovered, "object 20 hovered true");
        ok &= expect(!output.objects()[2].hovered, "object 30 hovered false");

        ok &= expect_size(result.selectedObjectCount, 0, "selectedObjectCount");
        ok &= expect_size(result.hoveredObjectCount, 1, "hoveredObjectCount");
        ok &= expect_size(result.changedObjectCount, 1, "changedObjectCount");
        ok &= expect(result.hoveredObjectApplied, "hoveredObjectApplied true");

        return ok;
    }

    bool test_selected_and_hovered_same_object()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: mesmo objeto selected e hovered ===\n";

        bool ok = true;

        graphics::RenderScene scene = make_three_object_scene();

        editor::SelectionState selection;
        selection.objects().set(editor::SceneNodeId{ 20 });
        selection.objects().set_hovered(editor::SceneNodeId{ 20 });

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, {}, &result);

        print_result(result);

        ok &= expect(!output.objects()[0].selected, "object 10 selected false");
        ok &= expect(!output.objects()[0].hovered, "object 10 hovered false");

        ok &= expect(output.objects()[1].selected, "object 20 selected true");
        ok &= expect(output.objects()[1].hovered, "object 20 hovered true");

        ok &= expect(!output.objects()[2].selected, "object 30 selected false");
        ok &= expect(!output.objects()[2].hovered, "object 30 hovered false");

        ok &= expect_size(result.selectedObjectCount, 1, "selectedObjectCount");
        ok &= expect_size(result.hoveredObjectCount, 1, "hoveredObjectCount");
        ok &= expect_size(result.changedObjectCount, 1, "changedObjectCount");
        ok &= expect(result.activeObjectApplied, "activeObjectApplied true");
        ok &= expect(result.hoveredObjectApplied, "hoveredObjectApplied true");

        return ok;
    }

    bool test_keep_existing_flags_when_clear_disabled()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: preservar flags antigas ===\n";

        bool ok = true;

        graphics::RenderScene scene;

        graphics::RenderObject objectA = make_object(10, "Object A");
        objectA.selected = true;

        graphics::RenderObject objectB = make_object(20, "Object B");
        objectB.hovered = true;

        scene.add_object(objectA);
        scene.add_object(objectB);
        scene.add_object(make_object(30, "Object C"));

        editor::SelectionState selection;
        selection.objects().set(editor::SceneNodeId{ 30 });

        editor::SelectionRenderOptions options{};
        options.clearExistingFlags = false;

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, options, &result);

        print_result(result);

        ok &= expect(output.objects()[0].selected, "object 10 selected preservado");
        ok &= expect(!output.objects()[0].hovered, "object 10 hovered false");

        ok &= expect(!output.objects()[1].selected, "object 20 selected false");
        ok &= expect(output.objects()[1].hovered, "object 20 hovered preservado");

        ok &= expect(output.objects()[2].selected, "object 30 selected aplicado");
        ok &= expect(!output.objects()[2].hovered, "object 30 hovered false");

        ok &= expect_size(result.selectedObjectCount, 2, "selectedObjectCount");
        ok &= expect_size(result.hoveredObjectCount, 1, "hoveredObjectCount");
        ok &= expect_size(result.changedObjectCount, 1, "changedObjectCount");

        return ok;
    }

    bool test_wireframe_selected_objects()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: wireframe em objetos selecionados ===\n";

        bool ok = true;

        graphics::RenderScene scene = make_three_object_scene();

        editor::SelectionState selection;
        selection.objects().set(editor::SceneNodeId{ 20 });

        editor::SelectionRenderOptions options{};
        options.wireframeSelectedObjects = true;

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, options, &result);

        print_result(result);

        ok &= expect(!output.objects()[0].wireframe, "object 10 wireframe false");
        ok &= expect(output.objects()[1].selected, "object 20 selected true");
        ok &= expect(output.objects()[1].wireframe, "object 20 wireframe true");
        ok &= expect(!output.objects()[2].wireframe, "object 30 wireframe false");

        ok &= expect_size(result.selectedObjectCount, 1, "selectedObjectCount");
        ok &= expect_size(result.changedObjectCount, 1, "changedObjectCount");

        return ok;
    }

    bool test_active_mesh_component_selection()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: selecao de componente marca mesh ativa ===\n";

        bool ok = true;

        graphics::RenderScene scene = make_three_object_scene();

        editor::SelectionState selection;
        selection.mesh().set_active_mesh(editor::SceneNodeId{ 30 });
        selection.mesh().set_vertex(kernel::geometry::VertexHandle{ 7 });

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, {}, &result);

        print_result(result);

        ok &= expect(!output.objects()[0].selected, "object 10 selected false");
        ok &= expect(!output.objects()[1].selected, "object 20 selected false");
        ok &= expect(output.objects()[2].selected, "object 30 selected por mesh component");

        ok &= expect_size(result.selectedObjectCount, 1, "selectedObjectCount");
        ok &= expect_size(result.hoveredObjectCount, 0, "hoveredObjectCount");
        ok &= expect_size(result.changedObjectCount, 1, "changedObjectCount");
        ok &= expect(result.activeMeshApplied, "activeMeshApplied true");
        ok &= expect(!result.activeMeshHoverApplied, "activeMeshHoverApplied false");

        return ok;
    }

    bool test_disable_active_mesh_component_selection()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: desativar selecao de componente ===\n";

        bool ok = true;

        graphics::RenderScene scene = make_three_object_scene();

        editor::SelectionState selection;
        selection.mesh().set_active_mesh(editor::SceneNodeId{ 30 });
        selection.mesh().set_vertex(kernel::geometry::VertexHandle{ 7 });

        editor::SelectionRenderOptions options{};
        options.applyActiveMeshSelection = false;

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, options, &result);

        print_result(result);

        ok &= expect(!output.objects()[0].selected, "object 10 selected false");
        ok &= expect(!output.objects()[1].selected, "object 20 selected false");
        ok &= expect(!output.objects()[2].selected, "object 30 selected false");

        ok &= expect_size(result.selectedObjectCount, 0, "selectedObjectCount");
        ok &= expect_size(result.changedObjectCount, 0, "changedObjectCount");
        ok &= expect(!result.activeMeshApplied, "activeMeshApplied false");

        return ok;
    }

    bool test_active_mesh_component_hover()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: hover de componente marca mesh ativa ===\n";

        bool ok = true;

        graphics::RenderScene scene = make_three_object_scene();

        editor::SelectionState selection;
        selection.mesh().set_active_mesh(editor::SceneNodeId{ 10 });
        selection.mesh().set_hovered_vertex(kernel::geometry::VertexHandle{ 3 });

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, {}, &result);

        print_result(result);

        ok &= expect(output.objects()[0].hovered, "object 10 hovered por componente");
        ok &= expect(!output.objects()[1].hovered, "object 20 hovered false");
        ok &= expect(!output.objects()[2].hovered, "object 30 hovered false");

        ok &= expect_size(result.selectedObjectCount, 0, "selectedObjectCount");
        ok &= expect_size(result.hoveredObjectCount, 1, "hoveredObjectCount");
        ok &= expect_size(result.changedObjectCount, 1, "changedObjectCount");
        ok &= expect(!result.activeMeshApplied, "activeMeshApplied false");
        ok &= expect(result.activeMeshHoverApplied, "activeMeshHoverApplied true");

        return ok;
    }

    bool test_render_object_id_zero_is_valid()
    {
        using namespace locus;

        std::cout << "\n=== SelectionRenderAdapter: RenderObject id 0 valido ===\n";

        bool ok = true;

        graphics::RenderScene scene;
        scene.add_object(make_object(0, "Object zero"));
        scene.add_object(make_object(10, "Valid object"));

        editor::SelectionState selection;
        selection.objects().set(editor::SceneNodeId{ 0 });
        selection.objects().set_hovered(editor::SceneNodeId{ 0 });

        editor::SelectionRenderResult result{};
        const graphics::RenderScene output =
            editor::SelectionRenderAdapter::apply_selection(scene, selection, {}, &result);

        print_result(result);

        ok &= expect(output.objects()[0].selected, "object id 0 selected true");
        ok &= expect(output.objects()[0].hovered, "object id 0 hovered true");
        ok &= expect(!output.objects()[1].selected, "object 10 selected false");
        ok &= expect(!output.objects()[1].hovered, "object 10 hovered false");

        ok &= expect_size(result.selectedObjectCount, 1, "selectedObjectCount");
        ok &= expect_size(result.hoveredObjectCount, 1, "hoveredObjectCount");
        ok &= expect_size(result.changedObjectCount, 1, "changedObjectCount");

        return ok;
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor SelectionRenderAdapter Smoke Test ===\n";

    bool ok = true;

    ok &= test_empty_scene();
    ok &= test_no_selection_clears_existing_flags();
    ok &= test_object_selection();
    ok &= test_active_object_selected_even_if_not_in_selected_set();
    ok &= test_disable_active_object_selection();
    ok &= test_hovered_object();
    ok &= test_selected_and_hovered_same_object();
    ok &= test_keep_existing_flags_when_clear_disabled();
    ok &= test_wireframe_selected_objects();
    ok &= test_active_mesh_component_selection();
    ok &= test_disable_active_mesh_component_selection();
    ok &= test_active_mesh_component_hover();
    ok &= ok &= test_render_object_id_zero_is_valid();

    std::cout << "\n=== Resultado final ===\n";

    if (ok) {
        std::cout << "[OK] Todos os testes de SelectionRenderAdapter passaram.\n";
        return EXIT_SUCCESS;
    }

    std::cout << "[FAIL] Algum teste de SelectionRenderAdapter falhou.\n";
    return EXIT_FAILURE;
}