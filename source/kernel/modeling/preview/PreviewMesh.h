/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/render/RenderMesh.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace locus::kernel::modeling {

	/**
	 * @brief Non-editable mesh payload generated for operation previews.
	 *
	 * PreviewMesh stores derived render data only. It is intended for transient
	 * visualization and must not be treated as authoritative editable topology.
	 */
	class PreviewMesh {
	public:
		/**
		 * @brief Creates an empty valid preview mesh.
		 */
		PreviewMesh() = default;

		/**
		 * @brief Creates a preview mesh from solid and wire render data.
		 *
		 * @param solidMesh Triangle render mesh used for shaded preview drawing.
		 * @param wireMesh Line render mesh used for wire overlay drawing.
		 */
		PreviewMesh(
			geometry::RenderMesh solidMesh,
			geometry::RenderMesh wireMesh = {}
		)
			: solidMesh_(std::move(solidMesh))
			, wireMesh_(std::move(wireMesh))
		{
		}

		/**
		 * @brief Creates a preview mesh from solid, wire, and diff data.
		 *
		 * @param solidMesh Triangle render mesh used for shaded preview drawing.
		 * @param wireMesh Line render mesh used for wire overlay drawing.
		 * @param diff Mesh diff produced while building the preview.
		 */
		PreviewMesh(
			geometry::RenderMesh solidMesh,
			geometry::RenderMesh wireMesh,
			geometry::LEMDiff diff
		)
			: solidMesh_(std::move(solidMesh))
			, wireMesh_(std::move(wireMesh))
			, diff_(std::move(diff))
		{
		}

		/**
		 * @brief Checks whether no preview geometry is available.
		 *
		 * @return True when both solid and wire meshes are empty.
		 */
		[[nodiscard]] bool empty() const
		{
			return solidMesh_.empty() && wireMesh_.empty();
		}

		/**
		 * @brief Checks whether the preview payload is valid.
		 *
		 * @return True when the preview can be consumed by higher layers.
		 */
		[[nodiscard]] bool valid() const
		{
			return valid_;
		}

		/**
		 * @brief Sets whether the preview payload is valid.
		 *
		 * @param valid New validity state.
		 */
		void set_valid(bool valid)
		{
			valid_ = valid;
		}

		/**
		 * @brief Returns the shaded render mesh.
		 *
		 * @return Read-only solid mesh.
		 */
		[[nodiscard]] const geometry::RenderMesh& solid_mesh() const
		{
			return solidMesh_;
		}

		/**
		 * @brief Returns the shaded render mesh.
		 *
		 * @return Mutable solid mesh.
		 */
		[[nodiscard]] geometry::RenderMesh& solid_mesh()
		{
			return solidMesh_;
		}

		/**
		 * @brief Replaces the shaded render mesh.
		 *
		 * @param mesh New solid mesh.
		 */
		void set_solid_mesh(geometry::RenderMesh mesh)
		{
			solidMesh_ = std::move(mesh);
		}

		/**
		 * @brief Returns the wire render mesh.
		 *
		 * @return Read-only wire mesh.
		 */
		[[nodiscard]] const geometry::RenderMesh& wire_mesh() const
		{
			return wireMesh_;
		}

		/**
		 * @brief Returns the wire render mesh.
		 *
		 * @return Mutable wire mesh.
		 */
		[[nodiscard]] geometry::RenderMesh& wire_mesh()
		{
			return wireMesh_;
		}

		/**
		 * @brief Replaces the wire render mesh.
		 *
		 * @param mesh New wire mesh.
		 */
		void set_wire_mesh(geometry::RenderMesh mesh)
		{
			wireMesh_ = std::move(mesh);
		}

		/**
		 * @brief Returns the diff produced by preview construction.
		 *
		 * @return Read-only diff.
		 */
		[[nodiscard]] const geometry::LEMDiff& diff() const
		{
			return diff_;
		}

		/**
		 * @brief Returns the diff produced by preview construction.
		 *
		 * @return Mutable diff.
		 */
		[[nodiscard]] geometry::LEMDiff& diff()
		{
			return diff_;
		}

		/**
		 * @brief Replaces the preview diff.
		 *
		 * @param diff New diff.
		 */
		void set_diff(geometry::LEMDiff diff)
		{
			diff_ = std::move(diff);
		}

		/**
		 * @brief Checks whether preview construction produced mesh changes.
		 *
		 * @return True when the stored diff is not empty.
		 */
		[[nodiscard]] bool changed() const
		{
			return !diff_.empty();
		}

		/**
		 * @brief Returns the optional status message.
		 *
		 * @return Human-readable status message.
		 */
		[[nodiscard]] const std::string& message() const
		{
			return message_;
		}

		/**
		 * @brief Replaces the status message.
		 *
		 * @param message New message.
		 */
		void set_message(std::string message)
		{
			message_ = std::move(message);
		}

		/**
		 * @brief Returns all non-fatal preview warnings.
		 *
		 * @return Read-only warning list.
		 */
		[[nodiscard]] const std::vector<std::string>& warnings() const
		{
			return warnings_;
		}

		/**
		 * @brief Adds a non-fatal warning.
		 *
		 * @param warning Warning text.
		 */
		void add_warning(std::string warning)
		{
			warnings_.push_back(std::move(warning));
		}

		/**
		 * @brief Removes all warning messages.
		 */
		void clear_warnings()
		{
			warnings_.clear();
		}

		/**
		 * @brief Returns the number of shaded preview vertices.
		 *
		 * @return Solid vertex count.
		 */
		[[nodiscard]] std::size_t solid_vertex_count() const
		{
			return solidMesh_.vertex_count();
		}

		/**
		 * @brief Returns the number of shaded preview triangles.
		 *
		 * @return Solid triangle count.
		 */
		[[nodiscard]] std::size_t solid_triangle_count() const
		{
			return solidMesh_.triangle_count();
		}

		/**
		 * @brief Returns the number of wire preview vertices.
		 *
		 * @return Wire vertex count.
		 */
		[[nodiscard]] std::size_t wire_vertex_count() const
		{
			return wireMesh_.vertex_count();
		}

		/**
		 * @brief Returns the number of wire preview lines.
		 *
		 * @return Wire line count.
		 */
		[[nodiscard]] std::size_t wire_line_count() const
		{
			return wireMesh_.line_count();
		}

		/**
		 * @brief Clears all preview data.
		 */
		void clear()
		{
			solidMesh_.clear();
			wireMesh_.clear();
			diff_.clear();
			message_.clear();
			warnings_.clear();
			valid_ = true;
		}

	private:
		geometry::RenderMesh solidMesh_{};
		geometry::RenderMesh wireMesh_{};
		geometry::LEMDiff diff_{};
		std::string message_{};
		std::vector<std::string> warnings_{};
		bool valid_ = true;
	};

}