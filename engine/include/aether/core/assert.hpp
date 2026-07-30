/**
 * @file assert.hpp
 * @brief Debug-Assertions und Unreachable-Hilfen.
 */
#pragma once

#include <aether/core/logger_fwd.hpp>

#include <cstdlib>
#include <string_view>

namespace aether::core {

/**
 * @brief Interner Assert-Handler (loggt und bricht im Debug ab).
 */
[[noreturn]] void assertion_failed(std::string_view expr,
                                   std::string_view file,
                                   int line,
                                   std::string_view message);

[[noreturn]] inline void unreachable_impl(std::string_view file, int line) {
    assertion_failed("unreachable", file, line, "code path should be unreachable");
}

} // namespace aether::core

// AETHER_DEBUG wird vom Build-System gesetzt (1/0). Fallback:
#ifndef AETHER_DEBUG
#  ifndef NDEBUG
#    define AETHER_DEBUG 1
#  else
#    define AETHER_DEBUG 0
#  endif
#endif

#if AETHER_DEBUG
#  define AETHER_ASSERT(expr)                                                          \
      do {                                                                             \
          if (!(expr)) {                                                               \
              ::aether::core::assertion_failed(#expr, __FILE__, __LINE__, "");         \
          }                                                                            \
      } while (0)

#  define AETHER_ASSERT_MSG(expr, msg)                                                 \
      do {                                                                             \
          if (!(expr)) {                                                               \
              ::aether::core::assertion_failed(#expr, __FILE__, __LINE__, (msg));      \
          }                                                                            \
      } while (0)
#else
#  define AETHER_ASSERT(expr)          ((void)0)
#  define AETHER_ASSERT_MSG(expr, msg) ((void)0)
#endif

#define AETHER_UNREACHABLE() ::aether::core::unreachable_impl(__FILE__, __LINE__)
