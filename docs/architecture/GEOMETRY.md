# Locus3D Architecture

This document contains high-level architecture notes for the geometry production code in `source/kernel/`.

The geometry layer is currently in an early production stage. Its first stable pieces are the editable mesh core, topology traversal/validation helpers, render-mesh derivation utilities, and shared kernel math/common types.

For the editable mesh design, see [`LEM.md`](LEM.md).

---

## Geometry Layer

The geometry layer is the modeling kernel foundation. It owns editable mesh data structures, topology utilities, geometry math, spatial queries, modeling operations, import/export boundaries, internal validation, and planned manufacturing analysis for printable geometry.

It should not own high-level editor state. Systems such as tools, UI panels, command history, scene/object management, selection workflows, snapping state, and viewport interaction should live above this layer and call into it through explicit geometry/modeling APIs.

Conceptually:

```txt
application/editor/tools
        |
        v
source/kernel modeling, validation, and planned manufacturing APIs
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
- `validation/`: internal kernel validation pipeline, reports, and checks for LEM/topology/geometric consistency. This module protects editable-mesh invariants and supports modeling tools; it is not the user-facing printability analyzer.
- `manufacturing/`: planned manufacturing and printability analysis module. This module will evaluate whether geometry is suitable for 3D printing under a print profile, including manifoldness, watertightness, thin walls, overhangs, islands, self-intersections, and similar fabrication concerns. It should be implemented after the geometry layer is stable.

### Validation And Manufacturing Boundary

`validation/` is an internal correctness module. It should answer whether the editable mesh and related kernel data are structurally coherent enough for tools, operations, derived data builders, import/export, and debugging.

`manufacturing/` is a planned fabrication-analysis module. It should answer whether a mesh is suitable for 3D printing under an explicit print profile. This module is intentionally deferred until the core geometry layer has stable topology, triangulation, spatial queries, and modeling invariants.

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
|       Vec.h
|       Mat.h
|       Ray.h
|       Bounds.h
|       Transform.h
|       Quaternion.h
|       Intersections.h
|       GeometryMath.h
|
+---geometry
|   +---mesh
|   |   |   LEMTypes.h
|   |   |   LEMHandles.h
|   |   |   LEM.h
|   |   |   LEM.cpp
|   |   |   LEMStorage.h
|   |   |   LEMEditor.h
|   |   |   LEMEditor.cpp
|   |   |   LEMDiff.h
|   |   |
|   |   +---editing
|   |   |   |   TopologyEditor.h
|   |   |   |   TopologyEditor.cpp
|   |   |   |   GeometryEditor.h
|   |   |   |   GeometryEditor.cpp
|   |   |   |   AttributeEditor.h
|   |   |   |   AttributeEditor.cpp
|   |   |   |
|   |   |   +---topology
|   |   |   |       TopologyCreation.h
|   |   |   |       TopologyCreation.cpp
|   |   |   |       TopologyRemoval.h
|   |   |   |       TopologyRemoval.cpp
|   |   |   |       TopologySplit.h
|   |   |   |       TopologySplit.cpp
|   |   |   |       TopologyCollapse.h
|   |   |   |       TopologyCollapse.cpp
|   |   |   |       TopologyFlip.h
|   |   |   |       TopologyFlip.cpp
|   |   |   |       TopologyRelink.h
|   |   |   |       TopologyRelink.cpp
|   |   |   |
|   |   |   +---geometry
|   |   |   |       GeometryPosition.h
|   |   |   |       GeometryPosition.cpp
|   |   |   |       GeometryTransform.h
|   |   |   |       GeometryTransform.cpp
|   |   |   |       GeometryNormals.h
|   |   |   |       GeometryNormals.cpp
|   |   |   |
|   |   |   \---attributes
|   |   |           AttributeSelection.h
|   |   |           AttributeSelection.cpp
|   |   |           AttributeVisibility.h
|   |   |           AttributeVisibility.cpp
|   |   |           AttributeShading.h
|   |   |           AttributeShading.cpp
|   |   |           AttributeTags.h
|   |   |           AttributeTags.cpp
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
|   |       SpatialIndex.h
|   |       BVH.h
|   |       BVH.cpp
|   |       BVHBuilder.h
|   |       BVHQuery.h
|   |
|   +---render
|   |       RenderMesh.h
|   |       MeshTriangulator.h
|   |       NormalBuilder.h
|   |       WireframeBuilder.h
|   |
|   +---primitives
|   |       IPrimitiveBuilder.h
|   |       PrimitiveParameters.h
|   |       BoxBuilder.h
|   |       CylinderBuilder.h
|   |       SphereBuilder.h
|   |       ConeBuilder.h
|   |       TorusBuilder.h
|   |       PrimitiveRegistry.h
|   |
|   \---queries
|           RaycastQuery.h
|           AdjacencyQuery.h
|           BoundsQuery.h
|           SelectionQuery.h
|           SelectionHit.h
|           ProximityQuery.h
|
+---modeling
|   +---core
|   |       IOperation.h
|   |       OperationContext.h
|   |       OperationResult.h
|   |
|   +---operations
|   |   +---face
|   |   |       ExtrudeFaceOp.h
|   |   |       ExtrudeFaceOp.cpp
|   |   |       InsetFaceOp.h
|   |   |       InsetFaceOp.cpp
|   |   |       FlipFaceOp.h
|   |   |       FlipFaceOp.cpp
|   |   |       SolidifyOp.h [!]
|   |   |
|   |   +---edge
|   |   |       BevelOp.h [!]
|   |   |       BevelOp.cpp [!]
|   |   |       EdgeSlideOp.h [!]
|   |   |       CreaseOp.h
|   |   |       CreaseOp.cpp
|   |   |
|   |   +---topology
|   |   |       LoopCutOp.h [!]
|   |   |       LoopCutOp.cpp [!]
|   |   |       SubdivideOp.h
|   |   |       SubdivideOp.cpp
|   |   |       MergeVerticesOp.h
|   |   |       MergeVerticesOp.cpp
|   |   |       BridgeEdgeOp.h [!]
|   |   |       FillHoleOp.h [!]
|   |   |
|   |   \---transform
|   |           TransformOp.h
|   |           TransformOp.cpp
|   |           ShrinkFattenOp.h
|   |           ShrinkFattenOp.cpp
|   |           RandomizeOp.h
|   |           RandomizeOp.cpp
|   |
|   \---preview
|           IPreviewStrategy.h [!]
|           OperationPreview.h [!]
|           PreviewMesh.h [!]
|           GhostMeshBuilder.h [!]
|
+---io
|       IExporter.h
|       IImporter.h
|       StlExporter.h
|       StlImporter.h
|       ObjExporter.h
|       ObjImporter.h
|       ThreeMFExporter.h [!]
|       FormatRegistry.h
|
+---validation
|   |       Validation.h
|   |       ValidationChecks.h
|   |       ValidationCore.h
|   | 
|   +---core
|   |       ValidationContext.h
|   |       ValidationReport.h
|   |       ValidationIssue.h
|   |       ValidationSeverity.h
|   |       IValidationCheck.h
|   |
|   +---checks
|   |   +---lem
|   |   |       HandleValidityCheck.h
|   |   |       FaceCycleCheck.h
|   |   |       RadialCycleCheck.h
|   |   |       ElementReferenceCheck.h
|   |   |
|   |   +---topology
|   |   |       TopologyCacheCheck.h [!]
|   |   |       ConnectivityConsistencyCheck.h
|   |   |
|   |   \---geometry
|   |           InvalidPositionCheck.h
|   |           DegenerateEditableFaceCheck.h
|   |
|   \---pipeline
|           ValidationPipeline.h
|           ValidationMode.h
|
\---manufacturing [!]
    +---core
    |       IAnalyzer.h [!]
    |       AnalysisContext.h [!]
    |       AnalysisReport.h [!]
    |       PrintIssue.h [!]
    |       PrintIssueType.h [!]
    |       IssueSeverity.h [!]
    |
    +---profiles
    |       PrintProfile.h [!]
    |       PrintTechnology.h [!]
    |       FDMProfile.h [!]
    |       SLAProfile.h [!]
    |       SLSProfile.h [!]
    |       ProfileRegistry.h [!]
    |
    +---mesh
    |       AnalysisMesh.h [!]
    |       AnalysisMeshBuilder.h [!]
    |       MeshHandleMapping.h [!]
    |
    +---analyzers
    |   +---topology
    |   |       ManifoldAnalyzer.h [!]
    |   |       WatertightAnalyzer.h [!]
    |   |       NormalConsistencyAnalyzer.h [!]
    |   |       IslandAnalyzer.h [!]
    |   |
    |   +---geometry
    |   |       DegenerateGeometryAnalyzer.h [!]
    |   |       SelfIntersectionAnalyzer.h [!]
    |   |
    |   +---thinwall
    |   |       IThinWallAnalyzer.h [!]
    |   |       ThinWallQuality.h [!]
    |   |       ThinWallAnalyzerFactory.h [!]
    |   |       CurvatureApproxAnalyzer.h [!]
    |   |       RaycastThinWallAnalyzer.h [!]
    |   |       ComputeThinWallAnalyzer.h [!]
    |   |
    |   \---process
    |           OverhangAnalyzer.h [!]
    |           MinimumFeatureSizeAnalyzer.h [!]
    |           SupportRequirementAnalyzer.h [!]
    |           VolumeAnalyzer.h [!]
    |
    \---pipeline
            AnalysisPipeline.h [!]
            AnalysisScheduler.h [!]
```

