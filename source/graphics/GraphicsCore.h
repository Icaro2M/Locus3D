/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file GraphicsCore.h
 * @brief Aggregator header for graphics core services and window integration.
 *
 * This header collects foundational graphics types, result/error handling,
 * device/context access, and window abstractions used by the rest of the
 * graphics subsystem.
 */

/*
 * Common graphics types and result handling.
 */
#include "graphics/common/GraphicsConfig.h"
#include "graphics/common/GraphicsError.h"
#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"

/*
 * Graphics context and device access.
 */
#include "graphics/context/GraphicsCapabilities.h"
#include "graphics/context/GraphicsContext.h"
#include "graphics/context/GraphicsDevice.h"
#include "graphics/context/OpenGLContext.h"

/*
 * Window and input integration.
 */
#include "graphics/window/Cursor.h"
#include "graphics/window/Window.h"
#include "graphics/window/WindowEvents.h"
