#pragma once

#include <cstddef>
#include <deque>
#include <mutex>
#include <string>

namespace ui {

enum class LogLevel { Debug, Info, Warn, Error };

struct LogEntry {
  LogLevel level;
  std::string text;
};

// Thread-safe. Can be called from any thread (e.g. websocket callbacks).
void log(LogLevel level, const std::string &text);

inline void logDebug(const std::string &text) { log(LogLevel::Debug, text); }
inline void logInfo(const std::string &text) { log(LogLevel::Info, text); }
inline void logWarn(const std::string &text) { log(LogLevel::Warn, text); }
inline void logError(const std::string &text) { log(LogLevel::Error, text); }

void drawLogWindow(bool *open);

} // namespace ui
