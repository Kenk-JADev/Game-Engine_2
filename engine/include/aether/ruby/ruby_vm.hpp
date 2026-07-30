/**
 * @file ruby_vm.hpp
 * @brief Eingebettete Ruby-VM für Spiellogik (konzeptionell RGSS-ähnlich, neu).
 *
 * Backend:
 *  - StubRubyVm  – immer verfügbar (parst/führt nicht aus, API-Smoke)
 *  - MRubyVM     – echte mruby-VM wenn AETHER_WITH_MRUBY
 *
 * Ruby ist ausschließlich für Spiellogik gedacht. Die Engine stellt Module:
 *   Graphics, Audio, Input, SceneManager, Player, NPC, Enemy,
 *   Camera, Weather, Inventory, Quest, Dialogue, Map
 */
#pragma once

#include <aether/core/types.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace aether::ruby {

enum class RubyBackend {
    Stub,
    MRuby,
};

/**
 * @brief Ergebnis einer Script-Ausführung.
 */
struct EvalResult {
    bool ok = false;
    std::string value;   ///< String-Repräsentation des Rückgabewerts
    std::string error;   ///< Fehlermeldung bei !ok
};

/**
 * @brief Registrierte Host-Funktion (C++ → von Ruby aufrufbar).
 *
 * Phase-1-Stub: Name + argc; echter Dispatch folgt mit mruby.
 */
struct HostFunction {
    std::string module; ///< z. B. "Audio"
    std::string name;   ///< z. B. "bgm_play"
    i32 arity = -1;     ///< -1 = variabel
    std::function<std::string(const std::vector<std::string>& args)> fn;
};

/**
 * @brief Ruby-VM-Schnittstelle.
 */
class RubyVM : public aether::NonMovable {
public:
    /**
     * @brief Erzeugt eine VM.
     * @param backend  MRuby wenn mit AETHER_WITH_MRUBY gebaut, sonst Stub.
     *                 Default wählt automatisch das beste verfügbare Backend.
     */
    [[nodiscard]] static std::unique_ptr<RubyVM> create(
#if defined(AETHER_WITH_MRUBY)
        RubyBackend backend = RubyBackend::MRuby
#else
        RubyBackend backend = RubyBackend::Stub
#endif
    );

    virtual ~RubyVM();

    [[nodiscard]] RubyBackend backend() const noexcept { return backend_; }

    /**
     * @brief Bootstrapped Built-in-Module (Graphics, Audio, …) als Ruby-Stubs.
     */
    virtual void define_engine_api() = 0;

    /** @brief Registriert eine Host-Funktion unter Module.name. */
    void define_function(HostFunction fn);

    /** @brief Führt Quelltext aus. */
    [[nodiscard]] virtual EvalResult eval(std::string_view source,
                                          std::string_view filename = "<eval>") = 0;

    /** @brief Lädt und führt eine Datei aus. */
    [[nodiscard]] virtual EvalResult load_file(std::string_view path) = 0;

    /**
     * @brief Hot-Reload: Datei erneut laden (Editor/Testspiel).
     * @return EvalResult
     */
    [[nodiscard]] EvalResult reload_file(std::string_view path);

    /** @brief Ruft eine zuvor definierte Host-Funktion auf (Tests). */
    [[nodiscard]] EvalResult call_host(std::string_view module,
                                       std::string_view name,
                                       const std::vector<std::string>& args = {});

    /** @brief Setzt/ liest eine globale Script-Variable (String). */
    virtual void set_global(std::string_view name, std::string_view value) = 0;
    [[nodiscard]] virtual std::string get_global(std::string_view name) const = 0;

    [[nodiscard]] const std::vector<std::string>& load_history() const noexcept {
        return load_history_;
    }

    /** @brief Letzte Fehlermeldung. */
    [[nodiscard]] const std::string& last_error() const noexcept { return last_error_; }

protected:
    explicit RubyVM(RubyBackend backend);

    HostFunction* find_host(std::string_view module, std::string_view name);
    const HostFunction* find_host(std::string_view module, std::string_view name) const;

    RubyBackend backend_;
    std::vector<HostFunction> host_functions_;
    std::vector<std::string> load_history_;
    std::string last_error_;
    std::unordered_map<std::string, std::string> globals_;
};

/**
 * @brief Liefert den vordefinierten Ruby-API-Bootstrap-Quelltext (Dokumentation/Stub).
 */
[[nodiscard]] std::string engine_api_bootstrap_source();

} // namespace aether::ruby
