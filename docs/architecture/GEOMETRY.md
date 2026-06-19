# Locus3D Architecture

This document contains high-level architecture notes for the geometry production code in `source/kernel/`.

The geometry layer is currently in an early production stage. Its first stable pieces are the editable mesh core, topology traversal/validation helpers, render-mesh derivation utilities, and shared kernel math/common types.

For the editable mesh design, see [`LEM.md`](LEM.md).

---

## Geometry Layer

The geometry layer is the modeling kernel foundation. It owns editable mesh data structures, topology utilities, geometry math, spatial queries, modeling operations, import/export boundaries, and validation for printable geometry.

It should not own high-level editor state. Systems such as tools, UI panels, command history, scene/object management, selection workflows, snapping state, and viewport interaction should live above this layer and call into it through explicit geometry/modeling APIs.

Conceptually:

```txt
application/editor/tools
        |
        v
source/kernel modeling and validation APIs
        |
        v
editable geometry, topology, queries, and derived render data
        |
        v
source/graphics render upload and viewport presentation
```

---

## Module Responsibilities

- `common/`: shared kernel identifiers, result/error types, scalar aliases, typed handles, and storage helpers.
- `math/`: geometry-oriented math helpers such as rays, bounds, transforms, intersections, matrices, vectors, and quaternions.
- `geometry/mesh/`: Locus Editable Mesh (LEM), typed mesh handles, element records, mesh storage/editing helpers, and mesh diffs.
- `geometry/topology/`: topology construction, traversal, and structural validation for editable meshes.
- `geometry/spatial/`: acceleration structures and spatial queries such as BVH construction, ray tests, bounds tests, and proximity lookup.
- `geometry/render/`: derived renderable mesh data, triangulation, normal generation, and wireframe extraction from editable geometry.
- `geometry/primitives/`: primitive mesh builders and the registry used to create built-in shapes.
- `geometry/queries/`: reusable geometry queries for raycasting, adjacency, bounds, selection hits, and proximity.
- `modeling/core/`: operation interfaces, execution context, and operation result types.
- `modeling/operations/`: concrete mesh modeling operations grouped by face, edge, topology, and transform behavior.
- `modeling/preview/`: non-destructive operation previews, ghost meshes, and preview strategy interfaces.
- `io/`: import/export interfaces and format-specific mesh serialization.
- `validation/`: validation pipeline, reports, and checks for topology, geometric consistency, and 3D-printability.

---

## Current File Tree

Generated in the style of `tree /f` from `source/kernel/`.

Legend:

- `[!]`: planned or incomplete file, not present in the current tree yet.

