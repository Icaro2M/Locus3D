/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <functional>
#include <iostream>
#include <mutex>
#include <string_view>

namespace locus::graphics
{
    /**
     * @brief Severity levels used by graphics subsystem logging.
     */
    enum class GraphicsLogLevel
    {
        Trace,
        Info,
        Warning,
        Error
    };

    /**
     * @brief Thread-safe static logger for graphics diagnostics.
     *
     * GraphicsLogger can forward messages to an application callback, or fall
     * back to standard streams when no callback is registered.
     */
    class GraphicsLogger
    {
    public:
        /**
         * @brief Callback type used to receive graphics log messages.
         */
        using LogCallback = std::function<void(GraphicsLogLevel, std::string_view)>;

        /**
         * @brief Installs a callback for all future graphics log messages.
         *
         * @param callback Callback invoked with severity and message text.
         */
        static void set_callback(LogCallback callback)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback_ = std::move(callback);
        }

        /**
         * @brief Removes the active log callback.
         */
        static void clear_callback()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback_ = nullptr;
        }

        /**
         * @brief Emits a trace-level message.
         *
         * @param message Message text.
         */
        static void trace(std::string_view message)
        {
            log(GraphicsLogLevel::Trace, message);
        }

        /**
         * @brief Emits an info-level message.
         *
         * @param message Message text.
         */
        static void info(std::string_view message)
        {
            log(GraphicsLogLevel::Info, message);
        }

        /**
         * @brief Emits a warning-level message.
         *
         * @param message Message text.
         */
        static void warning(std::string_view message)
        {
            log(GraphicsLogLevel::Warning, message);
        }

        /**
         * @brief Emits an error-level message.
         *
         * @param message Message text.
         */
        static void error(std::string_view message)
        {
            log(GraphicsLogLevel::Error, message);
        }

        /**
         * @brief Emits a message with an explicit severity level.
         *
         * @param level Message severity.
         * @param message Message text.
         */
        static void log(GraphicsLogLevel level, std::string_view message)
        {
            LogCallback callbackCopy;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                callbackCopy = callback_;
            }

            if (callbackCopy)
            {
                callbackCopy(level, message);
                return;
            }

            default_log(level, message);
        }

        /**
         * @brief Converts a log level to a lowercase name.
         *
         * @param level Log level to convert.
         * @return Static string for the level name.
         */
        static constexpr const char* level_name(GraphicsLogLevel level)
        {
            switch (level)
            {
            case GraphicsLogLevel::Trace:
                return "trace";
            case GraphicsLogLevel::Info:
                return "info";
            case GraphicsLogLevel::Warning:
                return "warning";
            case GraphicsLogLevel::Error:
                return "error";
            }

            return "unknown";
        }

    private:
        static void default_log(GraphicsLogLevel level, std::string_view message)
        {
            std::ostream& stream =
                level == GraphicsLogLevel::Error || level == GraphicsLogLevel::Warning
                ? std::cerr
                : std::clog;

            stream << "[graphics][" << level_name(level) << "] " << message << '\n';
        }

    private:
        inline static std::mutex mutex_;
        inline static LogCallback callback_;
    };
}
