#pragma once

#include "kernel/common/Error.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"

#include <string_view>

namespace locus::kernel::modeling {

class IOperation {
public:
    virtual ~IOperation() = default;

    [[nodiscard]] virtual std::string_view name() const = 0;

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
    [[nodiscard]] virtual OperationResult execute_impl(OperationContext& context) = 0;
};

}