#include "Log.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdarg>
#include <vector>

namespace Log {
    // Internal helper to generate the high-res timestamp string
    static std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::stringstream ss;
        // Output format: HH:MM:SS.mmm
        ss << std::put_time(std::localtime(&time_t_now), "%H:%M:%S")
           << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    // Internal helper for variadic string formatting with optional prefix and timestamp
    static void vfmt(const char* prefix, bool use_timestamp, const char* format, va_list args) {
        va_list args_copy;
        va_copy(args_copy, args);
        int size = vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);

        if (size > 0) {
            std::vector<char> buf(size + 1);
            vsnprintf(buf.data(), buf.size(), format, args);
            if (use_timestamp) {
                std::cout << "[" << timestamp() << "] " << (prefix ? prefix : "") << buf.data() << std::endl;
            } else {
                std::cout << (prefix ? prefix : "") << buf.data() << std::endl;
            }
        }
    }

    void msg(const std::string& message) {
        std::cout << message << std::endl;
    }

    void msg_t(const std::string& message) {
        std::cout << "[" << timestamp() << "] " << message << std::endl;
    }

    void fmt(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vfmt("", false, format, args);
        va_end(args);
    }

    void fmt_t(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vfmt("", true, format, args);
        va_end(args);
    }

    // --- type msg

    void info(const std::string& message) {
        msg("[INFO] " + message);
    }

    void debug(const std::string& message) {
        msg("[DEBUG] " + message);
    }

    void warn(const std::string& message) {
        msg("[WARN] " + message);
    }

    void error(const std::string& message) {
        msg("[ERROR] " + message);
    }

    // --- type msg with timestamp

    void info_t(const std::string& message) {
        msg_t("[INFO] " + message);
    }

    void debug_t(const std::string& message) {
        msg_t("[DEBUG] " + message);
    }

    void warn_t(const std::string& message) {
        msg_t("[WARN] " + message);
    }

    void error_t(const std::string& message) {
        msg_t("[ERROR] " + message);
    }

    // --- type fmt

    void info_fmt(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vfmt("[INFO] ", false, format, args);
        va_end(args);
    }

    void debug_fmt(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vfmt("[DEBUG] ", false, format, args);
        va_end(args);
    }

    void warn_fmt(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vfmt("[WARN] ", false, format, args);
        va_end(args);
    }

    void error_fmt(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vfmt("[ERROR] ", false, format, args);
        va_end(args);
    }

    // --- type fmt with timestamp

    void info_fmt_t(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vfmt("[INFO] ", true, format, args);
        va_end(args);
    }

    void debug_fmt_t(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vfmt("[DEBUG] ", true, format, args);
        va_end(args);
    }

    void warn_fmt_t(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vfmt("[WARN] ", true, format, args);
        va_end(args);
    }

    void error_fmt_t(const char* format, ...) {
        va_list args;
        va_start(args, format);
        vfmt("[ERROR] ", true, format, args);
        va_end(args);
    }
}