```txt
source\kernel
+---common
|       Id.h
|       Result.h
|       Error.h
|       Types.h
|       Handle.h
|       Pool.h
|
+---math
|       Vec.h [!]
|       Mat.h [!]
|       Ray.h
|       Bounds.h
|       Transform.h
|       Quaternion.h [!]
|       Intersections.h
|       GeometryMath.h
|
+---geometry
|   +---mesh
|   |   |   LEMTypes.h
|   |   |   LEMHandles.h
|   |   |   LEM.h
|   |   |   LEM.cpp
|   |   |   LEMStorage.h [!]
|   |   |   LEMEditor.h
|   |   |   LEMDiff.h
|   |   |
|   |   \---elements
|   |           Vertex.h
|   |           Edge.h
|   |           Loop.h
|   |           Face.h
|   |
|   +---topology
|   |       TopologyBuilder.h
|   |       TopologyTraversal.h
|   |       TopologyValidator.h
|   |
|   +---spatial
|   |       SpatialIndex.h [!]
|   |       BVH.h [!]
|   |       BVH.cpp [!]
|   |       BVHBuilder.h [!]
|   |       BVHQuery.h [!]
|   |
|   +---render
|   |       RenderMesh.h
|   |       RenderMesh.cpp [!]
|   |       MeshTriangulator.h
|   |       NormalBuilder.h
|   |       WireframeBuilder.h
|   |
|   +---primitives
|   |       IPrimitiveBuilder.h [!]
|   |       PrimitiveParameters.h
|   |       BoxBuilder.h
|   |       CylinderBuilder.h [!]
|   |       SphereBuilder.h [!]
|   |       ConeBuilder.h [!]
|   |       PrimitiveRegistry.h [!]
|   |
|   \---queries
|           RaycastQuery.h [!]
|           AdjacencyQuery.h
|           BoundsQuery.h
|           SelectionQuery.h [!]
|           SelectionHit.h
|           ProximityQuery.h [!]
|
+---modeling
|   +---core
|   |       IOperation.h
|   |       OperationContext.h
|   |       OperationResult.h
|   |
|   +---operations
|   |   +---face
|   |   |       ExtrudeFaceOp.h [!]
|   |   |       ExtrudeFaceOp.cpp [!]
|   |   |       InsetFaceOp.h [!]
|   |   |       InsetFaceOp.cpp [!]
|   |   |       FlipFaceOp.h
|   |   |       FlipFaceOp.cpp
|   |   |       SolidifyOp.h [!]
|   |   |
|   |   +---edge
|   |   |       BevelOp.h [!]
|   |   |       BevelOp.cpp [!]
|   |   |       EdgeSlideOp.h [!]
|   |   |       CreaseOp.h [!]
|   |   |
|   |   +---topology
|   |   |       LoopCutOp.h [!]
|   |   |       LoopCutOp.cpp [!]
|   |   |       SubdivideOp.h [!]
|   |   |       SubdivideOp.cpp [!]
|   |   |       MergeVerticesOp.h [!]
|   |   |       BridgeEdgeOp.h [!]
|   |   |       FillHoleOp.h [!]
|   |   |
|   |   \---transform
|   |           TransformOp.h
|   |           TransformOp.cpp
|   |           ShrinkFattenOp.h [!]
|   |           RandomizeOp.h [!]
|   |
|   \---preview
|           IPreviewStrategy.h [!]
|           OperationPreview.h [!]
|           PreviewMesh.h [!]
|           GhostMeshBuilder.h [!]
|
+---io
|       IExporter.h [!]
|       IImporter.h [!]
|       StlExporter.h [!]
|       StlImporter.h [!]
|       ObjExporter.h [!]
|       ObjImporter.h [!]
|       ThreeMFExporter.h [!]
|       FormatRegistry.h [!]
|
\---validation
    +---pipeline
    |       ValidationPipeline.h [!]
    |       ValidationContext.h [!]
    |       ValidationReport.h [!]
    |
    \---checks
        +---topology
        |       ManifoldCheck.h [!]
        |       BoundaryCheck.h [!]
        |
        +---geometry
        |       NormalConsistencyCheck.h [!]
        |       DegenerateCheck.h [!]
        |       IntersectionCheck.h [!]
        |
        \---printability
                ThinWallCheck.h [!]
                PrintabilityCheck.h [!]
```

---

## Planned Or Incomplete Geometry Files

The files marked with `[!]` in the tree are planned or incomplete parts of the geometry layer that are not present in the current `source/kernel/` tree yet.

