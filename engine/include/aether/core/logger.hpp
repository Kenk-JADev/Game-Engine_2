/**
 * @file logger.hpp
 * @brief Thread-sicherer Logger mit Konsole- und Datei-Sink.
 *
 * Format: [Timestamp] [Thread] [Level] [Subsystem] Message
 */
#pragma once

#include <aether/core/types.hpp>

#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace aether::core {

/** @brief Log-Schweregrad (aufsteigend). */
enum class LogLevel : int {
    Trace = 0,
    Debug = 1,
    Info  = 2,
    Warn  = 3,
    Error = 4,
    Fatal = 5,
    Off   = 6,
};

/**
 * @brief Konvertiert LogLevel in eine kurze Anzeigezeichenkette.
 */
[[nodiscard]] const char* to_string(LogLevel level) noexcept;

/**
 * @brief Abstrakter Log-Sink.
 */
class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual void write(LogLevel level, std::string_view formatted_line) = 0;
    virtual void flush() {}
};

/** @brief Schreibt farbig (ANSI) auf stdout/stderr. */
class ConsoleSink final : public ILogSink {
public:
    void write(LogLevel level, std::string_view formatted_line) override;
    void flush() override;
};

/** @brief Hängt Zeilen an eine Logdatei an. */
class FileSink final : public ILogSink {
public:
    explicit FileSink(std::string path);
    ~FileSink() override;

    FileSink(const FileSink&) = delete;
    FileSink& operator=(const FileSink&) = delete;

    [[nodiscard]] bool is_open() const noexcept { return stream_.is_open(); }
    [[nodiscard]] const std::string& path() const noexcept { return path_; }

    void write(LogLevel level, std::string_view formatted_line) override;
    void flush() override;

private:
    std::string path_;
    std::ofstream stream_;
    std::mutex mutex_;
};

/**
 * @brief Zentraler Logger (Singleton-ähnlich, aber über EngineContext gehalten).
 *
 * Lebensdauer: wird von EngineContext besitzt. Freie Funktionen
 * `aether::core::log_*` leiten an die aktive Instanz weiter.
 */
class Logger : public NonMovable {
public:
    Logger();
    ~Logger();

    void set_level(LogLevel level) noexcept { level_ = level; }
    [[nodiscard]] LogLevel level() const noexcept { return level_; }

    void add_sink(std::shared_ptr<ILogSink> sink);
    void clear_sinks();

    /**
     * @brief Schreibt eine Nachricht, falls level >= Schwelle.
     */
    void log(LogLevel level, std::string_view subsystem, std::string_view message);

    void flush();

    /** @brief Erzeugt Standard-Dateinamen logs/aether_YYYYMMDD.log */
    static std::string default_log_path(std::string_view directory = "logs");

private:
    [[nodiscard]] std::string format_line(LogLevel level,
                                          std::string_view subsystem,
                                          std::string_view message) const;

    LogLevel level_ = LogLevel::Info;
    std::vector<std::shared_ptr<ILogSink>> sinks_;
    mutable std::mutex mutex_;
};

// -----------------------------------------------------------------------------
// Prozessweite aktive Logger-Instanz (gesetzt durch EngineContext)
// -----------------------------------------------------------------------------

/** @brief Setzt den aktiven Logger (oder nullptr). Nicht besitzend. */
void set_active_logger(Logger* logger) noexcept;

/** @brief Liefert den aktiven Logger oder nullptr. */
[[nodiscard]] Logger* active_logger() noexcept;

void log_message(LogLevel level,
                 std::string_view subsystem,
                 std::string_view message);

inline void log_trace(std::string_view sub, std::string_view msg) {
    log_message(LogLevel::Trace, sub, msg);
}
inline void log_debug(std::string_view sub, std::string_view msg) {
    log_message(LogLevel::Debug, sub, msg);
}
inline void log_info(std::string_view sub, std::string_view msg) {
    log_message(LogLevel::Info, sub, msg);
}
inline void log_warn(std::string_view sub, std::string_view msg) {
    log_message(LogLevel::Warn, sub, msg);
}
inline void log_error(std::string_view sub, std::string_view msg) {
    log_message(LogLevel::Error, sub, msg);
}
inline void log_fatal(std::string_view sub, std::string_view msg) {
    log_message(LogLevel::Fatal, sub, msg);
}

} // namespace aether::core

// Bequeme Makros mit festem Subsystem-Tag
#define AETHER_LOG_TRACE(sub, msg) ::aether::core::log_trace((sub), (msg))
#define AETHER_LOG_DEBUG(sub, msg) ::aether::core::log_debug((sub), (msg))
#define AETHER_LOG_INFO(sub, msg)  ::aether::core::log_info((sub), (msg))
#define AETHER_LOG_WARN(sub, msg)  ::aether::core::log_warn((sub), (msg))
#define AETHER_LOG_ERROR(sub, msg) ::aether::core::log_error((sub), (msg))
#define AETHER_LOG_FATAL(sub, msg) ::aether::core::log_fatal((sub), (msg))
