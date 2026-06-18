#pragma once

#include "kernel/common/Error.h"
#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/topology/TopologyValidator.h"

#include <string>
#include <utility>

namespace locus::kernel::modeling {

enum class OperationStatus {
    Success,
    Failed,
    NoChange,
    Cancelled
};

class OperationResult {
public:
    OperationResult() = default;

    [[nodiscard]] static OperationResult success(geometry::LEMDiff diff = {})
    {
        OperationResult result;
        result.status_ = OperationStatus::Success;
        result.diff_ = std::move(diff);
        return result;
    }

    [[nodiscard]] static OperationResult no_change(std::string message = {})
    {
        OperationResult result;
        result.status_ = OperationStatus::NoChange;
        result.message_ = std::move(message);
        return result;
    }

    [[nodiscard]] static OperationResult cancelled(std::string message = {})
    {
        OperationResult result;
        result.status_ = OperationStatus::Cancelled;
        result.message_ = std::move(message);
        return result;
    }

    [[nodiscard]] static OperationResult fail(kernel::Error error)
    {
        OperationResult result;
        result.status_ = OperationStatus::Failed;
        result.error_ = std::move(error);
        return result;
    }

    [[nodiscard]] static OperationResult fail(kernel::ErrorCode code, std::string message = {})
    {
        return fail(kernel::Error::make(code, std::move(message)));
    }

    [[nodiscard]] bool is_success() const
    {
        return status_ == OperationStatus::Success;
    }

    [[nodiscard]] bool is_failure() const
    {
        return status_ == OperationStatus::Failed;
    }

    [[nodiscard]] bool changed() const
    {
        return status_ == OperationStatus::Success && !diff_.empty();
    }

    [[nodiscard]] explicit operator bool() const
    {
        return is_success();
    }

    [[nodiscard]] OperationStatus status() const
    {
        return status_;
    }

    [[nodiscard]] const kernel::Error& error() const
    {
        return error_;
    }

    [[nodiscard]] const std::string& message() const
    {
        return message_;
    }

    [[nodiscard]] geometry::LEMDiff& diff()
    {
        return diff_;
    }

    [[nodiscard]] const geometry::LEMDiff& diff() const
    {
        return diff_;
    }

    void set_validation_report(geometry::TopologyValidationReport report)
    {
        validationReport_ = std::move(report);
        hasValidationReport_ = true;
    }

    [[nodiscard]] bool has_validation_report() const
    {
        return hasValidationReport_;
    }

    [[nodiscard]] const geometry::TopologyValidationReport& validation_report() const
    {
        return validationReport_;
    }

private:
    OperationStatus status_ = OperationStatus::Success;
    kernel::Error error_{};
    std::string message_{};
    geometry::LEMDiff diff_{};
    geometry::TopologyValidationReport validationReport_{};
    bool hasValidationReport_ = false;
};

}