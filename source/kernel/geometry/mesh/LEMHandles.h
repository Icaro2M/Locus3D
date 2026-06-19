/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Handle.h"

namespace locus::kernel::geometry {

	/**
	 * @brief Tag type used to identify vertex handles.
	 */
	struct VertexTag {};

	/**
	 * @brief Tag type used to identify edge handles.
	 */
	struct EdgeTag {};

	/**
	 * @brief Tag type used to identify loop handles.
	 */
	struct LoopTag {};

	/**
	 * @brief Tag type used to identify face handles.
	 */
	struct FaceTag {};

	/**
	 * @brief Type-safe handle referencing a vertex element.
	 */
	using VertexHandle = Handle<VertexTag>;

	/**
	 * @brief Type-safe handle referencing an edge element.
	 */
	using EdgeHandle = Handle<EdgeTag>;

	/**
	 * @brief Type-safe handle referencing a loop element.
	 */
	using LoopHandle = Handle<LoopTag>;

	/**
	 * @brief Type-safe handle referencing a face element.
	 */
	using FaceHandle = Handle<FaceTag>;

}