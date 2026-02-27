#include "toast.h"

#include <algorithm>
#include <deque>
#include <string>

#include <imgui.h>

namespace ui {

struct Toast {
  std::string text;
  ToastSeverity severity;
  float lifetime;
  float elapsed;
  float currentY; // smoothly interpolated toward target y
  bool yInitialized;
};

static ImVec4 severityColor(ToastSeverity s) {
  switch (s) {
  case ToastSeverity::Debug:   return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
  case ToastSeverity::Info:    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  case ToastSeverity::Success: return ImVec4(0.2f, 1.0f, 0.4f, 1.0f);
  case ToastSeverity::Warning: return ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
  case ToastSeverity::Error:   return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
  }
  return ImVec4(1, 1, 1, 1);
}

static constexpr size_t kMaxToasts = 10;
static std::deque<Toast> gToasts;

void showToast(const std::string &text, ToastSeverity severity,
               float lifetime) {
  // Immediately remove the oldest toast when at capacity
  while (gToasts.size() >= kMaxToasts) {
    gToasts.pop_front();
  }
  gToasts.push_back({text, severity, lifetime, 0.0f, 0.0f, false});
}

void drawToasts(float deltaTime) {
  if (gToasts.empty())
    return;

  auto &io = ImGui::GetIO();
  float viewportH = io.DisplaySize.y;
  float margin = 16.0f;
  float gap = 4.0f;
  float padding = 8.0f;
  float lerpSpeed = 12.0f;

  constexpr ImGuiWindowFlags kFlags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
      ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;

  // Tick and remove expired toasts
  for (size_t i = 0; i < gToasts.size();) {
    gToasts[i].elapsed += deltaTime;
    if (gToasts[i].elapsed >= gToasts[i].lifetime) {
      gToasts.erase(gToasts.begin() + i);
    } else {
      ++i;
    }
  }

  // Render newest (back) at bottom, oldest (front) stacked above
  for (size_t ri = 0; ri < gToasts.size(); ++ri) {
    auto &t = gToasts[gToasts.size() - 1 - ri];
    float progress = t.elapsed / t.lifetime;

    // Compute alpha only — position is handled by lerp
    float alpha;
    if (progress < 0.1f) {
      alpha = progress / 0.1f;
    } else if (progress < 0.8f) {
      alpha = 1.0f;
    } else {
      alpha = 1.0f - (progress - 0.8f) / 0.2f;
    }

    ImVec2 textSize = ImGui::CalcTextSize(t.text.c_str());
    float toastHeight = textSize.y + (padding * 2);

    float targetY = viewportH - margin - ((ri + 1) * (toastHeight + gap));

    // New toasts start below viewport; all toasts lerp at the same speed
    if (!t.yInitialized) {
      t.currentY = viewportH + toastHeight;
      t.yInitialized = true;
    }
    float diff = targetY - t.currentY;
    t.currentY += diff * std::min(1.0f, lerpSpeed * deltaTime);

    float x = margin;

    char winName[32];
    snprintf(winName, sizeof(winName), "##toast_%zu", ri);

    ImGui::SetNextWindowPos(ImVec2(x, t.currentY));
    ImGui::SetNextWindowBgAlpha(alpha * 0.55f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(padding, padding));
    ImVec4 col = severityColor(t.severity);
    col.w = alpha;
    ImGui::PushStyleColor(ImGuiCol_Text, col);

    if (ImGui::Begin(winName, nullptr, kFlags)) {
      ImGui::TextUnformatted(t.text.c_str());
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
  }
}

} // namespace ui
