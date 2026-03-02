#pragma once

#include <imgui.h>

namespace ui {

// Returns a cached D3D9 texture for the target reticle image.
// Lazily decoded from the embedded PNG on first call.
ImTextureID getTargetTexture(int *w, int *h);

// Releases the cached texture. Call on shutdown.
void destroyTargetTexture();

} // namespace ui
