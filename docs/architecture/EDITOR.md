# Locus3D Architecture

This document contains high-level architecture notes for the planned editor production code in `source/editor/`.

This document is a development guide for the future object scene, selection, command history, tools, gizmos, snapping, synchronization, and document IO systems.

For the graphics foundation, see [`GRAPHICS.md`](GRAPHICS.md). For the geometry/modeling kernel, see [`GEOMETRY.md`](GEOMETRY.md). For the editable mesh design, see [`LEM.md`](LEM.md).

---

## Editor Layer

The editor layer is the interactive modeling surface that sits above the geometry kernel and graphics layer. It owns high-level scene/object state, selection workflows, command dispatch, undo/redo history, active tools, transform gizmos, snapping behavior, and synchronization with render, picking, and manufacturing-facing systems.

It should not own low-level geometry invariants or GPU resources. Mesh topology, modeling operations, spatial queries, validation, import/export mesh formats, and future manufacturing analysis should live in `source/kernel/`. Rendering, viewport drawing, GPU picking buffers, overlays, cameras, and render scenes should live in `source/graphics/`.

Conceptually:

```txt
application/ui
        |
        v
source/editor scene, tools, selection, commands, history, snapping
        |
        +--> source/kernel geometry, modeling, validation, manufacturing analysis
        |
        v
source/graphics render scene, overlays, picking, viewport presentation
```

---

## Module Responsibilities

- root files: editor aggregator and top-level editor context/state types.
- `scene/`: editor-owned object hierarchy, scene nodes, transforms, pivots, metadata, object types, and mesh/empty node records.
- `selection/`: object and mesh selection state, selection sets, selection granularity/scope, controller behavior, and serialization boundary.
- `command/`: command interface, command context, command dispatch, command registration, and concrete editor commands.
- `history/`: undo/redo stacks, history entries, and configuration for command retention/merging.
- `tools/`: active tool abstraction, tool registry/manager, tool context, interaction helpers, and concrete selection, transform, mesh, creation, and utility tools.
- `actions/`: command-like editor actions for non-modal operations, action registration, execution, and mesh operation action groups.
- `gizmo/`: transform gizmo state, axes, hit testing, snapping constraints, and manipulation controller.
- `transform/`: transform sessions, transform targets, coordinate spaces, and pivot resolution.
- `snapping/`: snap settings, snap contexts/results, snap solver, and snap providers for grids, mesh elements, increments, and angles.
- `sync/`: explicit synchronization from editor state into render scenes, picking data, and planned manufacturing workflows.
- `io/`: editor document serialization boundaries and the planned native Locus3D document format.

### Editor Boundaries

The editor layer should coordinate systems rather than absorbing their internals.

Selection can reference scene nodes and mesh element handles, but editable topology should remain in the geometry kernel. Gizmos and tools can request render overlays and picking IDs, but GPU resource ownership should remain in the graphics layer. Commands can execute kernel modeling operations, but the command history should store editor-level intent and reversible state rather than becoming a hidden geometry backend.

---

## Current File Tree

Generated in the style of `tree /f` from the planned `source/editor/` tree.

Legend:

- `[!]`: planned or incomplete file/directory, not present in the current tree yet.

