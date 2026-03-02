#include "target_texture.h"

#include <d3d9.h>
#include "target_png.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ui {

static LPDIRECT3DTEXTURE9 sTargetTex = nullptr;
static int sTexW = 0;
static int sTexH = 0;

ImTextureID getTargetTexture(int *w, int *h) {
  if (sTargetTex) {
    *w = sTexW;
    *h = sTexH;
    return (ImTextureID)sTargetTex;
  }

  int channels;
  unsigned char *pixels =
      stbi_load_from_memory(kTargetPng, (int)kTargetPngSize, &sTexW, &sTexH, &channels, 4);
  if (!pixels)
    return nullptr;

  // Get D3D9 device from ImGui's font texture
  auto *fontTex = (LPDIRECT3DTEXTURE9)ImGui::GetIO().Fonts->TexID;
  if (!fontTex) {
    stbi_image_free(pixels);
    return nullptr;
  }

  LPDIRECT3DDEVICE9 device = nullptr;
  fontTex->GetDevice(&device);
  if (!device) {
    stbi_image_free(pixels);
    return nullptr;
  }

  HRESULT hr = device->CreateTexture(sTexW, sTexH, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
                                     &sTargetTex, nullptr);
  device->Release();
  if (FAILED(hr)) {
    stbi_image_free(pixels);
    return nullptr;
  }

  D3DLOCKED_RECT locked;
  sTargetTex->LockRect(0, &locked, nullptr, 0);
  for (int y = 0; y < sTexH; y++) {
    auto *dst = (unsigned char *)locked.pBits + y * locked.Pitch;
    auto *src = pixels + y * sTexW * 4;
    for (int x = 0; x < sTexW; x++) {
      // RGBA -> BGRA
      dst[x * 4 + 0] = src[x * 4 + 2]; // B
      dst[x * 4 + 1] = src[x * 4 + 1]; // G
      dst[x * 4 + 2] = src[x * 4 + 0]; // R
      dst[x * 4 + 3] = src[x * 4 + 3]; // A
    }
  }
  sTargetTex->UnlockRect(0);

  stbi_image_free(pixels);

  *w = sTexW;
  *h = sTexH;
  return (ImTextureID)sTargetTex;
}

void destroyTargetTexture() {
  if (sTargetTex) {
    sTargetTex->Release();
    sTargetTex = nullptr;
  }
}

} // namespace ui