- [!] `math/Vec.h`
- [!] `math/Mat.h`
- [!] `math/Quaternion.h`
- [!] `geometry/mesh/LEMStorage.h`
- [!] `geometry/spatial/SpatialIndex.h`
- [!] `geometry/spatial/BVH.h`
- [!] `geometry/spatial/BVH.cpp`
- [!] `geometry/spatial/BVHBuilder.h`
- [!] `geometry/spatial/BVHQuery.h`
- [!] `geometry/render/RenderMesh.cpp`
- [!] `geometry/primitives/IPrimitiveBuilder.h`
- [!] `geometry/primitives/CylinderBuilder.h`
- [!] `geometry/primitives/SphereBuilder.h`
- [!] `geometry/primitives/ConeBuilder.h`
- [!] `geometry/primitives/PrimitiveRegistry.h`
- [!] `geometry/queries/RaycastQuery.h`
- [!] `geometry/queries/SelectionQuery.h`
- [!] `geometry/queries/ProximityQuery.h`
- [!] `modeling/operations/face/ExtrudeFaceOp.h`
- [!] `modeling/operations/face/ExtrudeFaceOp.cpp`
- [!] `modeling/operations/face/InsetFaceOp.h`
- [!] `modeling/operations/face/InsetFaceOp.cpp`
- [!] `modeling/operations/face/SolidifyOp.h`
- [!] `modeling/operations/edge/BevelOp.h`
- [!] `modeling/operations/edge/BevelOp.cpp`
- [!] `modeling/operations/edge/EdgeSlideOp.h`
- [!] `modeling/operations/edge/CreaseOp.h`
- [!] `modeling/operations/topology/LoopCutOp.h`
- [!] `modeling/operations/topology/LoopCutOp.cpp`
- [!] `modeling/operations/topology/SubdivideOp.h`
- [!] `modeling/operations/topology/SubdivideOp.cpp`
- [!] `modeling/operations/topology/MergeVerticesOp.h`
- [!] `modeling/operations/topology/BridgeEdgeOp.h`
- [!] `modeling/operations/topology/FillHoleOp.h`
- [!] `modeling/operations/transform/ShrinkFattenOp.h`
- [!] `modeling/operations/transform/RandomizeOp.h`
- [!] `modeling/preview/IPreviewStrategy.h`
- [!] `modeling/preview/OperationPreview.h`
- [!] `modeling/preview/PreviewMesh.h`
- [!] `modeling/preview/GhostMeshBuilder.h`
- [!] `io/IExporter.h`
- [!] `io/IImporter.h`
- [!] `io/StlExporter.h`
- [!] `io/StlImporter.h`
- [!] `io/ObjExporter.h`
- [!] `io/ObjImporter.h`
- [!] `io/ThreeMFExporter.h`
- [!] `io/FormatRegistry.h`
- [!] `validation/pipeline/ValidationPipeline.h`
- [!] `validation/pipeline/ValidationContext.h`
- [!] `validation/pipeline/ValidationReport.h`
- [!] `validation/checks/topology/ManifoldCheck.h`
- [!] `validation/checks/topology/BoundaryCheck.h`
- [!] `validation/checks/geometry/NormalConsistencyCheck.h`
- [!] `validation/checks/geometry/DegenerateCheck.h`
- [!] `validation/checks/geometry/IntersectionCheck.h`
- [!] `validation/checks/printability/ThinWallCheck.h`
- [!] `validation/checks/printability/PrintabilityCheck.h`

These should be added as their owning boundaries become clear. In particular, modeling operations should depend on stable LEM editing primitives and explicit operation contexts, while validation and printability checks should stay independent from editor UI and viewport state.

---

## Current Implemented Foundation

The existing kernel code already provides:

- typed kernel/common utilities through `Id.h`, `Error.h`, `Result.h`, and `Types.h`;
- basic geometry math through `Ray.h`, `Bounds.h`, `Transform.h`, `Intersections.h`, and `GeometryMath.h`;
- the core LEM mesh class with vertices, edges, loops, faces, typed handles, and element records;
- LEM editing helpers and mesh diffs for tracking geometry mutations;
- topology construction, traversal, and validation helpers for building and checking LEM structural consistency;
- adjacency, bounds, and selection hit query helpers for mesh tooling;
- render-derived mesh helpers for triangulation, normal construction, and wireframe generation;
- initial primitive and modeling operation boundaries through box creation, operation contexts/results, transform operations, and face flipping.

The next stable boundary is likely broader modeling coverage: reusable mesh mutation helpers are in place, so higher-level operations such as extrude, bevel, loop cut, and subdivision can build on them as their invariants become clear.

---

## Notes For AI-Assisted Work

When changing geometry code:

- Prefer keeping editable topology in LEM and derived data in render, query, spatial, export, or validation modules.
- Keep viewport rendering, GPU upload, object selection state, tool state, and UI interaction outside `source/kernel/`.
- Add modeling operations only after their mesh mutation invariants are clear and covered by topology validation.
- Treat triangulation, normals, wireframes, BVHs, and export meshes as rebuildable derived data.
- Prefer typed handles and explicit validation over raw pointers or implicit ownership between mesh elements.
- Update this document when files are added, renamed, or promoted from planned to implemented.
