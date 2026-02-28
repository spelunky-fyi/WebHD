#include "game_mode.h"

#include "game_modes/vs_chat.h"

#include <cstring>

static const std::vector<GameModeInfo> kModes = {
    {"vs_chat", "vs Chat",
     "Chat viewers interact with your Spelunky run by spawning enemies, items, "
     "and traps in real time."},
};

const std::vector<GameModeInfo> &getAvailableGameModes() { return kModes; }

std::unique_ptr<GameMode> createGameMode(const char *modeId) {
  if (std::strcmp(modeId, "vs_chat") == 0)
    return game_modes::createVsChatMode();
  return nullptr;
}
