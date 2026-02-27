#include "net.h"
#include "ui/log.h"

#include <chrono>
#include <ixwebsocket/IXWebSocket.h>
#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;
using namespace std::chrono;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

const char *connectionStateName(ConnectionState state) {
  switch (state) {
  case ConnectionState::Disconnected:
    return "Disconnected";
  case ConnectionState::Connecting:
    return "Connecting...";
  case ConnectionState::Authenticating:
    return "Authenticating...";
  case ConnectionState::CreatingLobby:
    return "Creating lobby...";
  case ConnectionState::JoiningLobby:
    return "Joining lobby...";
  case ConnectionState::InLobby:
    return "In lobby";
  }
  return "Unknown";
}

// ---------------------------------------------------------------------------
// WebSocketClient
// ---------------------------------------------------------------------------

WebSocketClient::WebSocketClient() : ws_(std::make_unique<ix::WebSocket>()) {}

WebSocketClient::~WebSocketClient() { disconnect(); }

void WebSocketClient::connect(const std::string &url,
                              const std::string &apiKey,
                              const std::string &modeId) {
  disconnect();

  api_key_ = apiKey;
  mode_id_ = modeId;
  state_ = ConnectionState::Connecting;
  error_.clear();

  ws_ = std::make_unique<ix::WebSocket>();
  ws_->setUrl(url);

  ui::logInfo("Connecting to " + url + " ...");

  ws_->setOnMessageCallback([this](const ix::WebSocketMessagePtr &msg) {
    switch (msg->type) {
    case ix::WebSocketMessageType::Open:
      ui::logInfo("WebSocket connected, authenticating...");
      state_ = ConnectionState::Authenticating;
      {
        json auth = {{"type", "Authenticate"}, {"key", api_key_}};
        auto bytes = json::to_msgpack(auth);
        ws_->sendBinary(
            ix::IXWebSocketSendData{(const char *)bytes.data(), bytes.size()});
      }
      break;

    case ix::WebSocketMessageType::Message:
      if (msg->binary) {
        handleMessage(msg->str);
      }
      break;

    case ix::WebSocketMessageType::Close:
      ui::logInfo("WebSocket closed");
      state_ = ConnectionState::Disconnected;
      lobby_id_.clear();
      break;

    case ix::WebSocketMessageType::Error:
      ui::logError("WebSocket error: " + msg->errorInfo.reason);
      state_ = ConnectionState::Disconnected;
      lobby_id_.clear();
      break;

    default:
      break;
    }
  });

  ws_->start();
}

void WebSocketClient::disconnect() {
  if (ws_) {
    ws_->stop();
  }
  state_ = ConnectionState::Disconnected;
  lobby_id_.clear();
  username_.clear();
}

