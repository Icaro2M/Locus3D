#pragma once

#include <functional>
#include <iostream>
#include <mutex>
#include <string_view>

namespace locus::graphics
{
    enum class GraphicsLogLevel
    {
        Trace,
        Info,
        Warning,
        Error
    };

    class GraphicsLogger
    {
    public:
        using LogCallback = std::function<void(GraphicsLogLevel, std::string_view)>;

        static void set_callback(LogCallback callback)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback_ = std::move(callback);
        }

        static void clear_callback()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            callback_ = nullptr;
        }

        static void trace(std::string_view message)
        {
            log(GraphicsLogLevel::Trace, message);
        }

        static void info(std::string_view message)
        {
            log(GraphicsLogLevel::Info, message);
        }

        static void warning(std::string_view message)
        {
            log(GraphicsLogLevel::Warning, message);
        }

        static void error(std::string_view message)
        {
            log(GraphicsLogLevel::Error, message);
        }

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