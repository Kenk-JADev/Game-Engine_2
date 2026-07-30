/**
 * @file logger.cpp
 * @brief Implementierung des Aether-Loggers.
 */
#include <aether/core/logger.hpp>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>

namespace aether::core {
namespace {

Logger* g_active_logger = nullptr;

std::string current_timestamp() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const auto t = clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()) %
                    1000;

    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << '.'
        << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

std::string thread_tag() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

const char* ansi_color(LogLevel level) {
    switch (level) {
    case LogLevel::Trace: return "\033[90m";
    case LogLevel::Debug: return "\033[36m";
    case LogLevel::Info:  return "\033[32m";
    case LogLevel::Warn:  return "\033[33m";
    case LogLevel::Error: return "\033[31m";
    case LogLevel::Fatal: return "\033[35m";
    default:              return "\033[0m";
    }
}

constexpr const char* kAnsiReset = "\033[0m";

} // namespace

const char* to_string(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info:  return "INFO";
    case LogLevel::Warn:  return "WARN";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Fatal: return "FATAL";
    case LogLevel::Off:   return "OFF";
    }
    return "???";
}

// -----------------------------------------------------------------------------
// ConsoleSink
// -----------------------------------------------------------------------------

void ConsoleSink::write(LogLevel level, std::string_view formatted_line) {
    std::ostream& out = (level >= LogLevel::Error) ? std::cerr : std::cout;
    out << ansi_color(level) << formatted_line << kAnsiReset << '\n';
}

void ConsoleSink::flush() {
    std::cout.flush();
    std::cerr.flush();
}

// -----------------------------------------------------------------------------
// FileSink
// -----------------------------------------------------------------------------

FileSink::FileSink(std::string path) : path_(std::move(path)) {
    namespace fs = std::filesystem;
    std::error_code ec;
    const auto parent = fs::path(path_).parent_path();
    if (!parent.empty()) {
        fs::create_directories(parent, ec);
    }
    stream_.open(path_, std::ios::out | std::ios::app);
}

FileSink::~FileSink() {
    if (stream_.is_open()) {
        stream_.flush();
        stream_.close();
    }
}

void FileSink::write(LogLevel /*level*/, std::string_view formatted_line) {
    std::lock_guard lock(mutex_);
    if (!stream_.is_open()) {
        return;
    }
    stream_ << formatted_line << '\n';
}

void FileSink::flush() {
    std::lock_guard lock(mutex_);
    if (stream_.is_open()) {
        stream_.flush();
    }
}

// -----------------------------------------------------------------------------
// Logger
// -----------------------------------------------------------------------------

Logger::Logger() = default;

Logger::~Logger() {
    flush();
    if (g_active_logger == this) {
        g_active_logger = nullptr;
    }
}

void Logger::add_sink(std::shared_ptr<ILogSink> sink) {
    if (!sink) {
        return;
    }
    std::lock_guard lock(mutex_);
    sinks_.push_back(std::move(sink));
}

void Logger::clear_sinks() {
    std::lock_guard lock(mutex_);
    sinks_.clear();
}

std::string Logger::format_line(LogLevel level,
                                std::string_view subsystem,
                                std::string_view message) const {
    std::ostringstream oss;
    oss << '[' << current_timestamp() << "] "
        << '[' << thread_tag() << "] "
        << '[' << to_string(level) << "] "
        << '[' << subsystem << "] "
        << message;
    return oss.str();
}

void Logger::log(LogLevel level, std::string_view subsystem, std::string_view message) {
    if (level < level_ || level == LogLevel::Off) {
        return;
    }
    const std::string line = format_line(level, subsystem, message);

    std::vector<std::shared_ptr<ILogSink>> sinks_copy;
    {
        std::lock_guard lock(mutex_);
        sinks_copy = sinks_;
    }
    for (auto& sink : sinks_copy) {
        sink->write(level, line);
    }
}

void Logger::flush() {
    std::vector<std::shared_ptr<ILogSink>> sinks_copy;
    {
        std::lock_guard lock(mutex_);
        sinks_copy = sinks_;
    }
    for (auto& sink : sinks_copy) {
        sink->flush();
    }
}

std::string Logger::default_log_path(std::string_view directory) {
    using clock = std::chrono::system_clock;
    const auto t = clock::to_time_t(clock::now());
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::ostringstream name;
    name << "aether_" << std::put_time(&tm_buf, "%Y%m%d") << ".log";
    return (std::filesystem::path(directory) / name.str()).string();
}

// -----------------------------------------------------------------------------
// Free functions
// -----------------------------------------------------------------------------

void set_active_logger(Logger* logger) noexcept {
    g_active_logger = logger;
}

Logger* active_logger() noexcept {
    return g_active_logger;
}

void log_message(LogLevel level, std::string_view subsystem, std::string_view message) {
    if (g_active_logger != nullptr) {
        g_active_logger->log(level, subsystem, message);
        return;
    }
    // Fallback vor Context-Boot: roh auf stderr/stdout
    if (level < LogLevel::Info) {
        return;
    }
    std::ostream& out = (level >= LogLevel::Error) ? std::cerr : std::cout;
    out << '[' << to_string(level) << "] [" << subsystem << "] " << message << '\n';
}

} // namespace aether::core