---

## Planned Or Incomplete Geometry Files

The files marked with `[!]` in the tree are planned or incomplete parts of the geometry layer that are not present in the current `source/kernel/` tree yet.

- [!] `modeling/operations/face/SolidifyOp.h`
- [!] `modeling/operations/edge/BevelOp.h`
- [!] `modeling/operations/edge/BevelOp.cpp`
- [!] `modeling/operations/edge/EdgeSlideOp.h`
- [!] `modeling/operations/topology/LoopCutOp.h`
- [!] `modeling/operations/topology/LoopCutOp.cpp`
- [!] `modeling/operations/topology/BridgeEdgeOp.h`
- [!] `modeling/operations/topology/FillHoleOp.h`
- [!] `modeling/preview/IPreviewStrategy.h`
- [!] `modeling/preview/OperationPreview.h`
- [!] `modeling/preview/PreviewMesh.h`
- [!] `modeling/preview/GhostMeshBuilder.h`
- [!] `io/ThreeMFExporter.h`
- [!] `validation/checks/topology/TopologyCacheCheck.h`
- [!] `manufacturing/core/IAnalyzer.h`
- [!] `manufacturing/core/AnalysisContext.h`
- [!] `manufacturing/core/AnalysisReport.h`
- [!] `manufacturing/core/PrintIssue.h`
- [!] `manufacturing/core/PrintIssueType.h`
- [!] `manufacturing/core/IssueSeverity.h`
- [!] `manufacturing/profiles/PrintProfile.h`
- [!] `manufacturing/profiles/PrintTechnology.h`
- [!] `manufacturing/profiles/FDMProfile.h`
- [!] `manufacturing/profiles/SLAProfile.h`
- [!] `manufacturing/profiles/SLSProfile.h`
- [!] `manufacturing/profiles/ProfileRegistry.h`
- [!] `manufacturing/mesh/AnalysisMesh.h`
- [!] `manufacturing/mesh/AnalysisMeshBuilder.h`
- [!] `manufacturing/mesh/MeshHandleMapping.h`
- [!] `manufacturing/analyzers/topology/ManifoldAnalyzer.h`
- [!] `manufacturing/analyzers/topology/WatertightAnalyzer.h`
- [!] `manufacturing/analyzers/topology/NormalConsistencyAnalyzer.h`
- [!] `manufacturing/analyzers/topology/IslandAnalyzer.h`
- [!] `manufacturing/analyzers/geometry/DegenerateGeometryAnalyzer.h`
- [!] `manufacturing/analyzers/geometry/SelfIntersectionAnalyzer.h`
- [!] `manufacturing/analyzers/thinwall/IThinWallAnalyzer.h`
- [!] `manufacturing/analyzers/thinwall/ThinWallQuality.h`
- [!] `manufacturing/analyzers/thinwall/ThinWallAnalyzerFactory.h`
- [!] `manufacturing/analyzers/thinwall/CurvatureApproxAnalyzer.h`
- [!] `manufacturing/analyzers/thinwall/RaycastThinWallAnalyzer.h`
- [!] `manufacturing/analyzers/thinwall/ComputeThinWallAnalyzer.h`
- [!] `manufacturing/analyzers/process/OverhangAnalyzer.h`
- [!] `manufacturing/analyzers/process/MinimumFeatureSizeAnalyzer.h`
- [!] `manufacturing/analyzers/process/SupportRequirementAnalyzer.h`
- [!] `manufacturing/analyzers/process/VolumeAnalyzer.h`
- [!] `manufacturing/pipeline/AnalysisPipeline.h`
- [!] `manufacturing/pipeline/AnalysisScheduler.h`

