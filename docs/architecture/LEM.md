# LEM — Locus Editable Mesh

The **Locus Editable Mesh (LEM)** is the editable polygon mesh representation used by the Locus3D geometry kernel.

LEM is inspired by classic **Half-Edge** mesh structures and by Blender's **BMesh** design.

From Half-Edge, LEM adopts the idea of explicit topological traversal around polygon boundaries through directed elements.

From BMesh, LEM adopts the separation between **Vertex**, **Edge**, **Loop**, and **Face**, as well as radial traversal around edges to represent boundary and non-manifold cases.

The C++ implementation is centered around the `LEM` class.

---

## Core Elements

LEM is composed of four main topological elements:

- `Vertex`
- `Edge`
- `Loop`
- `Face`

![LEM elements](../../assets/geometry/lem-elements.svg)

---

## Vertex

A `Vertex` represents a geometric point in the mesh.

A vertex stores its position and may reference one incident edge.

Conceptually:

```txt
Vertex = point
```

A vertex does not directly own faces. Faces are reached through the mesh topology.

---

## Edge

An `Edge` represents a connection between two vertices.

Conceptually:

```txt
Edge = connection between two vertices
```

Edges in LEM are **non-directional**.

That means:

```txt
edge(v0, v1) == edge(v1, v0)
```

An edge may be used by zero, one, two, or more faces.

This allows the mesh to represent:

- loose edges;
- boundary edges;
- manifold edges;
- non-manifold edges.

---

## Face

A `Face` represents a polygonal region bounded by loops.

Conceptually:

```txt
Face = polygon
```

A face may be triangular, quadrilateral, or an n-gon.

The face stores a reference to one loop in its boundary. The full boundary is reached by walking through the face cycle.

---

## Loop

A `Loop` represents a directed corner of a face boundary.

Conceptually:

```txt
Loop = use of a vertex and an edge inside a face
```

A loop references:

- one vertex;
- one edge;
- one face;
- the next loop around the face;
- the previous loop around the face;
- the next loop around the same edge;
- the previous loop around the same edge.

The loop is the central element used to navigate both the boundary of a face and the radial structure around an edge.

---

## Face Cycle

The **face cycle** links all loops around a face boundary.

Each loop has:

```txt
next
previous
```

For a quad face, the cycle contains four loops:

```txt
Loop A → Loop B → Loop C → Loop D → Loop A
```

![LEM face cycle](../../assets/geometry/lem-face-cycle.svg)

The face cycle is used to answer questions such as:

- which loops form this face;
- which vertices form this face;
- which edges form this face;
- how many sides this face has;
- how the face can be triangulated later.

---

## Radial Cycle

The **radial cycle** links all loops that reference the same edge.

Each loop has:

```txt
radialNext
radialPrevious
```

An edge stores one loop as an entry point into this radial cycle.

![LEM radial cycle](../../assets/geometry/lem-radial-cycle.svg)

The radial cycle is used to classify edge usage:

| Loops around edge | Meaning |
|---:|---|
| 0 | Loose edge |
| 1 | Boundary edge |
| 2 | Manifold edge |
| 3 or more | Non-manifold edge |

The radial cycle is one of the main reasons LEM uses a BMesh-like structure instead of relying only on a classic Half-Edge `twin` relationship.

---

## Handles

LEM uses typed handles instead of raw pointers for internal references.

For example, a loop references related elements through handles such as:

```txt
VertexHandle
EdgeHandle
FaceHandle
LoopHandle
```

This means that a BMesh-style relationship such as:

```txt
loop has a pointer to its vertex
loop has a pointer to its edge
loop has a pointer to its face
```

is represented in LEM as:

```txt
loop has a VertexHandle
loop has an EdgeHandle
loop has a FaceHandle
```

The relationship is conceptually the same. The representation is different.

Handles are preferred because they are easier to validate, easier to serialize, and safer with vector or pool-based storage.

---

## Polygonal Faces and Triangulation

LEM stores editable polygonal faces.

A quad is stored as one quad face, not as two editable triangles.

Internal diagonals required for rendering are not part of the editable topology.

Triangulation is derived when needed for systems such as:

- viewport rendering;
- GPU buffers;
- export;
- picking or selection acceleration;
- geometric analysis.

When the editable topology changes, derived triangulation data should be rebuilt or invalidated.

---

## Topology Cases

LEM should be able to represent valid and temporarily invalid modeling states.

Important cases include:

### Loose Vertex

A vertex that is not connected to any edge.

### Loose Edge

An edge that is not used by any face.

### Boundary Edge

An edge used by exactly one face.

### Manifold Edge

An edge used by exactly two faces.

### Non-Manifold Edge

An edge used by three or more faces.

Non-manifold topology may be invalid for 3D printing, but the editable mesh must still be able to represent it so that validation and repair tools can handle it.

---

## Design Rules

The following rules define the first version of LEM:

- LEM stores editable topology, not GPU data.
- Faces are polygonal.
- Edges are non-directional.
- Loops are directed.
- A loop represents a face corner.
- Face boundaries are traversed through `next` and `previous`.
- Edge usage is traversed through `radialNext` and `radialPrevious`.
- Internal triangulation diagonals are derived data, not editable topology.
- Elements reference each other through typed handles.
- Rendering, viewport, UI selection, object naming, and global transforms belong outside LEM.

---

## Initial Scope

The first version of LEM should support:

- adding vertices;
- finding or creating edges;
- adding polygonal faces;
- creating loops for faces;
- linking face cycles;
- linking radial cycles;
- traversing loops of a face;
- building simple test meshes such as a quad and a cube.

Operations such as extrusion, bevel, inset, boolean operations, deletion, undo/redo, UVs, materials, and advanced validation should be added only after the base topology is stable.

---

## References

### BMesh

- Blender Developer Documentation — BMesh  
  https://developer.blender.org/docs/features/objects/mesh/bmesh/

- Blender Python API — BMesh Module  
  https://docs.blender.org/api/current/bmesh.html

- Blender Python API — BMesh Types  
  https://docs.blender.org/api/current/bmesh.types.html

### Half-Edge

- OpenMesh Documentation — The Halfedge Data Structure  
  https://www.graphics.rwth-aachen.de/media/openmesh_static/Documentations/OpenMesh-6.0-Documentation/a00016.html

- OpenMesh Paper — *A Generic and Efficient Polygon Mesh Data Structure*  
  https://graphics.rwth-aachen.de/media/papers/openmesh1.pdf

- CGAL Documentation — Halfedge Data Structures  
  https://graphics.stanford.edu/courses/cs368-00-spring/TA/manuals/CGAL/ref-manual2/Halfedge_DS/Chapter_hds.html
