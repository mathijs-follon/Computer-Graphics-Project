#include "log/log.hpp"

#include <atomic>
#include <cstddef>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <vector>
namespace logger {
namespace {
std::atomic<std::size_t> g_messages_since_flush{0};
constexpr std::size_t k_flush_message_threshold = 48;
}  // namespace

std::shared_ptr<spdlog::logger> create_rotating_logger(std::string logger_name,
                                                       std::string file_path,
                                                       const std::size_t max_file_size_bytes,
                                                       const std::size_t max_files) {
    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        std::move(file_path), max_file_size_bytes, max_files));
    auto lg = std::make_shared<spdlog::logger>(std::move(logger_name), sinks.begin(), sinks.end());
    spdlog::initialize_logger(lg);
    return lg;
}

void set_default_logger(std::shared_ptr<spdlog::logger> logger) {
    spdlog::set_default_logger(std::move(logger));
}

std::shared_ptr<spdlog::logger> default_logger() {
    return spdlog::default_logger();
}

void flush() {
    if (const auto lg = default_logger()) {
        lg->flush();
    }
    g_messages_since_flush.store(0, std::memory_order_relaxed);
}

void flush_if_needed(const bool has_spare_frame_time) {
    const auto n = g_messages_since_flush.load(std::memory_order_relaxed);
    if (n == 0) {
        return;
    }
    if (!has_spare_frame_time && n < k_flush_message_threshold) {
        return;
    }
    flush();
}

std::size_t pending_message_count() noexcept {
    return g_messages_since_flush.load(std::memory_order_relaxed);
}

void mark_message_emitted() noexcept {
    g_messages_since_flush.fetch_add(1, std::memory_order_relaxed);
}
}  // namespace logger