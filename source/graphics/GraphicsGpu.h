/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file GraphicsGpu.h
 * @brief Aggregator header for GPU resource wrappers.
 *
 * This header collects low-level GPU abstractions used to create, update, and
 * bind graphics resources without exposing raw OpenGL objects to higher-level
 * renderer code.
 */

/*
 * GPU buffers, shaders, and vertex input state.
 */
#include "graphics/gpu/Buffer.h"
#include "graphics/gpu/Shader.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/gpu/VertexArray.h"
