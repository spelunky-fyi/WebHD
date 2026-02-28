#include "vs_chat.h"

#include "../interactions.h"
#include "../net.h"
#include "../ui/log.h"
#include "../ui/toast.h"

#include <chrono>
#include <optional>
#include <random>
#include <variant>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <hddll/hd.h>
#include <hddll/hd_entity.h>
#include <hddll/hddll.h>
#include <imgui.h>

namespace game_modes {

// ---------------------------------------------------------------------------
// Interaction catalog — single source of truth for names, costs, and entities
// ---------------------------------------------------------------------------

struct CatalogDef {
  CatalogInteraction info;
  uint32_t entity_id = 0; // 0 = handled specially (not a simple spawn)
};

static const std::vector<CatalogDef> kCatalog = {
    {
        .info =
            {
                .id = "spawn_web",
                .name = "Spawn Web",
                .cost = 10,
                .requires_coords = true,
            },
        .entity_id = 115,
    },
    {
        .info =
            {
                .id = "spawn_snake",
                .name = "Spawn Snake",
                .cost = 15,
                .requires_coords = true,
            },
        .entity_id = 1001,
    },
    {
        .info =
            {
                .id = "spawn_bat",
                .name = "Spawn Bat",
                .cost = 15,
                .requires_coords = true,
            },
        .entity_id = 1003,
    },
    {
        .info =
            {
                .id = "spawn_spider",
                .name = "Spawn Spider",
                .cost = 25,
                .requires_coords = true,
            },
        .entity_id = 1002,
    },
    {
        .info =
            {
                .id = "spawn_skeleton",
                .name = "Spawn Skeleton",
                .cost = 35,
                .requires_coords = true,
            },
        .entity_id = 1012,
    },
    {
        .info =
            {
                .id = "web_storm",
                .name = "Web Storm",
                .cost = 60,
            },
    },
    {
        .info =
            {
                .id = "stun_player",
                .name = "Stun Player",
                .cost = 40,
            },
    },
    {
        .info =
            {
                .id = "spawn_rope",
                .name = "Gift Rope",
                .cost = 20,
                .requires_coords = true,
            },
        .entity_id = 108,
    },
    {
        .info = {.id = "spawn_bomb",
                 .name = "Spawn Bomb",
                 .cost = 50,
                 .requires_coords = true,
                 .allows_velocity = true},
        .entity_id = 107,
    },
    {
        .info = {.id = "spawn_gold",
                 .name = "Spawn Gold",
                 .cost = 5,
                 .requires_coords = true,
                 .allows_velocity = true},
        .entity_id = 118,
    },
    {
        .info = {.id = "spawn_arrow",
                 .name = "Spawn Arrow",
                 .cost = 15,
                 .requires_coords = true,
                 .allows_velocity = true},
        .entity_id = 122,
    },
    {
        .info =
            {
                .id = "hired_hell",
                .name = "Hired Hell",
                .cost = 200,
                .requires_coords = true,
            },
    },
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
    static GameModeInfo i = {"vs_chat", "vs Chat", "Chat viewers interact with your Spelunky run."};
    return i;
  }

