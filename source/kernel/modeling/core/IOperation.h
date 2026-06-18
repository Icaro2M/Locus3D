/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Error.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"

#include <string_view>

namespace locus::kernel::modeling {

/**
 * @brief Base interface for mesh modeling operations.
 */
class IOperation {
public:
    /**
     * @brief Destroys the operation interface.
     */
    virtual ~IOperation() = default;

    /**
     * @brief Returns the stable operation name used by diagnostics and tooling.
     *
     * @return Operation name.
     */
    [[nodiscard]] virtual std::string_view name() const = 0;

    /**
     * @brief Executes the operation with shared context validation.
     *
     * @param context Operation execution context.
     * @return Operation result with optional diff and validation report.
     */
    [[nodiscard]] OperationResult execute(OperationContext& context)
    {
        if (!context.has_mesh()) {
            return OperationResult::fail(kernel::ErrorCode::InvalidArgument, "OperationContext does not contain a mesh.");
        }

        OperationResult result = execute_impl(context);

        if (!result.is_success()) {
            return result;
        }

        if (context.validateAfterExecute) {
            geometry::TopologyValidationReport report = geometry::TopologyValidator::validate(context.editable_mesh());
            result.set_validation_report(report);

            if (!report.valid()) {
                return OperationResult::fail(kernel::ErrorCode::InvalidState, "Operation produced invalid topology.");
            }
        }

        return result;
    }

private:
    /**
     * @brief Executes operation-specific mutation logic.
     *
     * @param context Operation execution context.
     * @return Operation-specific result before shared post-validation.
     */
    [[nodiscard]] virtual OperationResult execute_impl(OperationContext& context) = 0;
};

}
