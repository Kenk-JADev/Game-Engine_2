/**
 * @file version.cpp
 * @brief Explizite Versionssymbole (für ABI/Debugging).
 */
#include <aether/aether.hpp>

namespace aether {

// Anker-Symbol, damit die Engine-Lib die Version exportiert.
extern const char* const kEngineVersionString;
const char* const kEngineVersionString = AETHER_VERSION_STRING;

} // namespace aether