void WebSocketClient::handleMessage(const std::string &data) {
  try {
    // Decode using default json (std::map) for reading — key order doesn't
    // matter when deserializing.
    auto msg = nlohmann::json::from_msgpack(
        reinterpret_cast<const uint8_t *>(data.data()),
        reinterpret_cast<const uint8_t *>(data.data()) + data.size());
    auto type = msg.value("type", "");

    if (type == "Authenticated") {
      username_ = msg.value("username", "");
      ui::logInfo("Authenticated as " + username_);
      state_ = ConnectionState::CreatingLobby;
      json create = {
          {"type", "CreateLobby"}, {"mode_id", mode_id_}, {"private", false}};
      auto bytes = json::to_msgpack(create);
      ws_->sendBinary(
          ix::IXWebSocketSendData{(const char *)bytes.data(), bytes.size()});

    } else if (type == "LobbyCreated") {
      lobby_id_ = msg.value("lobby_id", "");
      ui::logInfo("Lobby created: " + lobby_id_);
      state_ = ConnectionState::JoiningLobby;
      json join = {{"type", "JoinLobby"},
                   {"lobby_id", lobby_id_},
                   {"role", "Player"}};
      auto bytes = json::to_msgpack(join);
      ws_->sendBinary(
          ix::IXWebSocketSendData{(const char *)bytes.data(), bytes.size()});

    } else if (type == "LobbyJoined") {
      ui::logInfo("Joined lobby");
      state_ = ConnectionState::InLobby;

    } else if (type == "GameOutput") {
      // Decode inner PlayerOutput message
      if (msg.contains("data") && msg["data"].is_binary()) {
        auto &innerBytes = msg["data"].get_binary();
        auto inner = nlohmann::json::from_msgpack(innerBytes);
        auto innerType = inner.value("type", "");

        if (innerType == "ExecuteInteraction") {
          ExecuteInteraction ei;
          ei.interaction_id = inner.value("interaction_id", "");
          ei.username = inner.value("username", "");
          if (inner.contains("x") && !inner["x"].is_null() &&
              inner.contains("y") && !inner["y"].is_null()) {
            ei.x = inner["x"].get<float>();
            ei.y = inner["y"].get<float>();
            ei.has_coords = true;
          }
          if (inner.contains("vx") && !inner["vx"].is_null() &&
              inner.contains("vy") && !inner["vy"].is_null()) {
            ei.vx = inner["vx"].get<float>();
            ei.vy = inner["vy"].get<float>();
            ei.has_velocity = true;
          }
          incoming_.push(ei);
        } else if (innerType == "ViewerCount") {
          ViewerCountUpdate vc;
          vc.count = inner.value("count", (size_t)0);
          incoming_.push(vc);
        }
      }

    } else if (type == "MemberJoined") {
      MemberJoinedMsg mj;
      mj.username = msg.value("username", "");
      mj.role = msg.value("role", "");
      incoming_.push(mj);

    } else if (type == "MemberLeft") {
      MemberLeftMsg ml;
      ml.username = msg.value("username", "");
      ml.role = msg.value("role", "");
      incoming_.push(ml);

    } else if (type == "AuthFailed" || type == "Error") {
      error_ = msg.value("message", msg.value("reason", "Unknown error"));
      ui::logError("Server: " + error_);
      state_ = ConnectionState::Disconnected;
      // Don't call ws_->stop() here — it deadlocks when called from callback.
      // The disconnect will be handled by the UI or destructor.
    }
  } catch (const std::exception &e) {
    error_ = std::string("Parse error: ") + e.what();
    ui::logError(error_);
  } catch (...) {
    error_ = "Unknown parse error";
    ui::logError(error_);
  }
}

void WebSocketClient::sendPosition(float x, float y) {
  if (state_ != ConnectionState::InLobby)
    return;

  // Throttle to ~10 Hz
  auto now = steady_clock::now();
  if (duration_cast<milliseconds>(now - last_position_send_).count() < 100)
    return;
  last_position_send_ = now;

  json inner = {{"type", "Position"}, {"x", x}, {"y", y}};
  sendGameInput(json::to_msgpack(inner));
}

void WebSocketClient::sendLevelData(uint32_t width, uint32_t height,
                                    const std::vector<uint8_t> &tiles) {
  if (state_ != ConnectionState::InLobby)
    return;

  json inner = {
      {"type", "LevelData"},
      {"width", width},
      {"height", height},
      {"tiles", json::binary_t(tiles)},
  };
  sendGameInput(json::to_msgpack(inner));
}

void WebSocketClient::sendLevelClear() {
  if (state_ != ConnectionState::InLobby)
    return;

  json inner = {{"type", "LevelClear"}};
  sendGameInput(json::to_msgpack(inner));
}

void WebSocketClient::sendGameInput(const std::vector<uint8_t> &innerBytes) {
  json outer = {
      {"type", "GameInput"},
      {"data", json::binary_t(innerBytes)},
  };
  auto bytes = json::to_msgpack(outer);
  ws_->sendBinary(
      ix::IXWebSocketSendData{(const char *)bytes.data(), bytes.size()});
}

std::optional<IncomingMsg> WebSocketClient::popIncoming() {
  return incoming_.pop();
}