These should be added as their owning boundaries become clear. In particular, modeling operations should depend on stable LEM editing primitives and explicit operation contexts. Internal validation should stay focused on mesh/kernel invariants, while manufacturing analysis should stay independent from editor UI and viewport state and should be implemented after the geometry layer is stable enough to provide reliable derived analysis meshes.

---

## Current Implemented Foundation

The existing kernel code already provides:

- typed kernel/common utilities through `Id.h`, `Error.h`, `Result.h`, and `Types.h`;
- basic geometry math through `Ray.h`, `Bounds.h`, `Transform.h`, `Intersections.h`, and `GeometryMath.h`;
- the core LEM mesh class with vertices, edges, loops, faces, typed handles, and element records;
- LEM editing helpers and mesh diffs for tracking geometry mutations;
- topology construction, traversal, and validation helpers for building and checking LEM structural consistency;
- raycast, proximity, adjacency, bounds, and selection hit query helpers for mesh tooling;
- BVH spatial acceleration for face raycasts and bounds overlap queries;
- render-derived mesh helpers for triangulation, normal construction, and wireframe generation;
- built-in primitive creation for boxes, cylinders, spheres, cones, and tori through a shared primitive registry;
- initial modeling operation boundaries through operation contexts/results, transform operations, and face flipping.

The next stable boundary is likely broader modeling coverage: reusable mesh mutation helpers are in place, so higher-level operations such as extrude, bevel, loop cut, and subdivision can build on them as their invariants become clear.

---

## Notes For AI-Assisted Work

When changing geometry code:

- Prefer keeping editable topology in LEM and derived data in render, query, spatial, export, or validation modules.
- Keep viewport rendering, GPU upload, object selection state, tool state, and UI interaction outside `source/kernel/`.
- Add modeling operations only after their mesh mutation invariants are clear and covered by topology validation.
- Treat triangulation, normals, wireframes, BVHs, export meshes, and future manufacturing analysis meshes as rebuildable derived data.
- Keep `validation/` focused on internal correctness and tool support; do not add user-facing printability policy there.
- Keep `manufacturing/` focused on fabrication analysis under print profiles; implement it after the geometry layer has stable topology, triangulation, spatial queries, and modeling invariants.
- Prefer typed handles and explicit validation over raw pointers or implicit ownership between mesh elements.
- Update this document when files are added, renamed, or promoted from planned to implemented.
