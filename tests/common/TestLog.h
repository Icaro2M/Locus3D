/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file TestLog.h
 * @brief Minimal logging helpers for test executables.
 */

#include <iostream>
#include <string_view>

namespace locus::tests {

/**
 * @brief Logging verbosity used by test helpers.
 */
enum class TestLogLevel {
    quiet,
    normal,
    verbose
};

namespace detail {

/**
 * @brief Stores the mutable process-wide test log level.
 *
 * @return Mutable log level reference.
 */
[[nodiscard]] inline TestLogLevel& test_log_level_storage()
{
    static TestLogLevel level = TestLogLevel::normal;
    return level;
}

/**
 * @brief Writes one formatted test log message.
 *
 * @param stream Output stream.
 * @param prefix Message prefix.
 * @param message Message body.
 */
inline void write_log(std::ostream& stream, std::string_view prefix, std::string_view message)
{
    stream << prefix << message << '\n';
}

} // namespace detail

/**
 * @brief Returns the active process-wide test log level.
 *
 * @return Current log level.
 */
[[nodiscard]] inline TestLogLevel test_log_level()
{
    return detail::test_log_level_storage();
}

/**
 * @brief Sets the active process-wide test log level.
 *
 * @param level New log level.
 */
inline void set_test_log_level(TestLogLevel level)
{
    detail::test_log_level_storage() = level;
}

/**
 * @brief Returns the active process-wide test log level.
 *
 * @return Current log level.
 */
[[nodiscard]] inline TestLogLevel current_test_log_level()
{
    return test_log_level();
}

/**
 * @brief Updates the active process-wide test log level.
 *
 * @param level New log level.
 */
inline void set_current_test_log_level(TestLogLevel level)
{
    set_test_log_level(level);
}

/**
 * @brief Logs an informational message when normal logging is enabled.
 *
 * @param message Message body.
 */
inline void log_info(std::string_view message)
{
    if (current_test_log_level() != TestLogLevel::quiet) {
        detail::write_log(std::cout, "[info] ", message);
    }
}

/**
 * @brief Logs a warning message when normal logging is enabled.
 *
 * @param message Message body.
 */
inline void log_warning(std::string_view message)
{
    if (current_test_log_level() != TestLogLevel::quiet) {
        detail::write_log(std::cerr, "[warning] ", message);
    }
}

/**
 * @brief Logs an error message.
 *
 * @param message Message body.
 */
inline void log_error(std::string_view message)
{
    detail::write_log(std::cerr, "[error] ", message);
}

/**
 * @brief Logs a verbose message only when verbose logging is enabled.
 *
 * @param message Message body.
 */
inline void log_verbose(std::string_view message)
{
    if (current_test_log_level() == TestLogLevel::verbose) {
        detail::write_log(std::cout, "[verbose] ", message);
    }
}

} // namespace locus::tests
