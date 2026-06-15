/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file GraphicsPicking.h
 * @brief Aggregator header for object picking utilities.
 *
 * This header collects GPU picking identifiers, buffers, renderer support, and
 * result types used to map viewport selections back to renderable objects.
 */

/*
 * Picking buffer, identifiers, renderer, and results.
 */
#include "graphics/picking/PickingBuffer.h"
#include "graphics/picking/PickingId.h"
#include "graphics/picking/PickingRenderer.h"
#include "graphics/picking/PickingResult.h"
