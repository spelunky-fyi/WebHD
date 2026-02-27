#pragma once

#include <string>

namespace ui {

enum class ToastSeverity { Debug, Info, Success, Warning, Error };

void showToast(const std::string &text, ToastSeverity severity = ToastSeverity::Info, float lifetime = 4.0f);
void drawToasts(float deltaTime);

} // namespace ui
