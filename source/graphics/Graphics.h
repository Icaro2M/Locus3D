/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file Graphics.h
 * @brief Main aggregator header for the Locus3D graphics subsystem.
 *
 * Include this header when an application needs the complete graphics API
 * surface, including core services, GPU resources, renderer modules, overlays,
 * picking, and debug utilities.
 */

/*
 * Graphics subsystem aggregators.
 */
#include "graphics/GraphicsCore.h"
#include "graphics/GraphicsGpu.h"
#include "graphics/GraphicsRenderer.h"
#include "graphics/GraphicsOverlay.h"
#include "graphics/GraphicsPicking.h"
#include "graphics/GraphicsDebug.h"
