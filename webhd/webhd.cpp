#include "webhd.h"

#include <memory>

#include <hddll/hd.h>
#include <imgui.h>

#include "ui/target_texture.h"

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static Settings gSettings;
static WebSocketClient *gClient = nullptr;
static std::unique_ptr<GameMode> gActiveMode;
static bool gShowSettings = false;
static bool gShowBrowser = false;
static bool gShowManage = false;
static bool gShowLogs = false;
static bool gPrivateLobby = false;

// ---------------------------------------------------------------------------
// Connection helpers
// ---------------------------------------------------------------------------

static void startMode(const char *modeId) {
  gActiveMode = createGameMode(modeId);
  if (!gActiveMode)
    return;

  if (!gClient)
    gClient = new WebSocketClient();
  gClient->setPrivate(gPrivateLobby);
  gClient->connect(gSettings.server, gSettings.apiToken, gActiveMode->info().id);
}

static void stopMode() {
  if (gClient) {
    gClient->disconnect();
  }
  gActiveMode.reset();
  gShowManage = false;
}

// ---------------------------------------------------------------------------
// HDDLL callbacks
// ---------------------------------------------------------------------------

namespace hddll {

void onInit() { loadSettings(gSettings); }

void onDestroy() {
  ui::destroyTargetTexture();
  gActiveMode.reset();
  if (gClient) {
    gClient->disconnect();
    delete gClient;
    gClient = nullptr;
  }
}

void onFrame() {
  if (!gGlobalState || !gCameraState)
    return;

  // Show cursor when ImGui wants mouse input
  ImGui::GetIO().MouseDrawCursor = ImGui::GetIO().WantCaptureMouse;

  // --- Process incoming messages ---
  if (gClient && gActiveMode) {
    while (auto msg = gClient->popIncoming()) {
      gActiveMode->handleMessage(*msg);
    }
    gActiveMode->onFrame(*gClient);
  }

  // --- Main menu bar ---
  if (ImGui::BeginMainMenuBar()) {
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Web HD");
    ImGui::Separator();

    if (gActiveMode) {
      if (ImGui::MenuItem("Manage Online")) {
        gShowManage = !gShowManage;
      }
      ImGui::Separator();
      ImGui::Text("%s", gActiveMode->info().name);
    } else {
      if (ImGui::MenuItem("Play Online")) {
        gShowBrowser = !gShowBrowser;
      }
    }

    // Right-aligned status + settings
    float rightWidth = 0;
    {
      const char *statusText = "";
      if (gClient && gActiveMode) {
        statusText = gClient->isInLobby() ? "In Lobby" : connectionStateName(gClient->state());
      }
      auto username = gClient ? gClient->username() : std::string();
      float statusW = 0;
      if (gClient && gActiveMode) {
        statusW = ImGui::CalcTextSize(statusText).x + 8;
        if (!username.empty())
          statusW += ImGui::CalcTextSize(username.c_str()).x + 12;
      }
      float settingsW = ImGui::CalcTextSize("Settings").x + 16;
      float logsW = ImGui::CalcTextSize("Logs").x + 16;
      rightWidth = statusW + settingsW + logsW;

      ImGui::SameLine(ImGui::GetWindowWidth() - rightWidth);

      if (gClient && gActiveMode) {
        ImVec4 col = gClient->isInLobby() ? ImVec4(0, 1, 0.5f, 1) : ImVec4(1, 1, 0, 1);
        ImGui::TextColored(col, "%s", statusText);
        if (!username.empty()) {
          ImGui::SameLine();
          ImGui::Text("%s", username.c_str());
        }
        ImGui::SameLine();
      }

      if (ImGui::MenuItem("Settings")) {
        gShowSettings = !gShowSettings;
      }
      if (ImGui::MenuItem("Logs")) {
        gShowLogs = !gShowLogs;
      }
    }

    ImGui::EndMainMenuBar();
  }

  // --- Windows ---
  ui::drawSettingsWindow(&gShowSettings, gSettings);
  ui::drawLogWindow(&gShowLogs);

  // Game mode browser
  const char *selectedMode = nullptr;
  ui::drawGameModeBrowser(&gShowBrowser, &selectedMode, &gPrivateLobby);
  if (selectedMode) {
    startMode(selectedMode);
    gShowManage = true;
  }

  // Manage window for active mode
  if (gActiveMode && gShowManage) {
    ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Manage", &gShowManage)) {
      gActiveMode->drawManageUI(*gClient);
    }
    ImGui::End();

    // If mode disconnected via drawManageUI, clean up
    if (gClient && gClient->state() == ConnectionState::Disconnected && gActiveMode) {
      gActiveMode.reset();
      gShowManage = false;
    }
  }

  // --- Toast notifications (always drawn, on top of everything) ---
  ui::drawToasts(ImGui::GetIO().DeltaTime);
}

} // namespace hddll
