/**
 * @file smoke_test.cpp
 * @brief Minimaler Smoke-Test: Engine-Version erreichbar, Link ok.
 */
#include <aether/aether.hpp>
#include <cstdio>
#include <cstring>

int main() {
    const char* ver = aether::version();
    if (ver == nullptr || std::strlen(ver) == 0) {
        std::fputs("FAIL: version() empty\n", stderr);
        return 1;
    }
    std::printf("OK: Aether Engine %s\n", ver);
    return 0;
}
