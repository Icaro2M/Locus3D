/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file GraphicsOverlay.h
 * @brief Aggregator header for viewport overlay renderers.
 *
 * This header collects editor visualization helpers used to draw axes, grids,
 * bounding boxes, measurements, normals, and other non-selectable overlay
 * geometry on top of the main scene.
 */

/*
 * Overlay renderer modules.
 */
#include "graphics/overlay/renderers/AxisRenderer.h"
#include "graphics/overlay/renderers/BoundingBoxRenderer.h"
#include "graphics/overlay/renderers/GizmoRenderer.h"
#include "graphics/overlay/renderers/GridRenderer.h"
#include "graphics/overlay/renderers/MeasurementRenderer.h"
#include "graphics/overlay/renderers/NormalRenderer.h"
#include "graphics/overlay/renderers/PointMarkerRenderer.h"
#include "graphics/overlay/renderers/ScreenSpaceLineRenderer.h"
#include "graphics/overlay/renderers/SurfaceOverlayRenderer.h"
