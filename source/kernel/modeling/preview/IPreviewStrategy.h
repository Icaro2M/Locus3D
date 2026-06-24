/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/preview/OperationPreview.h"

#include <string_view>

namespace locus::kernel::modeling {

	/**
	 * @brief Interface implemented by non-destructive operation preview strategies.
	 *
	 * A preview strategy should not mutate the source mesh stored in the operation
	 * context. Strategies that need to run modeling logic should do so on a copied
	 * ghost mesh and return derived render data.
	 */
	class IPreviewStrategy {
	public:
		/**
		 * @brief Destroys the preview strategy interface.
		 */
		virtual ~IPreviewStrategy() = default;

		/**
		 * @brief Returns the stable strategy name used by tooling and diagnostics.
		 *
		 * @return Strategy name.
		 */
		[[nodiscard]] virtual std::string_view name() const = 0;

		/**
		 * @brief Builds a preview for the given operation context.
		 *
		 * @param context Source operation context.
		 * @return Operation preview payload.
		 */
		[[nodiscard]] virtual OperationPreview build_preview(
			const OperationContext& context) = 0;
	};

}