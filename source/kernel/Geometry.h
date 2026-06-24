/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file Geometry.h
 * @brief Aggregator header for the public kernel geometry layer.
 *
 * This header collects editable mesh types, topology utilities, spatial
 * queries, derived render mesh generation, primitive builders, and geometry
 * query helpers.
 */

/*
 * Editable mesh API.
 */
#include "kernel/EditableMesh.h"

/*
 * Topology utilities.
 */
#include "kernel/geometry/topology/TopologyBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"

/*
 * Spatial indexing and acceleration.
 */
#include "kernel/geometry/spatial/SpatialIndex.h"
#include "kernel/geometry/spatial/BVH.h"
#include "kernel/geometry/spatial/BVHBuilder.h"
#include "kernel/geometry/spatial/BVHQuery.h"

/*
 * Derived render geometry.
 */
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/render/WireframeBuilder.h"

/*
 * Primitive builders and registry.
 */
#include "kernel/geometry/primitives/IPrimitiveBuilder.h"
#include "kernel/geometry/primitives/PrimitiveParameters.h"
#include "kernel/geometry/primitives/BoxBuilder.h"
#include "kernel/geometry/primitives/CylinderBuilder.h"
#include "kernel/geometry/primitives/SphereBuilder.h"
#include "kernel/geometry/primitives/ConeBuilder.h"
#include "kernel/geometry/primitives/TorusBuilder.h"
#include "kernel/geometry/primitives/PrimitiveRegistry.h"

/*
 * Geometry queries.
 */
#include "kernel/geometry/queries/RaycastQuery.h"
#include "kernel/geometry/queries/AdjacencyQuery.h"
#include "kernel/geometry/queries/BoundsQuery.h"
#include "kernel/geometry/queries/SelectionQuery.h"
#include "kernel/geometry/queries/SelectionHit.h"
#include "kernel/geometry/queries/ProximityQuery.h"
