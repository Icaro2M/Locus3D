#include "EditorSceneSnapshotBuilder.h"

EditorSceneSnapshot EditorSceneSnapshotBuilder::capture(
    const SceneContext& sceneContext,
    const EditorState& editorState
)
{
    EditorSceneSnapshot snapshot;

    const auto& objects = sceneContext.getScene().getObjects();
    SceneObject* selectedObject = editorState.getSelectedObject();

    for (size_t i = 0; i < objects.size(); ++i)
    {
        const SceneObject* object = objects[i];

        if (object == nullptr)
        {
            continue;
        }

        const Mesh& mesh = object->getMesh();
        const Transform& transform = object->getTransform();

        EditorObjectSnapshot objectSnapshot;
        objectSnapshot.name = object->getName();
        objectSnapshot.vertices = mesh.getVertices();
        objectSnapshot.indices = mesh.getIndices();
        objectSnapshot.logicalFaces = mesh.getLogicalFaces();
        objectSnapshot.position = transform.getPosition();
        objectSnapshot.rotation = transform.getRotation();
        objectSnapshot.scale = transform.getScale();

        if (object == selectedObject)
        {
            snapshot.context.selectedObjectIndex = static_cast<int>(snapshot.objects.size());
        }

        snapshot.objects.push_back(std::move(objectSnapshot));
    }

    snapshot.context.faceModeActive = editorState.isFaceModeActive();
    snapshot.context.activeTool = editorState.getActiveTool();
    snapshot.context.transformMode = editorState.getTransformMode();
    snapshot.context.transformSpace = editorState.getTransformSpace();

    return snapshot;
}

SceneObject* EditorSceneSnapshotBuilder::restore(
    const EditorSceneSnapshot& snapshot,
    SceneContext& sceneContext,
    EditorState& editorState
)
{
    sceneContext.clear();

    SceneObject* selectedObject = nullptr;

    for (size_t i = 0; i < snapshot.objects.size(); ++i)
    {
        const EditorObjectSnapshot& objectSnapshot = snapshot.objects[i];

        SceneObject* object = sceneContext.createObjectFromMeshData(
            objectSnapshot.name,
            objectSnapshot.vertices,
            objectSnapshot.indices,
            objectSnapshot.logicalFaces,
            objectSnapshot.position,
            objectSnapshot.rotation,
            objectSnapshot.scale
        );

        if (static_cast<int>(i) == snapshot.context.selectedObjectIndex)
        {
            selectedObject = object;
        }
    }

    editorState.setSelectedObject(selectedObject);
    editorState.clearSelectedFace();
    editorState.clearHoveredFace();
    editorState.setFaceModeActive(snapshot.context.faceModeActive);
    editorState.setActiveTool(snapshot.context.activeTool);
    editorState.setTransformMode(snapshot.context.transformMode);
    editorState.setTransformSpace(snapshot.context.transformSpace);
    editorState.setTransformAxis(TransformAxis::None);

    return selectedObject;
}