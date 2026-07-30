/**
 * @file engine_context.cpp
 * @brief EngineContext Lebenszyklus.
 */
#include <aether/core/engine_context.hpp>
#include <aether/core/assert.hpp>

#include <stdexcept>
#include <string>

#if defined(__has_include)
#  if __has_include(<aether/version.hpp>)
#    include <aether/version.hpp>
#  endif
#endif

#ifndef AETHER_VERSION_STRING
#  define AETHER_VERSION_STRING "0.1.0-dev"
#endif

namespace aether::core {
namespace {

EngineContext* g_active_context = nullptr;

} // namespace

EngineContext::EngineContext(EngineConfig config)
    : config_(std::move(config)) {}

std::unique_ptr<EngineContext> EngineContext::create(EngineConfig config) {
    auto ctx = std::unique_ptr<EngineContext>(new EngineContext(std::move(config)));
    ctx->setup_logging();

    const f64 fixed = (ctx->config_.graphics.frame_rate > 0)
                          ? (1.0 / static_cast<f64>(ctx->config_.graphics.frame_rate))
                          : (1.0 / 60.0);
    ctx->time_ = std::make_unique<TimeSystem>(fixed);
    ctx->thread_pool_ = std::make_unique<ThreadPool>(ctx->config_.worker_threads);
    ctx->events_ = std::make_unique<EventBus>();

    set_active(ctx.get());
    set_active_logger(ctx->logger_.get());

    log_info("Core", std::string("Aether Engine ") + AETHER_VERSION_STRING +
                         " booting (" + to_string(ctx->config_.mode) + ")");
    return ctx;
}

EngineContext::~EngineContext() {
    if (state_ == ContextState::Running || state_ == ContextState::Created) {
        shutdown();
    }
    if (g_active_context == this) {
        g_active_context = nullptr;
    }
}

void EngineContext::setup_logging() {
    logger_ = std::make_unique<Logger>();
    logger_->set_level(log_level_from_string(config_.log.level));

#if AETHER_DEBUG
    // In Debug mindestens Debug-Level, sofern User nicht "trace" will
    if (logger_->level() > LogLevel::Debug &&
        log_level_from_string(config_.log.level) == LogLevel::Info) {
        logger_->set_level(LogLevel::Debug);
    }
#endif

    if (config_.log.console) {
        logger_->add_sink(std::make_shared<ConsoleSink>());
    }
    if (config_.log.file) {
        const auto path = Logger::default_log_path(config_.paths.logs_dir.string());
        auto file_sink = std::make_shared<FileSink>(path);
        if (file_sink->is_open()) {
            logger_->add_sink(std::move(file_sink));
        } else {
            // Console-Fallback-Hinweis über temporären Sink
            if (!config_.log.console) {
                logger_->add_sink(std::make_shared<ConsoleSink>());
            }
            logger_->log(LogLevel::Warn, "Core",
                         "Could not open log file: " + path);
        }
    }
}

void EngineContext::start() {
    AETHER_ASSERT(state_ == ContextState::Created || state_ == ContextState::Stopped);
    state_ = ContextState::Running;
    time_->reset();
    set_active(this);
    set_active_logger(logger_.get());
    events_->publish(EngineBootEvent{config_.mode});
    log_info("Core", "EngineContext started");
}

void EngineContext::shutdown() {
    if (state_ == ContextState::Stopped || state_ == ContextState::Stopping) {
        return;
    }
    state_ = ContextState::Stopping;
    log_info("Core", "EngineContext shutting down…");

    if (events_) {
        events_->publish(EngineShutdownEvent{});
        events_->clear();
    }
    if (thread_pool_) {
        thread_pool_->shutdown();
    }
    if (logger_) {
        logger_->flush();
    }

    state_ = ContextState::Stopped;
    log_info("Core", "EngineContext stopped");
    if (logger_) {
        logger_->flush();
    }
}

bool EngineContext::pump_frame() {
    if (state_ != ContextState::Running) {
        return false;
    }
    const f64 dt = time_->update();
    const u64 frame = time_->frame_count();
    events_->publish(FrameBeginEvent{dt, frame});
    events_->publish(FrameEndEvent{dt, frame});
    return true;
}

void EngineContext::set_active(EngineContext* ctx) noexcept {
    g_active_context = ctx;
}

EngineContext* EngineContext::active() noexcept {
    return g_active_context;
}

} // namespace aether::core
