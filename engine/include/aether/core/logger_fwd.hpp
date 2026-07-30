/**
 * @file logger_fwd.hpp
 * @brief Forward-Deklarationen für den Logger (vermeidet Zyklen mit assert).
 */
#pragma once

#include <string_view>

namespace aether::core {

enum class LogLevel : int;

void log_message(LogLevel level,
                 std::string_view subsystem,
                 std::string_view message);

} // namespace aether::core
