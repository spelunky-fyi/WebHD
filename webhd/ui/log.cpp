#include "log.h"

#include <imgui.h>

namespace ui {

static constexpr size_t kMaxLogEntries = 200;
static std::mutex gLogMutex;
static std::deque<LogEntry> gLogEntries;

void log(LogLevel level, const std::string &text) {
  std::lock_guard<std::mutex> lock(gLogMutex);
  gLogEntries.push_back({level, text});
  if (gLogEntries.size() > kMaxLogEntries)
    gLogEntries.pop_front();
}

static const char *levelLabel(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return "[DEBUG]";
  case LogLevel::Info:
    return "[INFO]";
  case LogLevel::Warn:
    return "[WARN]";
  case LogLevel::Error:
    return "[ERROR]";
  }
  return "[?]";
}

static ImVec4 levelColor(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
  case LogLevel::Info:
    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  case LogLevel::Warn:
    return ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
  case LogLevel::Error:
    return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
  }
  return ImVec4(1, 1, 1, 1);
}

void drawLogWindow(bool *open) {
  if (!*open)
    return;

  ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Logs", open)) {
    ImGui::End();
    return;
  }

  if (ImGui::Button("Clear")) {
    std::lock_guard<std::mutex> lock(gLogMutex);
    gLogEntries.clear();
  }

  ImGui::Separator();
  ImGui::BeginChild("LogScroll", ImVec2(0, 0), false,
                     ImGuiWindowFlags_HorizontalScrollbar);

  {
    std::lock_guard<std::mutex> lock(gLogMutex);
    for (auto &entry : gLogEntries) {
      ImVec4 col = levelColor(entry.level);
      ImGui::TextColored(col, "%-7s %s", levelLabel(entry.level),
                         entry.text.c_str());
    }
  }

  if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    ImGui::SetScrollHereY(1.0f);

  ImGui::EndChild();
  ImGui::End();
}

} // namespace ui
