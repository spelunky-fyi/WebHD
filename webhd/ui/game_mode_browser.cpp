#include "game_mode_browser.h"

#include "../game_mode.h"

#include <imgui.h>

namespace ui {

void drawGameModeBrowser(bool *open, const char **selectedModeId) {
  if (!*open)
    return;

  ImGui::SetNextWindowSize(ImVec2(420, 0), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Play Online", open)) {
    ImGui::End();
    return;
  }

  for (auto &mode : getAvailableGameModes()) {
    ImGui::PushID(mode.id);

    // Card-style bordered box
    ImGui::BeginGroup();
    ImGui::Text("%s", mode.name);
    ImGui::TextWrapped("%s", mode.description);
    if (ImGui::Button("Start")) {
      *selectedModeId = mode.id;
      *open = false;
    }
    ImGui::EndGroup();

    // Draw border around the card
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    min.x -= 4;
    min.y -= 4;
    max.x += 4;
    max.y += 4;
    ImGui::GetWindowDrawList()->AddRect(
        min, max, ImGui::GetColorU32(ImGuiCol_Border), 4.0f);
    ImGui::Dummy(ImVec2(0, 4));

    ImGui::PopID();
  }

  ImGui::End();
}

} // namespace ui