  void onFrame(WebSocketClient &client) override {
    if (!client.isInLobby()) {
      catalogSent_ = false;
      return;
    }

    // Send the interaction catalog once after joining
    if (!catalogSent_) {
      std::vector<CatalogInteraction> interactions;
      interactions.reserve(kCatalog.size());
      for (const auto &def : kCatalog) {
        interactions.push_back(def.info);
      }
      client.sendCatalog(interactions, 1, 200);
      catalogSent_ = true;
    }

    bool has_player = hddll::gGlobalState && hddll::gGlobalState->player1 != nullptr;
    bool has_level = hddll::gGlobalState && hddll::gGlobalState->level_state != nullptr;
    bool in_level = has_player && has_level;

    if (in_level) {
      // Detect level transition and send level data
      uint32_t currentLevel = hddll::gGlobalState->level;
      if (currentLevel != cachedLevel_) {
        cachedLevel_ = currentLevel;
        cachedFloorHash_ = computeFloorHash();
        ui::logDebug("Level changed to " + std::to_string(currentLevel) + ", sending level data");
        sendLevelData(client);
        wasInLevel_ = true;
      }

      // Periodically check for floor changes (destroyed tiles, etc.)
      auto now = std::chrono::steady_clock::now();
      if (now - lastFloorCheck_ >= std::chrono::milliseconds(500)) {
        lastFloorCheck_ = now;
        size_t hash = computeFloorHash();
        if (hash != cachedFloorHash_) {
          cachedFloorHash_ = hash;
          sendLevelData(client);
        }
      }

      // Send position (throttled inside client)
      client.sendPosition(hddll::gGlobalState->player1->x, hddll::gGlobalState->player1->y);
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
      ImGui::SameLine();
      if (ImGui::Button("Copy Link")) {
        std::string url = "https://webhd.mossranking.com/lobby/" + client.lobbyId();
        if (OpenClipboard(nullptr)) {
          EmptyClipboard();
          HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, url.size() + 1);
          if (hg) {
            memcpy(GlobalLock(hg), url.c_str(), url.size() + 1);
            GlobalUnlock(hg);
            SetClipboardData(CF_TEXT, hg);
          }
          CloseClipboard();
        }
      }
      ImGui::Text("Viewers: %zu", viewerCount_);
    }

    auto err = client.lastError();
    if (!err.empty()) {
      ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", err.c_str());
    }

    if (client.forceJoinAvailable()) {
      if (ImGui::Button("Kick existing connection & rejoin")) {
        client.forceJoinLobby();
      }
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

    // Debug interaction panel — fire interactions without a web client
    {
      bool has_player = hddll::gGlobalState && hddll::gGlobalState->player1 != nullptr;
      bool has_level = hddll::gGlobalState && hddll::gGlobalState->level_state != nullptr;
      if (has_player && has_level && ImGui::CollapsingHeader("Debug Interactions")) {
        float maxX = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        auto &style = ImGui::GetStyle();
        for (size_t i = 0; i < kCatalog.size(); i++) {
          const auto &def = kCatalog[i];
          if (ImGui::Button(def.info.name.c_str())) {
            ExecuteInteraction ei;
            ei.interaction_id = def.info.id;
            ei.username = "[debug]";
            if (def.info.requires_coords) {
              auto tile = findOpenTileNearPlayer();
              if (!tile) {
                addLogEntry("[debug] No open tile found for " + def.info.id);
              } else {
                ei.x = tile->first;
                ei.y = tile->second;
                ei.has_coords = true;
                executeInteraction(ei);
              }
            } else {
              executeInteraction(ei);
            }
          }
          if (i + 1 < kCatalog.size()) {
            float nextWidth = ImGui::CalcTextSize(kCatalog[i + 1].info.name.c_str()).x +
                              style.FramePadding.x * 2.0f + style.ItemSpacing.x;
            if (ImGui::GetItemRectMax().x + nextWidth < maxX)
              ImGui::SameLine();
          }
        }
      }
    }

    ImGui::Separator();
    if (ImGui::Button("Disconnect")) {
      client.disconnect();
    }
  }

private:
  uint32_t cachedLevel_ = 0;
  bool wasInLevel_ = false;
  bool catalogSent_ = false;
  size_t viewerCount_ = 0;
  std::deque<EventLogEntry> eventLog_;
  size_t cachedFloorHash_ = 0;
  std::chrono::steady_clock::time_point lastFloorCheck_;