```txt
source\editor 
|   Editor.h 
|   Editor.cpp 
|   EditorTypes.h 
|   EditorContext.h 
|   EditorState.h
|
+---scene 
|       EditorScene.h 
|       EditorScene.cpp 
|       SceneNode.h 
|       SceneNode.cpp 
|       SceneTree.h 
|       SceneTree.cpp 
|       SceneNodeId.h 
|       NodeTransform.h 
|       NodeTransform.cpp 
|       NodePivot.h 
|       NodeMetadata.h 
|       NodeType.h 
|       EmptyNode.h 
|       MeshNode.h
|
+---selection
|       SelectionState.h
|       SelectionState.cpp
|       ObjectSelection.h
|       ObjectSelection.cpp
|       MeshSelection.h
|       MeshSelection.cpp
|       SelectionSet.h
|       SelectionSet.cpp
|       SelectionGranularity.h
|       SelectionScope.h
|       SelectionController.h
|       SelectionController.cpp
|       SelectionSerializer.h
|
+---command
|   |   ICommand.h
|   |   CommandContext.h 
|   |   CommandResult.h
|   |   CommandDispatcher.h 
|   |   CommandDispatcher.cpp 
|   |   CommandRegistry.h 
|   |   CommandRegistry.cpp
|   |
|   +---scene
|   |       CreateEmptyNodeCommand.h
|   |       CreateEmptyNodeCommand.cpp
|   |       CreateMeshNodeCommand.h
|   |       CreateMeshNodeCommand.cpp
|   |       DeleteNodeCommand.h 
|   |       DeleteNodeCommand.cpp
|   |       DuplicateNodeCommand.h
|   |       DuplicateNodeCommand.cpp
|   |       RenameNodeCommand.h
|   |       RenameNodeCommand.cpp
|   |       ReparentNodeCommand.h
|   |       ReparentNodeCommand.cpp
|   |       SetNodeVisibilityCommand.h
|   |       SetNodeVisibilityCommand.cpp
|   |       SetNodeLockCommand.h
|   |       SetNodeLockCommand.cpp
|   |       SetNodeSelectableCommand.h
|   |       SetNodeSelectableCommand.cpp
|   |
|   +---selection
|   |       ObjectSelectionSnapshot.h
|   |       MeshSelectionSnapshot.h
|   |       SelectObjectCommand.h
|   |       SelectObjectCommand.cpp
|   |       ToggleObjectSelectionCommand.h
|   |       ToggleObjectSelectionCommand.cpp
|   |       ClearObjectSelectionCommand.h
|   |       ClearObjectSelectionCommand.cpp
|   |       SelectMeshComponentCommand.h
|   |       SelectMeshComponentCommand.cpp
|   |       ToggleMeshComponentSelectionCommand.h
|   |       ToggleMeshComponentSelectionCommand.cpp
|   |       ClearMeshSelectionCommand.h
|   |       ClearMeshSelectionCommand.cpp
|   |       SetSelectionGranularityCommand.h
|   |       SetSelectionGranularityCommand.cpp
|   |       SetSelectionScopeCommand.h
|   |       SetSelectionScopeCommand.cpp
|   |
|   +---transform
|   |       NodeTransformSnapshot.h
|   |       SetNodeTransformCommand.h
|   |       SetNodeTransformCommand.cpp
|   |       TranslateNodeCommand.h
|   |       TranslateNodeCommand.cpp
|   |       RotateNodeCommand.h
|   |       RotateNodeCommand.cpp
|   |       ScaleNodeCommand.h
|   |       ScaleNodeCommand.cpp
|   |       SetNodePivotCommand.h
|   |       SetNodePivotCommand.cpp
|   |
|   +---mesh
|   |       MeshSnapshot.h
|   |       ApplyMeshOperationCommand.h
|   |       ApplyMeshOperationCommand.cpp
|   |       ReplaceMeshCommand.h
|   |       ReplaceMeshCommand.cpp
|   |       EditMeshSelectionCommand.h
|   |       EditMeshSelectionCommand.cpp
|   |
|   \---document
|           ClearSceneCommand.h
|           ClearSceneCommand.cpp
|           ImportMeshCommand.h
|           ImportMeshCommand.cpp
|
+---history
|       HistoryStack.h
|       HistoryStack.cpp
|       HistoryEntry.h
|       HistoryConfig.h
|
+---tools
|   |   Tools.h [!]
|   |
|   +---core
|   |       ToolId.h
|   |       ToolCategory.h
|   |       ToolState.h
|   |       ToolCapabilities.h
|   |       ToolDescriptor.h
|   |       ToolEvent.h
|   |       ToolInputState.h [!]
|   |       ToolResult.h
|   |       ToolContext.h
|   |       ITool.h
|   |
|   +---management
|   |       ActiveTool.h
|   |       ToolRegistry.h
|   |       ToolRegistry.cpp
|   |       ToolManager.h
|   |       ToolManager.cpp
|   |
|   +---interaction [!]
|   |       ToolCancelReason.h
|   |       ToolCapture.h
|   |       ModalTool.h
|   |       DragTool.h
|   |       DragTool.cpp
|   |
|   +---selection [!]
|   |   |   SelectTool.h [!]
|   |   |   SelectTool.cpp [!]
|   |   |
|   |   \---shapes [!]
|   |           ISelectionShape.h [!]
|   |           PointSelectionShape.h [!]
|   |           PointSelectionShape.cpp [!]
|   |           BoxSelectionShape.h [!]
|   |           BoxSelectionShape.cpp [!]
|   |           CircleSelectionShape.h [!]
|   |           CircleSelectionShape.cpp [!]
|   |           LassoSelectionShape.h [!]
|   |           LassoSelectionShape.cpp [!]
|   |
|   +---transform [!]
|   |       TransformTool.h [!]
|   |       TransformTool.cpp [!]
|   |       ITransformToolSession.h [!]
|   |       ObjectTransformToolSession.h [!]
|   |       ObjectTransformToolSession.cpp [!]
|   |       MeshTransformToolSession.h [!]
|   |       MeshTransformToolSession.cpp [!]
|   |       PivotTool.h [!]
|   |       PivotTool.cpp [!]
|   |
|   +---mesh [!]
|   |   +---core [!]
|   |   |       MeshToolTarget.h [!]
|   |   |       MeshOperationSession.h [!]
|   |   |       MeshOperationSession.cpp [!]
|   |   |       MeshDragOperationTool.h [!]
|   |   |       MeshDragOperationTool.cpp [!]
|   |   |
|   |   +---face [!]
|   |   |       ExtrudeFaceTool.h [!]
|   |   |       ExtrudeFaceTool.cpp [!]
|   |   |       InsetFaceTool.h [!]
|   |   |       InsetFaceTool.cpp [!]
|   |   |
|   |   +---edge [!]
|   |   |       EdgeSlideTool.h [!]
|   |   |       EdgeSlideTool.cpp [!]
|   |   |       BevelTool.h [!]
|   |   |       BevelTool.cpp [!]
|   |   |
|   |   \---topology [!]
|   |           LoopCutTool.h [!]
|   |           LoopCutTool.cpp [!]
|   |
|   +---creation [!]
|   |       PrimitiveCreateTool.h [!]
|   |       PrimitiveCreateTool.cpp [!]
|   |
|   \---utility [!]
|           MeasureTool.h [!]
|           MeasureTool.cpp [!]
|           InspectTool.h [!]
|           InspectTool.cpp [!]
|           SetOriginTool.h [!]
|           SetOriginTool.cpp [!]
|
+---actions [!]
|   |   ActionRegistry.h [!]
|   |   ActionRegistry.cpp [!]
|   |   ActionExecutor.h [!]
|   |   ActionExecutor.cpp [!]
|   |   Actions.h [!]
|   |
|   +---core [!]
|   |       ActionId.h [!]
|   |       ActionCategory.h [!]
|   |       ActionDescriptor.h [!]
|   |       ActionContext.h [!]
|   |       ActionResult.h [!]
|   |       IEditorAction.h [!]
|   |
|   \---mesh [!]
|       |   MeshOperationAction.h [!]
|       |   MeshOperationAction.cpp [!]
|       |
|       +---vertex [!]
|       |       register_vertex_actions.h [!]
|       |       register_vertex_actions.cpp [!]
|       |
|       +---edge [!]
|       |       register_edge_actions.h [!]
|       |       register_edge_actions.cpp [!]
|       |
|       +---face [!]
|       |       register_face_actions.h [!]
|       |       register_face_actions.cpp [!]
|       |
|       \---topology [!]
|               register_topology_actions.h [!]
|               register_topology_actions.cpp [!]
|
+---gizmo
|       GizmoMode.h
|       GizmoState.h
|       GizmoController.h
|       GizmoController.cpp
|       GizmoAxis.h
|       GizmoHit.h
|       GizmoSnap.h
|       GizmoSnap.cpp
|       GizmoConstraint.h
|       GizmoConstraint.cpp
|       TransformGizmo.h
|       TransformGizmo.cpp
|
+---transform
|       TransformSpace.h
|       TransformSession.h
|       TransformSession.cpp
|       TransformTarget.h
|       TransformTarget.cpp
|       TransformPivotResolver.h
|       TransformPivotResolver.cpp
|
+---snapping
|       SnapMode.h
|       SnapSettings.h
|       SnapSettings.cpp
|       SnapTarget.h
|       SnapResult.h
|       SnapContext.h
|       ISnapProvider.h
|       SnapSolver.h
|       SnapSolver.cpp
|       GridSnapProvider.h
|       GridSnapProvider.cpp
|       VertexSnapProvider.h
|       VertexSnapProvider.cpp
|       EdgeSnapProvider.h
|       EdgeSnapProvider.cpp
|       FaceSnapProvider.h
|       FaceSnapProvider.cpp
|       IncrementSnapProvider.h
|       IncrementSnapProvider.cpp
|       AngleSnapProvider.h
|       AngleSnapProvider.cpp
|
+---render
|       RenderAdapterTypes.h
|       RenderMeshUploadAdapter.h
|       RenderMeshUploadAdapter.cpp
|       MeshNodeRenderAdapter.h
|       MeshNodeRenderAdapter.cpp
|       SceneRenderAdapter.h
|       SceneRenderAdapter.cpp
|       SelectionRenderAdapter.h 
|       SelectionRenderAdapter.cpp 
|       OverlayRenderAdapter.h
|       OverlayRenderAdapter.cpp
|       PreviewRenderAdapter.h [!]
|       PreviewRenderAdapter.cpp [!]
|       PickingRenderAdapter.h
|       PickingRenderAdapter.cpp
|
+---sync
|       EditorSync.h
|       EditorSync.cpp
|       RenderSceneSync.h
|       RenderSceneSync.cpp
|       PickingSync.h
|       PickingSync.cpp
|       ManufacturingSync.h [!]
|       ManufacturingSync.cpp [!]
|
\---io [!]
        IDocumentSerializer.h [!]
        DocumentReader.h [!]
        DocumentWriter.h [!]
        Locus3DFormat.h [!]
        Locus3DFormat.cpp [!]
```

