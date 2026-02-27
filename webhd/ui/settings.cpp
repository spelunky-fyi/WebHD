#include "settings.h"

#include <cstring>
#include <fstream>

#include <imgui.h>
#include <nlohmann/json.hpp>

static const char *kSettingsFile = "webhd-settings.json";

void loadSettings(Settings &s) {
  std::ifstream f(kSettingsFile);
  if (!f.is_open())
    return;

  try {
    auto j = nlohmann::json::parse(f);
    if (j.contains("server") && j["server"].is_string()) {
      std::string sv = j["server"];
      std::strncpy(s.server, sv.c_str(), sizeof(s.server) - 1);
      s.server[sizeof(s.server) - 1] = '\0';
    }
    if (j.contains("apiToken") && j["apiToken"].is_string()) {
      std::string tk = j["apiToken"];
      std::strncpy(s.apiToken, tk.c_str(), sizeof(s.apiToken) - 1);
      s.apiToken[sizeof(s.apiToken) - 1] = '\0';
    }
  } catch (...) {
  }
}

void saveSettings(const Settings &s) {
  nlohmann::ordered_json j;
  j["server"] = s.server;
  j["apiToken"] = s.apiToken;

  std::ofstream f(kSettingsFile);
  if (f.is_open())
    f << j.dump(2);
}

namespace ui {

void drawSettingsWindow(bool *open, Settings &s) {
  if (!*open)
    return;

  ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Settings", open)) {
    ImGui::End();
    return;
  }

  bool changed = false;
  changed |= ImGui::InputText("Server", s.server, sizeof(s.server));
  ImGui::SameLine();
  if (ImGui::Button("Default")) {
    std::strncpy(s.server, kDefaultServer, sizeof(s.server) - 1);
    s.server[sizeof(s.server) - 1] = '\0';
    changed = true;
  }

  static bool showToken = false;
  ImGuiInputTextFlags flags =
      showToken ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_Password;
  changed |=
      ImGui::InputText("API Token", s.apiToken, sizeof(s.apiToken), flags);
  ImGui::SameLine();
  ImGui::Checkbox("Show", &showToken);

  if (changed)
    saveSettings(s);

  ImGui::End();
}

} // namespace ui