  size_t computeFloorHash() {
    if (!hddll::gGlobalState || !hddll::gGlobalState->level_state)
      return 0;
    auto *ls = hddll::gGlobalState->level_state;
    size_t hash = 0;
    for (size_t i = 0; i < hddll::ENTITY_FLOORS_COUNT; i++) {
      uint16_t t =
          ls->entity_floors[i] ? static_cast<uint16_t>(ls->entity_floors[i]->entity_type) : 0;
      hash ^= std::hash<uint16_t>{}(t) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
    return hash;
  }

  void addLogEntry(const std::string &text) {
    eventLog_.push_back({text});
    if (eventLog_.size() > kMaxEventLog)
      eventLog_.pop_front();
  }

  std::optional<std::pair<float, float>> findOpenTileNearPlayer() {
    if (!hddll::gGlobalState || !hddll::gGlobalState->player1 || !hddll::gGlobalState->level_state)
      return std::nullopt;

    auto *player = hddll::gGlobalState->player1;
    auto *ls = hddll::gGlobalState->level_state;
    int px = (int)player->x;
    int py = (int)player->y;

    static constexpr std::pair<int, int> offsets[] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1},
    };
    constexpr int height = (int)(hddll::ENTITY_FLOORS_COUNT / 46);

    for (auto [dx, dy] : offsets) {
      int tx = px + dx;
      int ty = py + dy;
      if (tx < 0 || tx >= 46 || ty < 0 || ty >= height)
        continue;
      if (ls->entity_floors[(size_t)ty * 46 + (size_t)tx] == nullptr)
        return std::pair{(float)tx, (float)ty};
    }
    return std::nullopt;
  }

  void executeInteraction(const ExecuteInteraction &ei) {
    if (!hddll::gGlobalState || !hddll::gGlobalState->player1)
      return;

    auto *player = hddll::gGlobalState->player1;

    if (ei.interaction_id == "web_storm") {
      spawnWebStorm(player->x, player->y);
      addLogEntry(ei.username + " used Web Storm!");
      ui::showToast(ei.username + " used Web Storm!");
      return;
    }

    if (ei.interaction_id == "stun_player") {
      static std::mt19937 rng{std::random_device{}()};
      static std::uniform_real_distribution<float> dist(0.1f, 0.3f);
      static std::bernoulli_distribution coin(0.5);
      player->stun_timer = 200;
      player->stunned = 1;
      player->velocity_x = coin(rng) ? dist(rng) : -dist(rng);
      player->velocity_y = dist(rng);
      addLogEntry(ei.username + " stunned the player!");
      ui::showToast(ei.username + " stunned the player!");
      return;
    }

    if (ei.interaction_id == "hired_hell") {
      float x = ei.has_coords ? ei.x : player->x;
      float y = ei.has_coords ? ei.y : player->y;

      // Use the first hired-hand texture from player data, fallback to 30
      uint32_t texId = 30;
      if (player->player_data && player->player_data->hh_texture_id[0] != 0)
        texId = player->player_data->hh_texture_id[0];

      auto *hand = hddll::gGlobalState->SpawnHiredHand(x, y, texId);
      auto *shield =
          static_cast<hddll::EntityActive *>(hddll::gGlobalState->SpawnEntity(x, y, 523, true));

      if (hand && shield) {
        hand->holding_entity = shield;
        shield->holder_entity = hand;
      }

      addLogEntry(ei.username + " unleashed Hired Hell!");
      ui::showToast(ei.username + " unleashed Hired Hell!");
      return;
    }

    // Look up entity ID from catalog
    uint32_t entityId = 0;
    for (const auto &def : kCatalog) {
      if (def.info.id == ei.interaction_id) {
        entityId = def.entity_id;
        break;
      }
    }
    if (entityId == 0) {
      ui::logWarn("Unknown spawn interaction: " + ei.interaction_id);
      return;
    }

    float x = ei.has_coords ? ei.x : player->x;
    float y = ei.has_coords ? ei.y : player->y;

    auto *entity =
        static_cast<hddll::EntityActive *>(hddll::gGlobalState->SpawnEntity(x, y, entityId, true));

    if (entity && ei.has_velocity) {
      entity->velocity_x = ei.vx;
      entity->velocity_y = ei.vy;
    }

    std::string coords_str;
    if (ei.has_coords) {
      coords_str = " at (" + std::to_string((int)x) + ", " + std::to_string((int)y) + ")";
    }
    addLogEntry(ei.username + " used " + ei.interaction_id + coords_str);
    ui::showToast(ei.username + " used " + ei.interaction_id);
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

std::unique_ptr<GameMode> createVsChatMode() { return std::make_unique<VsChatMode>(); }

} // namespace game_modes
