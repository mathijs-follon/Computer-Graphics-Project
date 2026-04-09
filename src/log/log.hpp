#ifndef CG_OPENGL_PROJECT_LOG_HPP
#define CG_OPENGL_PROJECT_LOG_HPP
#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "world/registry.hpp"

#define LOG_INFO(...) ::logger::info(__VA_ARGS__)
#define LOG_DEBUG(...) ::logger::debug(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) ::logger::warn(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) ::logger::error(__FILE__, __LINE__, __VA_ARGS__)
#define LOG_CRITICAL(...) ::logger::critical(__FILE__, __LINE__, __VA_ARGS__)

namespace logger {
std::shared_ptr<spdlog::logger> create_rotating_logger(std::string logger_name,
                                                       std::string file_path,
                                                       std::size_t max_file_size_bytes,
                                                       std::size_t max_files);

void set_default_logger(std::shared_ptr<spdlog::logger> logger);
std::shared_ptr<spdlog::logger> default_logger();
void flush();
void flush_if_needed(bool has_spare_frame_time);
[[nodiscard]] std::size_t pending_message_count() noexcept;
void mark_message_emitted() noexcept;

template <typename... Args>
void info(const std::string_view fmt, Args&&... args) {
    auto lg = default_logger();
    if (!lg) {
        return;
    }
    lg->log(spdlog::level::info, fmt::runtime(fmt), std::forward<Args>(args)...);
    mark_message_emitted();
    flush_if_needed(false);
}

template <typename... Args>
void debug(const char* file, int line, const std::string_view fmt, Args&&... args) {
    auto lg = default_logger();
    if (!lg) {
        return;
    }
    lg->log(spdlog::level::debug, "[{}:{}] {}", file, line,
            fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    mark_message_emitted();
    flush_if_needed(false);
}

template <typename... Args>
void warn(const char* file, int line, const std::string_view fmt, Args&&... args) {
    auto lg = default_logger();
    if (!lg) {
        return;
    }
    lg->log(spdlog::level::warn, "[{}:{}] {}", file, line,
            fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    mark_message_emitted();
    flush_if_needed(false);
}

template <typename... Args>
void error(const char* file, int line, const std::string_view fmt, Args&&... args) {
    auto lg = default_logger();
    if (!lg) {
        return;
    }
    lg->log(spdlog::level::err, "[{}:{}] {}", file, line,
            fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    mark_message_emitted();
    flush_if_needed(false);
}

template <typename... Args>
void critical(const char* file, int line, const std::string_view fmt, Args&&... args) {
    auto lg = default_logger();
    if (!lg) {
        return;
    }
    lg->log(spdlog::level::critical, "[{}:{}] {}", file, line,
            fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...));
    mark_message_emitted();
    flush_if_needed(false);
}

inline void loggerSetupSystem(Registry&) {
    auto name = "cg";
    auto file_path = "cg_openGL_project.log";
    size_t max_file_size_bytes = 5 * 1024 * 1024;
    size_t max_files = 3;

    if (const auto default_logger = logger::default_logger();
        default_logger && default_logger->name().empty()) {
        auto logger = create_rotating_logger(name, file_path, max_file_size_bytes, max_files);
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::warn);
        logger::set_default_logger(std::move(logger));
    }
    LOG_INFO("Logger is set up. Using name: '{}', file: '{}', max file size (b): {}, max files: {}",
             name, file_path, max_file_size_bytes, max_files);
}
}  // namespace logger

#endif  // CG_OPENGL_PROJECT_LOG_HPP
