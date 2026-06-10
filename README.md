# Geometric Modeling — Half-Edge Mesh Project

A geometric modeling project using a **half-edge** data structure to represent and manipulate 3D meshes.

## Implemented Features

---

### 1. readFile — OBJ File Loading

**File**: `myMesh.cpp` — `myMesh::readFile()`

Reads `.obj` files and builds the half-edge structure:

- **Vertices (`v`)**: Reads (x, y, z) coordinates, creates a `myVertex` with its `myPoint3D`
- **Faces (`f`)**: Reads vertex indices (supports `v/vt/vn` format), creates halfedges with:
  - `next` / `prev` connections (circular loop around the face)
  - `adjacent_face` and `source` assignment
  - **Twin search** via a `map<pair<int,int>, myHalfedge*>` keyed by (source, destination) pairs
  - `originof` assignment for each vertex

Degenerate faces (< 3 vertices) are ignored. After loading, `checkMesh()` verifies integrity and `normalize()` centers and scales the mesh.

---

### 2. Compute Normals

**Files**: `myMesh.cpp`, `myFace.cpp`, `myVertex.cpp`

- **Per-face normals** (`myFace::computeNormal()`): Cross product of the first two edges of the face, then normalization. Uses `myVector3D::setNormal()`.
- **Per-vertex normals** (`myVertex::computeNormal()`): Average of adjacent face normals, obtained by circulating around the vertex via `twin->next`. Handles edge cases:
  - `originof == NULL`: isolated vertex (not referenced by any face) --> skip
  - `twin == NULL`: boundary vertex (open mesh) --> stop the loop

---

### 3. Silhouette

**File**: `main.cpp` — `display()` function

Real-time silhouette edge detection. For each halfedge with a twin:

1. Compute the edge midpoint
2. Compute the view vector `q` (midpoint --> camera)
3. Dot product of `q` with the normals of both adjacent faces (`d1` and `d2`)
4. If `d1 * d2 < 0` --> the two faces are oriented differently relative to the camera --> **silhouette edge**

Silhouette edges are drawn in red with a line width of 4px and update dynamically with camera rotation.

---

### 4. Triangulation

**File**: `myMesh.cpp`

#### Convex faces (basic)

`myMesh::triangulate()` iteratively splits non-triangular faces using a fan approach: it detaches the first two halfedges of the face to form a new triangle, and repeats until the remaining face is a triangle.

For each split:
- Creation of 2 new halfedges (twins of each other) forming the diagonal
- Creation of a new face
- Update of all `next`, `prev`, `adjacent_face` pointers

#### Concave faces (advanced)

`myMesh::triangulate(myFace *f)` uses the **ear-clipping** algorithm:

1. **Normal computation** using Newell's method (robust for concave and non-planar polygons)
2. **Ear search** — iterates over each vertex to test:
   - **Convexity**: the cross product `AB × BC` points in the same direction as the face normal
   - **No interior vertex**: no other polygon vertex lies inside the candidate triangle (point-in-triangle test using cross products projected onto the normal)
3. **Positioning**: `f->adjacent_halfedge` is set to the found ear so the splitting code cuts at the right location
4. **Fallback**: if no ear is found (degenerate polygon), cuts at the starting point

Successfully tested on `gear.obj` and `c_gear.obj`.

---

### 5. Half-Edge Data Structure Tests

**File**: `myMesh.cpp` — `myMesh::checkMesh()`

Comprehensive half-edge structure integrity verification with 6 tests:

| # | Test | Verifies |
|---|------|----------|
| 1 | Twin symmetry | `twin != NULL` and `twin->twin == he` |
| 2 | Next/Prev consistency | `next->prev == he` and `prev->next == he` |
| 3 | Face loop | Following `next` returns to start (max 1000 iterations) |
| 4 | Twin-source | `twin->source == next->source` |
| 5 | Face back-pointer | `face->adjacent_halfedge->adjacent_face == face` and all halfedges of the face point to it |
| 6 | Vertex back-pointer | `vertex->originof->source == vertex` (warning if `originof == NULL`) |

Output printed to console: `"Mesh check passed! All tests OK"` or a detailed error report. Called automatically after `readFile()` and `generateSurfaceOfRevolution()`.

---

### 6. Surface of Revolution

**File**: `myMesh.cpp` — `myMesh::generateSurfaceOfRevolution()`

Mesh generation by revolving a 2D profile curve around the Y axis:

1. **Input**: profile (points in the XZ plane, `X` = radius, `Z` = height) + number of slices
2. **Vertices**: `n × slices` vertices created by rotating each profile point by `2π × j / slices`
3. **Faces**: `(n-1) × slices` quads with full half-edge connectivity (next, prev, source, adjacent_face, twins via `twin_map`)
4. **Output**: normalized mesh verified via `checkMesh()`

The profile is read from a `profile.txt` file (format: first line = number of slices, then `radius height` per line). Accessible via right-click --> **Generate Mesh**.

Example profiles provided in `profiles/`:

| File | Shape |
|------|-------|
| `sphere.txt` | Sphere |
| `cylinder.txt` | Cylinder |
| `cone.txt` | Cone |
| `torus.txt` | Torus |

---

### 7. Mesh Simplification — Shortest Edge Collapse

**File**: `myMesh.cpp`

Progressive mesh simplification by collapsing the shortest edge:

- **`simplify()`**: Finds the globally shortest edge, then calls `simplify(vertex)` on its endpoint
- **`simplify(myVertex *v)`**: Finds the shortest edge around the given vertex, then performs the collapse:

1. **Merge**: moves `v_keep` to the edge midpoint
2. **Redirect**: all halfedges originating from `v_remove` now point to `v_keep`
3. **Face removal**: the 2 adjacent triangles are removed (6 halfedges, 2 faces)
4. **Twin reconnection**: the side halfedges of the removed triangles become twins of each other
5. **Fix `originof`**: ensures no vertex points to a deleted halfedge
6. **Cleanup**: removal from vectors and memory deallocation

Accessible via right-click --> **Simplification** (each click collapses one edge). Requires a **triangulated** mesh.
