/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Error.h"
#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/topology/TopologyValidator.h"

#include <string>
#include <utility>

namespace locus::kernel::modeling {

/**
 * @brief High-level completion state for a modeling operation.
 */
enum class OperationStatus {
    /**
     * @brief Operation completed successfully.
     */
    Success,
    /**
     * @brief Operation failed with an error.
     */
    Failed,
    /**
     * @brief Operation completed without modifying the mesh.
     */
    NoChange,
    /**
     * @brief Operation was cancelled before making a committed change.
     */
    Cancelled
};

/**
 * @brief Result payload returned by modeling operation execution.
 */
class OperationResult {
public:
    OperationResult() = default;

    /**
     * @brief Creates a successful result.
     *
     * @param diff Mesh changes produced by the operation.
     * @return Successful operation result.
     */
    [[nodiscard]] static OperationResult success(geometry::LEMDiff diff = {})
    {
        OperationResult result;
        result.status_ = OperationStatus::Success;
        result.diff_ = std::move(diff);
        return result;
    }

    /**
     * @brief Creates a result for an operation that made no changes.
     *
     * @param message Optional human-readable reason.
     * @return No-change operation result.
     */
    [[nodiscard]] static OperationResult no_change(std::string message = {})
    {
        OperationResult result;
        result.status_ = OperationStatus::NoChange;
        result.message_ = std::move(message);
        return result;
    }

    /**
     * @brief Creates a cancelled operation result.
     *
     * @param message Optional human-readable reason.
     * @return Cancelled operation result.
     */
    [[nodiscard]] static OperationResult cancelled(std::string message = {})
    {
        OperationResult result;
        result.status_ = OperationStatus::Cancelled;
        result.message_ = std::move(message);
        return result;
    }

    /**
     * @brief Creates a failed operation result.
     *
     * @param error Error describing the failure.
     * @return Failed operation result.
     */
    [[nodiscard]] static OperationResult fail(kernel::Error error)
    {
        OperationResult result;
        result.status_ = OperationStatus::Failed;
        result.error_ = std::move(error);
        return result;
    }

    /**
     * @brief Creates a failed operation result from an error code.
     *
     * @param code Machine-readable error code.
     * @param message Optional human-readable message.
     * @return Failed operation result.
     */
    [[nodiscard]] static OperationResult fail(kernel::ErrorCode code, std::string message = {})
    {
        return fail(kernel::Error::make(code, std::move(message)));
    }

    /**
     * @brief Checks whether the operation completed successfully.
     *
     * @return True for OperationStatus::Success.
     */
    [[nodiscard]] bool is_success() const
    {
        return status_ == OperationStatus::Success;
    }

    /**
     * @brief Checks whether the operation failed.
     *
     * @return True for OperationStatus::Failed.
     */
    [[nodiscard]] bool is_failure() const
    {
        return status_ == OperationStatus::Failed;
    }

    /**
     * @brief Checks whether the successful operation produced mesh changes.
     *
     * @return True when the status is success and the diff is not empty.
     */
    [[nodiscard]] bool changed() const
    {
        return status_ == OperationStatus::Success && !diff_.empty();
    }

    /**
     * @brief Converts the result to true when it is successful.
     */
    [[nodiscard]] explicit operator bool() const
    {
        return is_success();
    }

    /**
     * @brief Returns the operation status.
     *
     * @return Completion status.
     */
    [[nodiscard]] OperationStatus status() const
    {
        return status_;
    }

    /**
     * @brief Returns the failure error.
     *
     * @return Error payload.
     */
    [[nodiscard]] const kernel::Error& error() const
    {
        return error_;
    }

    /**
     * @brief Returns the optional status message.
     *
     * @return Human-readable message.
     */
    [[nodiscard]] const std::string& message() const
    {
        return message_;
    }

    /**
     * @brief Returns the mutable mesh diff.
     *
     * @return Mutable diff reference.
     */
    [[nodiscard]] geometry::LEMDiff& diff()
    {
        return diff_;
    }

    /**
     * @brief Returns the mesh diff.
     *
     * @return Read-only diff reference.
     */
    [[nodiscard]] const geometry::LEMDiff& diff() const
    {
        return diff_;
    }

    /**
     * @brief Attaches a topology validation report to the result.
     *
     * @param report Validation report produced after execution.
     */
    void set_validation_report(geometry::TopologyValidationReport report)
    {
        validationReport_ = std::move(report);
        hasValidationReport_ = true;
    }

    /**
     * @brief Checks whether a validation report is attached.
     *
     * @return True when validation_report() can be read.
     */
    [[nodiscard]] bool has_validation_report() const
    {
        return hasValidationReport_;
    }

    /**
     * @brief Returns the attached topology validation report.
     *
     * @return Read-only validation report.
     */
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
