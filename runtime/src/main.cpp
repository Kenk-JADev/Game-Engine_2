/**
 * @file main.cpp
 * @brief Game.exe / Game – Runtime-Einstiegspunkt.
 */
#include <aether/runtime/bootstrap.hpp>

#include <exception>
#include <iostream>

int main(int argc, char** argv) {
    try {
        const auto options = aether::runtime::parse_args(argc, argv);
        return aether::runtime::run_game(options);
    } catch (const std::exception& ex) {
        std::cerr << "Fatal: " << ex.what() << '\n';
        return 2;
    } catch (...) {
        std::cerr << "Fatal: unknown error\n";
        return 2;
    }
}
