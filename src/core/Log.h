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

    // with prepended log type
    void info(const std::string& message);
    void debug(const std::string& message);
    void warn(const std::string& message);
    void error(const std::string& message);
}
