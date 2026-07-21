#include "Log.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdarg>
#include <vector>

namespace Log {
    // Internal helper to generate the high-res timestamp string
    static std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        // Output format: HH:MM:SS.mmm
        ss << std::put_time(std::localtime(&time_t_now), "%H:%M:%S") 
           << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }

    void msg(const std::string& message) {
        std::cout << message << std::endl;
    }

    void msg_t(const std::string& message) {
        std::cout << "[" << get_timestamp() << "] " << message << std::endl;
    }

    void fmt(const char* format, ...) {
        va_list args;
        va_start(args, format);

        // Determine required buffer size
        va_list args_copy;
        va_copy(args_copy, args);
        int size = vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);

        if (size > 0) {
            std::vector<char> buf(size + 1);
            vsnprintf(buf.data(), buf.size(), format, args);
            std::cout << buf.data() << std::endl;
        }

        va_end(args);
    }

    void fmt_t(const char* format, ...) {
        va_list args;
        va_start(args, format);

        // Determine required buffer size
        va_list args_copy;
        va_copy(args_copy, args);
        int size = vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);

        if (size > 0) {
            std::vector<char> buf(size + 1);
            vsnprintf(buf.data(), buf.size(), format, args);
            std::cout << "[" << get_timestamp() << "] " << buf.data() << std::endl;
        }
        
        va_end(args);
    }

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
}
