/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/render/WireframeBuilder.h"
#include "kernel/modeling/core/IOperation.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/preview/OperationPreview.h"
#include "kernel/modeling/preview/PreviewMesh.h"

#include <string>
#include <utility>

namespace locus::kernel::modeling {

	/**
	 * @brief Settings used when building a preview from an editable ghost mesh.
	 */
	struct GhostMeshBuildOptions {
		/**
		 * @brief Normal mode used for shaded preview render data.
		 */
		geometry::NormalBuildMode normalMode = geometry::NormalBuildMode::Flat;

		/**
		 * @brief True when wireframe preview data should be generated.
		 */
		bool buildWireframe = true;

		/**
		 * @brief True when the copied mesh should be validated after preview execution.
		 */
		bool validateAfterPreview = false;

		/**
		 * @brief True when operations should rebuild normals on the copied mesh.
		 */
		bool rebuildNormals = true;

		/**
		 * @brief True when the preview context allows non-manifold topology.
		 */
		bool allowNonManifold = true;
	};

	/**
	 * @brief Utility that turns copied editable meshes into non-destructive previews.
	 */
	class GhostMeshBuilder {
	public:
		/**
		 * @brief Builds preview render data from an editable mesh.
		 *
		 * @param mesh Source mesh.
		 * @param options Preview build options.
		 * @return Preview mesh containing derived solid and optional wire data.
		 */
		[[nodiscard]] static PreviewMesh build_preview_mesh(
			const geometry::LEM& mesh,
			const GhostMeshBuildOptions& options = {}
		)
		{
			geometry::RenderMesh solidMesh =
				geometry::MeshTriangulator::triangulate(mesh);

			geometry::NormalBuilder::rebuild_normals(
				solidMesh,
				options.normalMode
			);

			geometry::RenderMesh wireMesh;
			if (options.buildWireframe) {
				wireMesh = geometry::WireframeBuilder::build(mesh);
			}

			return PreviewMesh{
				std::move(solidMesh),
				std::move(wireMesh)
			};
		}

		/**
		 * @brief Builds a ready operation preview from an editable mesh.
		 *
		 * @param mesh Source mesh.
		 * @param options Preview build options.
		 * @return Operation preview wrapping derived render data.
		 */
		[[nodiscard]] static OperationPreview build_preview(
			const geometry::LEM& mesh,
			const GhostMeshBuildOptions& options = {}
		)
		{
			return OperationPreview::ready(build_preview_mesh(mesh, options));
		}

		/**
		 * @brief Copies a mesh, executes an operation on the copy, and previews it.
		 *
		 * @param sourceMesh Source editable mesh that remains unchanged.
		 * @param operation Operation executed against the copied mesh.
		 * @param options Preview build options.
		 * @return Operation preview generated from the modified copy.
		 */
		[[nodiscard]] static OperationPreview build_operation_preview(
			const geometry::LEM& sourceMesh,
			IOperation& operation,
			const GhostMeshBuildOptions& options = {}
		)
		{
			geometry::LEM ghostMesh = sourceMesh;

			OperationContext context;
			context.mesh = &ghostMesh;
			context.validateAfterExecute = options.validateAfterPreview;
			context.rebuildNormals = options.rebuildNormals;
			context.allowNonManifold = options.allowNonManifold;

			OperationResult result = operation.execute(context);

			if (result.is_failure()) {
				return OperationPreview::failed(result.error().message);
			}

			if (result.status() == OperationStatus::Cancelled) {
				return OperationPreview::invalidated(result.message());
			}

			PreviewMesh previewMesh = build_preview_mesh(ghostMesh, options);
			previewMesh.set_diff(result.diff());

			if (!result.message().empty()) {
				previewMesh.set_message(result.message());
			}

			if (result.status() == OperationStatus::NoChange) {
				OperationPreview preview = OperationPreview::empty(result.message());
				preview.set_mesh(std::move(previewMesh));
				return preview;
			}

			return OperationPreview::ready(std::move(previewMesh));
		}

		/**
		 * @brief Copies a mesh and returns both the copied mesh and derived preview.
		 *
		 * @param sourceMesh Source editable mesh that remains unchanged.
		 * @param options Preview build options.
		 * @return Pair containing the copied mesh and its preview.
		 */
		[[nodiscard]] static std::pair<geometry::LEM, OperationPreview>
			build_ghost_preview(
				const geometry::LEM& sourceMesh,
				const GhostMeshBuildOptions& options = {}
			)
		{
			geometry::LEM ghostMesh = sourceMesh;
			OperationPreview preview = build_preview(ghostMesh, options);
			return { std::move(ghostMesh), std::move(preview) };
		}
	};

}