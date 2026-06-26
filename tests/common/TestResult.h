/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file TestResult.h
 * @brief Simple success/failure result type for test helpers.
 */

#include <string>
#include <utility>

namespace locus::tests {

/**
 * @brief Represents the outcome of a lightweight test helper.
 */
struct TestResult {
    bool success = false;
    std::string message;

    /**
     * @brief Creates a successful result.
     *
     * @param message Optional message.
     * @return Passing result.
     */
    [[nodiscard]] static TestResult pass(std::string message = {})
    {
        return { true, std::move(message) };
    }

    /**
     * @brief Creates a failing result.
     *
     * @param message Failure message.
     * @return Failing result.
     */
    [[nodiscard]] static TestResult fail(std::string message)
    {
        return { false, std::move(message) };
    }
};

} // namespace locus::tests
