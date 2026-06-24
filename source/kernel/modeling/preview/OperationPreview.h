/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/modeling/preview/PreviewMesh.h"

#include <string>
#include <utility>

namespace locus::kernel::modeling {

	/**
	 * @brief High-level state of an operation preview.
	 */
	enum class OperationPreviewStatus {
		/**
		 * @brief Preview was generated and can be displayed.
		 */
		Ready,

		/**
		 * @brief Preview was generated but contains no displayable geometry.
		 */
		Empty,

		/**
		 * @brief Preview is stale and should be rebuilt before display.
		 */
		Invalidated,

		/**
		 * @brief Preview generation failed.
		 */
		Failed
	};

	/**
	 * @brief Result object returned by preview strategies and ghost builders.
	 */
	class OperationPreview {
	public:
		/**
		 * @brief Creates an empty preview object.
		 */
		OperationPreview() = default;

		/**
		 * @brief Creates a ready preview.
		 *
		 * @param mesh Preview mesh payload.
		 * @return Ready operation preview.
		 */
		[[nodiscard]] static OperationPreview ready(PreviewMesh mesh)
		{
			OperationPreview preview;
			preview.status_ = mesh.empty()
				? OperationPreviewStatus::Empty
				: OperationPreviewStatus::Ready;
			preview.mesh_ = std::move(mesh);
			return preview;
		}

		/**
		 * @brief Creates an empty preview result.
		 *
		 * @param message Optional reason.
		 * @return Empty operation preview.
		 */
		[[nodiscard]] static OperationPreview empty(std::string message = {})
		{
			OperationPreview preview;
			preview.status_ = OperationPreviewStatus::Empty;
			preview.message_ = std::move(message);
			return preview;
		}

		/**
		 * @brief Creates an invalidated preview result.
		 *
		 * @param message Optional reason.
		 * @return Invalidated operation preview.
		 */
		[[nodiscard]] static OperationPreview invalidated(std::string message = {})
		{
			OperationPreview preview;
			preview.status_ = OperationPreviewStatus::Invalidated;
			preview.message_ = std::move(message);
			return preview;
		}

		/**
		 * @brief Creates a failed preview result.
		 *
		 * @param message Failure reason.
		 * @return Failed operation preview.
		 */
		[[nodiscard]] static OperationPreview failed(std::string message = {})
		{
			OperationPreview preview;
			preview.status_ = OperationPreviewStatus::Failed;
			preview.message_ = std::move(message);
			preview.mesh_.set_valid(false);
			return preview;
		}

		/**
		 * @brief Checks whether the preview is ready for display.
		 *
		 * @return True when the status is Ready.
		 */
		[[nodiscard]] bool is_ready() const
		{
			return status_ == OperationPreviewStatus::Ready;
		}

		/**
		 * @brief Checks whether preview generation failed.
		 *
		 * @return True when the status is Failed.
		 */
		[[nodiscard]] bool is_failure() const
		{
			return status_ == OperationPreviewStatus::Failed;
		}

		/**
		 * @brief Checks whether the preview contains no geometry.
		 *
		 * @return True when the status is Empty.
		 */
		[[nodiscard]] bool is_empty() const
		{
			return status_ == OperationPreviewStatus::Empty;
		}

		/**
		 * @brief Checks whether the preview is invalidated.
		 *
		 * @return True when the status is Invalidated.
		 */
		[[nodiscard]] bool is_invalidated() const
		{
			return status_ == OperationPreviewStatus::Invalidated;
		}

		/**
		 * @brief Converts the preview to true when it is ready.
		 */
		[[nodiscard]] explicit operator bool() const
		{
			return is_ready();
		}

		/**
		 * @brief Returns the preview status.
		 *
		 * @return Current preview status.
		 */
		[[nodiscard]] OperationPreviewStatus status() const
		{
			return status_;
		}

		/**
		 * @brief Sets the preview status.
		 *
		 * @param status New status.
		 */
		void set_status(OperationPreviewStatus status)
		{
			status_ = status;
		}

		/**
		 * @brief Returns the preview mesh.
		 *
		 * @return Read-only preview mesh.
		 */
		[[nodiscard]] const PreviewMesh& mesh() const
		{
			return mesh_;
		}

		/**
		 * @brief Returns the preview mesh.
		 *
		 * @return Mutable preview mesh.
		 */
		[[nodiscard]] PreviewMesh& mesh()
		{
			return mesh_;
		}

		/**
		 * @brief Replaces the preview mesh.
		 *
		 * @param mesh New preview mesh.
		 */
		void set_mesh(PreviewMesh mesh)
		{
			mesh_ = std::move(mesh);
			if (status_ == OperationPreviewStatus::Ready && mesh_.empty()) {
				status_ = OperationPreviewStatus::Empty;
			}
		}

		/**
		 * @brief Returns the status message.
		 *
		 * @return Human-readable message.
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
		 * @brief Clears the preview payload and marks it invalidated.
		 */
		void invalidate()
		{
			mesh_.clear();
			status_ = OperationPreviewStatus::Invalidated;
		}

	private:
		OperationPreviewStatus status_ = OperationPreviewStatus::Empty;
		PreviewMesh mesh_{};
		std::string message_{};
	};

}