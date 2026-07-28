#pragma once
#include <string>

namespace Log {
    // Standard text logger
    void msg(const std::string& message);

    // with prepended timestamp
    void msg_t(const std::string& message);

    // Formatted helper for quickly printing text with primitive data types
    void fmt(const char* format, ...);

    // with prepended timestamp
    void fmt_t(const char* format, ...);

    // msg with prepended log type
    void info(const std::string& message);
    void debug(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);

    // msg with prepended log type, timestamp
    void info_t(const std::string& message);
    void debug_t(const std::string& message);
    void warn_t(const std::string& message);
    void error_t(const std::string& message);

    // fmt with prepended log type
    void info_fmt(const char* format, ...);
    void debug_fmt(const char* format, ...);
    void warn_fmt(const char* format, ...);
    void error_fmt(const char* format, ...);

    // fmt with prepended log type, timestamp
    void info_fmt_t(const char* format, ...);
    void debug_fmt_t(const char* format, ...);
    void warn_fmt_t(const char* format, ...);
    void error_fmt_t(const char* format, ...);
}
