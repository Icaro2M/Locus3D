/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

 /**
  * @file GraphicsPrimitives.h
  * @brief Aggregator header for generic CPU-side graphical primitives.
  *
  * This header collects the types used to construct points, lines, triangles,
  * and other simple graphical geometry before conversion into mesh upload
  * payloads.
  *
  * The primitives module does not own GPU resources and carries no editor,
  * scene, selection, or kernel-specific semantics.
  */

  /*
   * Primitive geometry representation and construction.
   */
#include "graphics/primitives/PrimitiveVertex.h"
#include "graphics/primitives/ObjectHighlight.h"
#include "graphics/primitives/PrimitiveMesh.h"
#include "graphics/primitives/PrimitiveBuilder.h"
#include "graphics/primitives/PointMarker.h"
#include "graphics/primitives/ScreenSpaceLine.h"
#include "graphics/primitives/SurfaceOverlay.h"

   /*
	* Conversion into the standard graphics upload representation.
	*/
#include "graphics/primitives/PrimitiveMeshConverter.h"
