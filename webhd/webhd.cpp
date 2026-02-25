#include "webhd.h"

#include <imgui.h>

namespace hddll {

void onInit() {}

void onDestroy() {}

void onFrame() {
  if (!gGlobalState || !gCameraState)
    return;

  ImGuiIO &io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2{0.f, 0.f});
  ImGui::SetNextWindowSize(io.DisplaySize);
  ImGui::SetNextWindowBgAlpha(0.f);
  ImGui::Begin("Web HD", nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoCollapse |
                   ImGuiWindowFlags_NoBringToFrontOnFocus |
                   ImGuiWindowFlags_NoScrollbar |
                   ImGuiWindowFlags_NoInputs);
  ImGui::End();
}

} // namespace hddll
