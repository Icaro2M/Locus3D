/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file Preview.h
 * @brief Aggregator header for non-destructive operation preview support.
 *
 * This header collects preview strategies, preview execution, preview mesh
 * storage, and ghost mesh generation used by modeling operations.
 */

/*
 * Operation preview infrastructure.
 */
#include "kernel/modeling/preview/IPreviewStrategy.h"
#include "kernel/modeling/preview/OperationPreview.h"
#include "kernel/modeling/preview/PreviewMesh.h"
#include "kernel/modeling/preview/GhostMeshBuilder.h"
