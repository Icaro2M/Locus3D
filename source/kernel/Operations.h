/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file Operations.h
 * @brief Aggregator header for implemented concrete modeling operations.
 *
 * This header collects face, edge, topology, and transform operations provided
 * by the kernel modeling layer.
 */

/*
 * Face operations.
 */
#include "kernel/modeling/operations/face/ExtrudeFaceOp.h"
#include "kernel/modeling/operations/face/InsetFaceOp.h"
#include "kernel/modeling/operations/face/FlipFaceOp.h"
#include "kernel/modeling/operations/face/SolidifyOp.h"

/*
 * Edge operations.
 */
#include "kernel/modeling/operations/edge/BevelOp.h"
#include "kernel/modeling/operations/edge/EdgeSlideOp.h"
#include "kernel/modeling/operations/edge/CreaseOp.h"

/*
 * Topology operations.
 */
#include "kernel/modeling/operations/topology/LoopCutOp.h"
#include "kernel/modeling/operations/topology/SubdivideOp.h"
#include "kernel/modeling/operations/topology/MergeVerticesOp.h"
#include "kernel/modeling/operations/topology/BridgeEdgeOp.h"
#include "kernel/modeling/operations/topology/FillHoleOp.h"

/*
 * Transform operations.
 */
#include "kernel/modeling/operations/transform/TransformOp.h"
#include "kernel/modeling/operations/transform/ShrinkFattenOp.h"
#include "kernel/modeling/operations/transform/RandomizeOp.h"
