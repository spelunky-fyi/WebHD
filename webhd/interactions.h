#pragma once

// Reusable interaction helpers for game modes.
// These operate directly on the game state via hddll globals.

// Spawns a 3x3 grid of webs (entity 115) centered on (cx, cy),
// skipping tiles that have a floor entity.
void spawnWebStorm(float cx, float cy);