---

## Planned Or Incomplete Editor Files

Initial implementation should probably begin with a small, stable foundation:

- [!] `tools/Tools.h`
- [!] `tools/core/ToolInputState.h`
- [!] `tools/selection/SelectTool.h`
- [!] `tools/selection/SelectTool.cpp`
- [!] `tools/selection/shapes/ISelectionShape.h`
- [!] `tools/selection/shapes/PointSelectionShape.h`
- [!] `tools/selection/shapes/PointSelectionShape.cpp`
- [!] `tools/selection/shapes/BoxSelectionShape.h`
- [!] `tools/selection/shapes/BoxSelectionShape.cpp`
- [!] `tools/selection/shapes/CircleSelectionShape.h`
- [!] `tools/selection/shapes/CircleSelectionShape.cpp`
- [!] `tools/selection/shapes/LassoSelectionShape.h`
- [!] `tools/selection/shapes/LassoSelectionShape.cpp`
- [!] `tools/transform/TransformTool.h`
- [!] `tools/transform/TransformTool.cpp`
- [!] `tools/transform/ITransformToolSession.h`
- [!] `tools/transform/ObjectTransformToolSession.h`
- [!] `tools/transform/ObjectTransformToolSession.cpp`
- [!] `tools/transform/MeshTransformToolSession.h`
- [!] `tools/transform/MeshTransformToolSession.cpp`
- [!] `tools/transform/PivotTool.h`
- [!] `tools/transform/PivotTool.cpp`
- [!] `tools/mesh/core/MeshToolTarget.h`
- [!] `tools/mesh/core/MeshOperationSession.h`
- [!] `tools/mesh/core/MeshOperationSession.cpp`
- [!] `tools/mesh/core/MeshDragOperationTool.h`
- [!] `tools/mesh/core/MeshDragOperationTool.cpp`
- [!] `tools/mesh/face/ExtrudeFaceTool.h`
- [!] `tools/mesh/face/ExtrudeFaceTool.cpp`
- [!] `tools/mesh/face/InsetFaceTool.h`
- [!] `tools/mesh/face/InsetFaceTool.cpp`
- [!] `tools/mesh/edge/EdgeSlideTool.h`
- [!] `tools/mesh/edge/EdgeSlideTool.cpp`
- [!] `tools/mesh/edge/BevelTool.h`
- [!] `tools/mesh/edge/BevelTool.cpp`
- [!] `tools/mesh/topology/LoopCutTool.h`
- [!] `tools/mesh/topology/LoopCutTool.cpp`
- [!] `tools/creation/PrimitiveCreateTool.h`
- [!] `tools/creation/PrimitiveCreateTool.cpp`
- [!] `tools/utility/MeasureTool.h`
- [!] `tools/utility/MeasureTool.cpp`
- [!] `tools/utility/InspectTool.h`
- [!] `tools/utility/InspectTool.cpp`
- [!] `tools/utility/SetOriginTool.h`
- [!] `tools/utility/SetOriginTool.cpp`
- [!] `actions/core/ActionId.h`
- [!] `actions/core/ActionCategory.h`
- [!] `actions/core/ActionDescriptor.h`
- [!] `actions/core/ActionContext.h`
- [!] `actions/core/ActionResult.h`
- [!] `actions/core/IEditorAction.h`
- [!] `actions/ActionRegistry.h`
- [!] `actions/ActionRegistry.cpp`
- [!] `actions/ActionExecutor.h`
- [!] `actions/ActionExecutor.cpp`
- [!] `actions/mesh/MeshOperationAction.h`
- [!] `actions/mesh/MeshOperationAction.cpp`
- [!] `actions/mesh/vertex/register_vertex_actions.h`
- [!] `actions/mesh/vertex/register_vertex_actions.cpp`
- [!] `actions/mesh/edge/register_edge_actions.h`
- [!] `actions/mesh/edge/register_edge_actions.cpp`
- [!] `actions/mesh/face/register_face_actions.h`
- [!] `actions/mesh/face/register_face_actions.cpp`
- [!] `actions/mesh/topology/register_topology_actions.h`
- [!] `actions/mesh/topology/register_topology_actions.cpp`
- [!] `actions/Actions.h`
- [!] `render/PreviewRenderAdapter.h`
- [!] `render/PreviewRenderAdapter.cpp`
- [!] `sync/ManufacturingSync.h`
- [!] `sync/ManufacturingSync.cpp`
- [!] `io/IDocumentSerializer.h`
- [!] `io/DocumentReader.h`
- [!] `io/DocumentWriter.h`
- [!] `io/Locus3DFormat.h`
- [!] `io/Locus3DFormat.cpp`

These should be added in dependency order. The first useful milestone is likely editor scene identity, node transforms, object selection, and render/picking synchronization. Commands, history, tools, gizmos, snapping, and document IO can then build on that foundation without guessing at ownership.

---

## Notes For AI-Assisted Work

When changing future editor code:

- Keep editor state above `source/kernel/` and `source/graphics/`.
- Prefer explicit IDs, handles, and contexts over raw ownership between editor nodes, meshes, render objects, and commands.
- Keep command execution reversible and route user-visible mutations through command/history boundaries when possible.
- Keep selection state separate from geometry topology and render highlighting.
- Treat snapping and gizmo manipulation as editor interaction systems that consume geometry queries and graphics picking results through explicit APIs.
- Keep render and picking synchronization one-way from editor state into graphics-owned scene/picking data.
- Keep manufacturing synchronization separate from viewport interaction; fabrication analysis should remain owned by the kernel manufacturing module when it exists.
- Update this document when files are added, renamed, or promoted from planned to implemented.
