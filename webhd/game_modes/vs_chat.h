#pragma once

#include "../game_mode.h"

#include <memory>

namespace game_modes {
std::unique_ptr<GameMode> createVsChatMode();
} // namespace game_modes
