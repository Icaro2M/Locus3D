/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file GraphicsRenderer.h
 * @brief Aggregator header for scene rendering and viewport presentation.
 *
 * This header collects visual material definitions, camera utilities, lighting,
 * mesh upload/cache types, render queue execution, scene objects, and viewport
 * state used by the main renderer.
 */

/*
 * Visual appearance and material configuration.
 */
#include "graphics/appearance/BuiltinVisualMaterials.h"
#include "graphics/appearance/ViewportPalette.h"
#include "graphics/appearance/VisualMaterial.h"
#include "graphics/appearance/VisualMaterialInstance.h"
#include "graphics/appearance/VisualMaterialLibrary.h"

/*
 * Camera and projection utilities.
 */
#include "graphics/camera/Camera.h"
#include "graphics/camera/CameraRayBuilder.h"
#include "graphics/camera/OrbitCameraRig.h"
#include "graphics/camera/Projection.h"

/*
 * Lighting state.
 */
#include "graphics/lighting/Light.h"
#include "graphics/lighting/LightEnvironment.h"
#include "graphics/lighting/ShadingMode.h"

/*
 * Mesh upload, GPU storage, and render cache.
 */
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshDrawData.h"
#include "graphics/mesh/MeshRenderCache.h"
#include "graphics/mesh/MeshUploadData.h"
#include "graphics/mesh/MeshUploader.h"

/*
 * Render queue, pipeline, and renderer execution.
 */
#include "graphics/renderer/DrawList.h"
#include "graphics/renderer/RenderCommand.h"
#include "graphics/renderer/RenderPipeline.h"
#include "graphics/renderer/RenderQueue.h"
#include "graphics/renderer/RenderStats.h"
#include "graphics/renderer/Renderer.h"

/*
 * Scene object representation.
 */
#include "graphics/scene/RenderLayer.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"
#include "graphics/scene/RenderTransform.h"
#include "graphics/scene/RenderVisibility.h"

/*
 * Viewport state and settings.
 */
#include "graphics/viewport/Viewport.h"
#include "graphics/viewport/ViewportSettings.h"
#include "graphics/viewport/ViewportState.h"
