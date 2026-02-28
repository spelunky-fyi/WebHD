#include "interactions.h"

#include <hddll/hd.h>
#include <hddll/hd_entity.h>
#include <hddll/hddll.h>

void spawnWebStorm(float cx, float cy) {
  if (!hddll::gGlobalState)
    return;

  int px = (int)cx;
  int py = (int)cy;
  auto *ls = hddll::gGlobalState->level_state;

  for (int dy = -2; dy <= 2; dy++) {
    for (int dx = -2; dx <= 2; dx++) {
      int tx = px + dx;
      int ty = py + dy;
      if (tx < 0 || tx >= 46 || ty < 0 || ty >= (int)(hddll::ENTITY_FLOORS_COUNT / 46))
        continue;
      size_t idx = (size_t)ty * 46 + (size_t)tx;
      if (ls && ls->entity_floors[idx] != nullptr)
        continue;
      hddll::gGlobalState->SpawnEntity((float)tx, (float)ty, 115, true);
    }
  }
}
