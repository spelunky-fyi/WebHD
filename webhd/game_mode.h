#pragma once

#include "net.h"

#include <memory>
#include <vector>

struct GameModeInfo {
  const char *id;
  const char *name;
  const char *description;
};

class GameMode {
public:
  virtual ~GameMode() = default;
  virtual const GameModeInfo &info() const = 0;
  virtual void onFrame(WebSocketClient &client) = 0;
  virtual void handleMessage(const IncomingMsg &msg) = 0;
  virtual void drawManageUI(WebSocketClient &client) = 0;
};

const std::vector<GameModeInfo> &getAvailableGameModes();
std::unique_ptr<GameMode> createGameMode(const char *modeId);
