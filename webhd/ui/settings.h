#pragma once

inline constexpr const char *kDefaultServer = "wss://webhd.mossranking.com/ws";

struct Settings {
  char server[256] = "wss://webhd.mossranking.com/ws";
  char apiToken[256] = "";
};

void loadSettings(Settings &s);
void saveSettings(const Settings &s);

namespace ui {
void drawSettingsWindow(bool *open, Settings &s);
} // namespace ui
