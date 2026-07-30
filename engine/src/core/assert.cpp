/**
 * @file assert.cpp
 * @brief Assert-Handler-Implementierung.
 */
#include <aether/core/assert.hpp>
#include <aether/core/logger.hpp>

#include <iostream>
#include <sstream>

namespace aether::core {

[[noreturn]] void assertion_failed(std::string_view expr,
                                   std::string_view file,
                                   int line,
                                   std::string_view message) {
    std::ostringstream oss;
    oss << "Assertion failed: (" << expr << ") at " << file << ':' << line;
    if (!message.empty()) {
        oss << " – " << message;
    }
    const std::string text = oss.str();

    log_message(LogLevel::Fatal, "Assert", text);
    if (Logger* lg = active_logger()) {
        lg->flush();
    }

    std::cerr << text << std::endl;
    std::abort();
}

} // namespace aether::core
