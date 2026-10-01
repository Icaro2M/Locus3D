#include "overlay/tests/VisibilitySceneTests.h"
#include "overlay/EditorBridge.h"
#include "overlay/LocusViewport.h"
#include "overlay/OutlinerWidget.h"
#include "editor/render/SelectionRenderAdapter.h"
#include "editor/scene/MeshNode.h"
#include "editor/sync/EditorSync.h"
#include "editor/tools/selection/shapes/SelectionShapeTypes.h"
#include "graphics/mesh/MeshRenderCache.h"
#include "graphics/mesh/MeshUploader.h"

#include <QHBoxLayout>
#include <QPersistentModelIndex>
#include <QSignalSpy>
#include <QTest>
#include <QTreeWidget>

#include <cstdio>

namespace locus::overlay::tests {
namespace {
bool check(bool condition, const char* message)
{
    if (!condition) std::fprintf(stderr, "Visibility scene regression: %s\n", message);
    return condition;
}

const graphics::RenderObject* object(const graphics::RenderScene& scene, editor::SceneNodeId id)
{
    for (const auto& entry : scene.objects()) if (entry.id == id.value) return &entry;
    return nullptr;
}

bool run_scenario(bool selected, bool twoObjects)
{
    EditorBridge bridge;
    QString error;
    QObject::connect(&bridge, &EditorBridge::errorOccurred, &bridge,
                     [&error](const QString& message) { error = message; });
    // Window (and GPU viewport) must be destroyed before the bridge/document.
    QWidget window;
    auto* layout = new QHBoxLayout(&window);
    auto* outliner = new OutlinerWidget(bridge, &window);
    auto* viewport = new LocusViewport(bridge, &window);
    outliner->setFixedWidth(300);
    layout->addWidget(outliner);
    layout->addWidget(viewport, 1);
    QObject::connect(&bridge, &EditorBridge::changed, outliner, &OutlinerWidget::refresh);
    window.resize(1100, 700);
    window.show();
    if (!check(QTest::qWaitForWindowExposed(&window), "fixture window not exposed")) return false;
    QTest::qWait(200);
    bridge.add_primitive(kernel::geometry::PrimitiveType::Box);
    const auto cubeId = bridge.selected_node();
    bridge.rename_node(cubeId, QStringLiteral("Visibility cube"));
    bridge.set_transform(0, -0.6);
    bridge.set_transform(4, 23.0);
    bridge.set_transform(6, 0.8);
    const auto transform = bridge.selected_transform();
    const auto topology = bridge.selected_mesh();
    editor::SceneNodeId secondId{};
    if (twoObjects) {
        bridge.add_primitive(kernel::geometry::PrimitiveType::Box);
        secondId = bridge.selected_node();
        bridge.set_transform(0, 0.9);
    }
    auto& document = bridge.document();
    auto& editor = document.editor();
    if (selected) bridge.select_node(cubeId);
    else {
        editor.selection_controller().clear_objects();
        editor.mark_dirty(editor::EditorDirtyFlags::Selection | editor::EditorDirtyFlags::Render);
        bridge.refresh();
    }
    if (selected) bridge.activate_transform(editor::GizmoMode::Translate);
    QTest::qWait(150);
    auto* tree = outliner->findChild<QTreeWidget*>();
    QTreeWidgetItem* cube = nullptr;
    for (auto iterator = QTreeWidgetItemIterator(tree); *iterator; ++iterator)
        if ((*iterator)->data(0, Qt::UserRole).toULongLong() == cubeId.value) cube = *iterator;
    if (!check(error.isEmpty() && viewport->healthy() && cube && transform && topology,
               "fixture creation failed")) return false;
    const QPersistentModelIndex index(tree->model()->index(tree->indexOfTopLevelItem(cube), 1));
    QSignalSpy resets(tree->model(), &QAbstractItemModel::modelReset);
    const auto* node = dynamic_cast<const editor::MeshNode*>(editor.scene().find_node(cubeId));
    if (!check(node != nullptr, "cube node is not a mesh")) return false;
    const auto* mesh = &node->mesh();
    const auto revision = node->mesh_revision();
    const auto matrix = node->transform().matrix();
    const auto* initialObject = object(document.editor_sync().render_scene(), cubeId);
    if (!check(initialObject && initialObject->mesh && initialObject->mesh->is_valid(),
               "initial GPU upload missing")) return false;
    const auto* gpu = initialObject->mesh;

    auto validate = [&](bool visible) {
        const auto framebuffer = viewport->grabFramebuffer();
        const auto* currentNode = dynamic_cast<const editor::MeshNode*>(editor.scene().find_node(cubeId));
        const auto* rendered = object(document.editor_sync().render_scene(), cubeId);
        if (!check(error.isEmpty() && viewport->healthy() && index.isValid() && resets.isEmpty() &&
                   bridge.nodes().size() == (twoObjects ? 2 : 1) && cube->text(0) == QStringLiteral("Visibility cube"),
                   "UI row/name/scene changed unexpectedly")) return false;
        if (!check(currentNode == node && &currentNode->mesh() == mesh && currentNode->mesh_revision() == revision &&
                   currentNode->is_visible() == visible && currentNode->mesh().vertex_count() == topology->vertices &&
                   currentNode->mesh().face_count() == topology->faces && currentNode->transform().matrix() == matrix,
                   "visibility lost or modified source mesh")) return false;
        if (!check(rendered && rendered->mesh == gpu && gpu->is_valid() && rendered->visibility.visible == visible &&
                   rendered->visibility.selectable == visible && (cube->checkState(1) == Qt::Checked) == visible,
                   "GPU mesh/visibility/selectability inconsistent")) return false;
        if (!check(bridge.selected_node() == (selected ? cubeId : editor::SceneNodeId{}),
                   "visibility changed selection")) return false;
        if (selected) {
            const auto currentTransform = bridge.selected_transform();
            if (!check(currentTransform && currentTransform->values == transform->values,
                       "visibility lost transform")) return false;
        } else {
            const auto& t = currentNode->transform();
            if (!check(t.position().x == static_cast<float>(transform->values[0]) &&
                       t.scale().x == static_cast<float>(transform->values[6]), "unselected transform lost")) return false;
        }
        const auto highlights = editor::SelectionRenderAdapter::build_object_highlights(
            document.editor_sync().render_scene(), editor.selection());
        if (!check(highlights.size() == (selected && visible ? 1u : 0u),
                   "hidden/unselected object received selection overlay")) return false;
        if (twoObjects) {
            const auto* other = object(document.editor_sync().render_scene(), secondId);
            if (!check(other && other->visibility.visible && other->mesh && other->mesh->is_valid(),
                       "hiding cube disturbed the other object")) return false;
        }
        viewport->makeCurrent();
        bool cubeHit = false;
        bool secondHit = false;
        const auto hits = bridge.viewport().read_picking_region(document,
            {{0.0f, 0.0f}, {float(framebuffer.width() - 1), float(framebuffer.height() - 1)}});
        if (hits) {
            for (const auto pickingId : hits.value()) {
                const auto hitId = document.editor_sync().picking_sync().scene_node_id(pickingId);
                cubeHit |= hitId == cubeId;
                secondHit |= hitId == secondId;
            }
        }
        viewport->doneCurrent();
        return check(bool(hits) && cubeHit == visible && (!twoObjects || secondHit),
                     "hidden object picked / visible object not selectable");
    };

    if (!validate(true)) return false;
    for (int step = 0; step < 12; ++step) {
        const bool visible = step % 2 != 0;
        cube->setCheckState(1, visible ? Qt::Checked : Qt::Unchecked);
        if (!validate(visible)) return false;
    }
    for (int cycle = 0; cycle < 4; ++cycle) {
        bridge.undo();
        if (!validate(false)) return false;
        bridge.redo();
        if (!validate(true)) return false;
    }
    bridge.select_node(cubeId);
    if (!check(bridge.selected_mesh().has_value() && bridge.selected_node() == cubeId,
               "shown cube no longer selectable")) return false;

    // Additionally exercise actual omission/reinsertion into RenderScene.
    viewport->makeCurrent();
    bool cachePassed = true;
    {
        graphics::MeshRenderCache cache;
        graphics::MeshUploader uploader;
        editor::EditorSync sync;
        editor::EditorSyncOptions options;
        options.clearDirtyFlagsAfterSync = false;
        options.renderSceneOptions.sceneOptions.includeHiddenNodes = false;
        options.renderSceneOptions.sceneOptions.allowNullGpuMeshes = false;
        options.renderSceneOptions.sceneOptions.meshRevisionResolver =
            [](const editor::MeshNode& n) { return n.mesh_revision(); };
        const graphics::GpuMesh* cachedGpu = nullptr;
        for (int step = 0; step < 9 && cachePassed; ++step) {
            const bool visible = step % 2 == 0;
            bridge.set_visibility(cubeId, visible);
            editor.mark_dirty(editor::EditorDirtyFlags::Scene | editor::EditorDirtyFlags::Render);
            cache.begin_frame();
            const auto result = sync.sync_cached_if_needed(editor, cache, uploader, options);
            const auto* entry = object(sync.render_scene(), cubeId);
            cachePassed = check(bool(result) && sync.render_scene().object_count() ==
                (visible ? (twoObjects ? 2u : 1u) : (twoObjects ? 1u : 0u)), "omitted scene rebuild failed");
            if (visible && cachePassed) {
                if (!cachedGpu && entry) cachedGpu = entry->mesh;
                cachePassed = check(entry && entry->mesh == cachedGpu && cachedGpu && cachedGpu->is_valid(),
                                    "reinserted render object lost GPU ownership");
            } else if (cachePassed) {
                cachePassed = check(!entry && cache.find({cubeId.value, revision}) == cachedGpu,
                                    "hidden node invalidated cached GPU mesh");
            }
        }
        cachePassed = check(cachePassed && cache.size() == (twoObjects ? 2u : 1u),
                            "visibility created duplicate cache entries");
        sync.clear();
        cache.clear();
    }
    viewport->doneCurrent();
    std::fprintf(stderr, "Visibility scene regression: selected=%d objects=%d %s\n",
                 selected, twoObjects ? 2 : 1, cachePassed ? "passed" : "failed");
    return cachePassed;
}
}

bool run_visibility_scene_tests()
{
    for (const bool selected : {true, false})
        for (const bool twoObjects : {false, true})
            if (!run_scenario(selected, twoObjects)) return false;
    return true;
}
}
