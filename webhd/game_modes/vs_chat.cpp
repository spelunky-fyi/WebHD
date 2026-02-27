#include "vs_chat.h"

#include "../net.h"
#include "../ui/log.h"
#include "../ui/toast.h"

#include <unordered_map>
#include <variant>

#include <hddll/hd.h>
#include <hddll/hd_entity.h>
#include <hddll/hddll.h>
#include <imgui.h>

namespace game_modes {

// ---------------------------------------------------------------------------
// Entity ID mapping for interactions
// ---------------------------------------------------------------------------

static const std::unordered_map<std::string, uint32_t> kInteractionEntities = {
    {"spawn_web", 115},     {"spawn_snake", 1001},    {"spawn_bat", 1003},
    {"spawn_spider", 1002}, {"spawn_skeleton", 1012}, {"spawn_rope", 108},
    {"spawn_bomb", 107},
};

// ---------------------------------------------------------------------------
// VsChatMode
// ---------------------------------------------------------------------------

struct EventLogEntry {
  std::string text;
};

static constexpr size_t kMaxEventLog = 50;

class VsChatMode : public GameMode {
public:
  const GameModeInfo &info() const override {
    static GameModeInfo i = {"vs_chat", "vs Chat",
                             "Chat viewers interact with your Spelunky run."};
    return i;
  }

  void onFrame(WebSocketClient &client) override {
    if (!client.isInLobby())
      return;

    bool has_player =
        hddll::gGlobalState && hddll::gGlobalState->player1 != nullptr;
    bool has_level =
        hddll::gGlobalState && hddll::gGlobalState->level_state != nullptr;
    bool in_level = has_player && has_level;

    if (in_level) {
      // Detect level transition and send level data
      uint32_t currentLevel = hddll::gGlobalState->level;
      if (currentLevel != cachedLevel_) {
        cachedLevel_ = currentLevel;
        ui::logDebug("Level changed to " + std::to_string(currentLevel) +
                     ", sending level data");
        sendLevelData(client);
        wasInLevel_ = true;
      }

      // Send position (throttled inside client)
      client.sendPosition(hddll::gGlobalState->player1->x,
                          hddll::gGlobalState->player1->y);
    } else if (wasInLevel_) {
      // Transitioned out of level — send clear
      client.sendLevelClear();
      cachedLevel_ = 0;
      wasInLevel_ = false;
    }
  }

  void handleMessage(const IncomingMsg &msg) override {
    if (auto *ei = std::get_if<ExecuteInteraction>(&msg)) {
      executeInteraction(*ei);
    } else if (auto *vc = std::get_if<ViewerCountUpdate>(&msg)) {
      viewerCount_ = vc->count;
    } else if (auto *mj = std::get_if<MemberJoinedMsg>(&msg)) {
      ui::showToast(mj->username + " joined as " + mj->role);
    } else if (auto *ml = std::get_if<MemberLeftMsg>(&msg)) {
      ui::showToast(ml->username + " left");
    }
  }

  void drawManageUI(WebSocketClient &client) override {
    if (client.isInLobby()) {
      ImGui::Text("Lobby: %s", client.lobbyId().c_str());
      ImGui::Text("Viewers: %zu", viewerCount_);
    }

    auto err = client.lastError();
    if (!err.empty()) {
      ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", err.c_str());
    }

    // Event log
    if (!eventLog_.empty()) {
      ImGui::Separator();
      ImGui::Text("Recent Events:");
      ImGui::BeginChild("EventLog", ImVec2(0, 120), true);
      for (auto &e : eventLog_) {
        ImGui::TextWrapped("%s", e.text.c_str());
      }
      ImGui::SetScrollHereY(1.0f);
      ImGui::EndChild();
    }

    ImGui::Separator();
    if (ImGui::Button("Disconnect")) {
      client.disconnect();
    }
  }

private:
  uint32_t cachedLevel_ = 0;
  bool wasInLevel_ = false;
  size_t viewerCount_ = 0;
  std::deque<EventLogEntry> eventLog_;

  void addLogEntry(const std::string &text) {
    eventLog_.push_back({text});
    if (eventLog_.size() > kMaxEventLog)
      eventLog_.pop_front();
  }

  void executeInteraction(const ExecuteInteraction &ei) {
    if (!hddll::gGlobalState || !hddll::gGlobalState->player1)
      return;

    auto *player = hddll::gGlobalState->player1;

    if (ei.interaction_id == "stun_player") {
      hddll::gGlobalState->SpawnEntity(player->x, player->y, 115, true);
      addLogEntry(ei.username + " stunned the player!");
      return;
    }

    auto it = kInteractionEntities.find(ei.interaction_id);
    if (it == kInteractionEntities.end()) {
      ui::logWarn("Unknown interaction: " + ei.interaction_id);
      return;
    }

    float x = ei.has_coords ? ei.x : player->x;
    float y = ei.has_coords ? ei.y : player->y;

    auto *entity = static_cast<hddll::EntityActive *>(
        hddll::gGlobalState->SpawnEntity(x, y, it->second, true));

    if (entity && ei.has_velocity) {
      entity->velocity_x = ei.vx;
      entity->velocity_y = ei.vy;
    }

    std::string coords_str;
    if (ei.has_coords) {
      coords_str = " at (" + std::to_string((int)x) + ", " +
                   std::to_string((int)y) + ")";
    }
    addLogEntry(ei.username + " used " + ei.interaction_id + coords_str);
  }

  void sendLevelData(WebSocketClient &client) {
    if (!client.isInLobby())
      return;
    if (!hddll::gGlobalState || !hddll::gGlobalState->level_state)
      return;

    constexpr uint32_t width = 46;
    constexpr uint32_t height = hddll::ENTITY_FLOORS_COUNT / 46;
    // Pack u16 entity types as little-endian bytes (2 bytes per tile)
    std::vector<uint8_t> tiles(hddll::ENTITY_FLOORS_COUNT * 2);

    auto *ls = hddll::gGlobalState->level_state;
    for (size_t i = 0; i < hddll::ENTITY_FLOORS_COUNT; i++) {
      uint16_t entity_type = 0;
      if (ls->entity_floors[i] != nullptr) {
        entity_type = static_cast<uint16_t>(ls->entity_floors[i]->entity_type);
      }
      tiles[i * 2] = static_cast<uint8_t>(entity_type & 0xFF);
      tiles[i * 2 + 1] = static_cast<uint8_t>((entity_type >> 8) & 0xFF);
    }

    client.sendLevelData(width, height, tiles);
  }
};

std::unique_ptr<GameMode> createVsChatMode() {
  return std::make_unique<VsChatMode>();
}

} // namespace game_modes
