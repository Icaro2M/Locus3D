/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file EditableMesh.h
 * @brief Aggregator header for the editable LEM mesh and direct mesh editors.
 *
 * This header collects the editable mesh core, handles, storage, diff support,
 * mesh elements, direct editors, and low-level editing modules.
 */

/*
 * Editable mesh core.
 */
#include "kernel/geometry/mesh/LEMTypes.h"
#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMStorage.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/mesh/LEMDiff.h"

/*
 * Editable mesh elements.
 */
#include "kernel/geometry/mesh/elements/Vertex.h"
#include "kernel/geometry/mesh/elements/Edge.h"
#include "kernel/geometry/mesh/elements/Loop.h"
#include "kernel/geometry/mesh/elements/Face.h"

/*
 * Direct mesh editors.
 */
#include "kernel/geometry/mesh/editing/TopologyEditor.h"
#include "kernel/geometry/mesh/editing/GeometryEditor.h"
#include "kernel/geometry/mesh/editing/AttributeEditor.h"

/*
 * Topology editing modules.
 */
#include "kernel/geometry/mesh/editing/topology/TopologyCreation.h"
#include "kernel/geometry/mesh/editing/topology/TopologyRemoval.h"
#include "kernel/geometry/mesh/editing/topology/TopologySplit.h"
#include "kernel/geometry/mesh/editing/topology/TopologyCollapse.h"
#include "kernel/geometry/mesh/editing/topology/TopologyFlip.h"
#include "kernel/geometry/mesh/editing/topology/TopologyRelink.h"

/*
 * Geometry editing modules.
 */
#include "kernel/geometry/mesh/editing/geometry/GeometryPosition.h"
#include "kernel/geometry/mesh/editing/geometry/GeometryTransform.h"
#include "kernel/geometry/mesh/editing/geometry/GeometryNormals.h"

/*
 * Attribute editing modules.
 */
#include "kernel/geometry/mesh/editing/attributes/AttributeSelection.h"
#include "kernel/geometry/mesh/editing/attributes/AttributeVisibility.h"
#include "kernel/geometry/mesh/editing/attributes/AttributeShading.h"
#include "kernel/geometry/mesh/editing/attributes/AttributeTags.h"
