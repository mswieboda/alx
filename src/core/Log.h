#pragma once
#include <string>

#ifndef ALX_ENABLE_DEBUG
#  ifdef DEBUG
#    define ALX_ENABLE_DEBUG 1
#  else
#    define ALX_ENABLE_DEBUG 0
#  endif
#endif // !defined(ALX_ENABLE_DEBUG)

#if ALX_ENABLE_DEBUG

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
} // namespace Log

#else // !ALX_ENABLE_DEBUG

namespace Log {
    inline void msg(const std::string&) {}
    inline void msg_t(const std::string&) {}
    inline void fmt(const char*, ...) {}
    inline void fmt_t(const char*, ...) {}
    inline void info(const std::string&) {}
    inline void debug(const std::string&) {}
    inline void warn(const std::string&) {}
    inline void error(const std::string&) {}
    inline void info_t(const std::string&) {}
    inline void debug_t(const std::string&) {}
    inline void warn_t(const std::string&) {}
    inline void error_t(const std::string&) {}
    inline void info_fmt(const char*, ...) {}
    inline void debug_fmt(const char*, ...) {}
    inline void warn_fmt(const char*, ...) {}
    inline void error_fmt(const char*, ...) {}
    inline void info_fmt_t(const char*, ...) {}
    inline void debug_fmt_t(const char*, ...) {}
    inline void warn_fmt_t(const char*, ...) {}
    inline void error_fmt_t(const char*, ...) {}
} // namespace Log

#endif // ALX_ENABLE_DEBUG

