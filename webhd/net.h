#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <variant>

// Forward declaration
namespace ix {
class WebSocket;
}

// ---------------------------------------------------------------------------
// Thread-safe message queue
// ---------------------------------------------------------------------------

template <typename T> class MessageQueue {
public:
  void push(T item) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(std::move(item));
  }

  std::optional<T> pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty())
      return std::nullopt;
    T item = std::move(queue_.front());
    queue_.pop();
    return item;
  }

private:
  std::mutex mutex_;
  std::queue<T> queue_;
};

// ---------------------------------------------------------------------------
// Incoming message types (server → DLL player)
// ---------------------------------------------------------------------------

struct ExecuteInteraction {
  std::string interaction_id;
  std::string username;
  float x = 0.f;
  float y = 0.f;
  bool has_coords = false;
  float vx = 0.f;
  float vy = 0.f;
  bool has_velocity = false;
};

struct ViewerCountUpdate {
  size_t count = 0;
};

struct MemberJoinedMsg {
  std::string username;
  std::string role;
};

struct MemberLeftMsg {
  std::string username;
  std::string role;
};

using IncomingMsg =
    std::variant<ExecuteInteraction, ViewerCountUpdate, MemberJoinedMsg, MemberLeftMsg>;

// ---------------------------------------------------------------------------
// Catalog types (DLL → Server)
// ---------------------------------------------------------------------------

struct CatalogInteraction {
  std::string id;
  std::string name;
  uint32_t cost = 0;
  bool requires_coords = false;
  bool allows_velocity = false;
};

// ---------------------------------------------------------------------------
// Connection state
// ---------------------------------------------------------------------------

enum class ConnectionState {
  Disconnected,
  Connecting,
  Authenticating,
  CreatingLobby,
  JoiningLobby,
  InLobby,
};

const char *connectionStateName(ConnectionState state);

// ---------------------------------------------------------------------------
// WebSocket client
// ---------------------------------------------------------------------------

class WebSocketClient {
public:
  WebSocketClient();
  ~WebSocketClient();

  void connect(const std::string &url, const std::string &apiKey, const std::string &modeId);
  void disconnect();

  ConnectionState state() const { return state_; }
  std::string lobbyId() const { return lobby_id_; }
  std::string username() const { return username_; }
  bool isInLobby() const { return state_ == ConnectionState::InLobby; }
  std::string lastError() const { return error_; }
  bool forceJoinAvailable() const { return force_join_available_; }
  void forceJoinLobby();

  bool isPrivate() const { return private_; }
  void setPrivate(bool p) { private_ = p; }

  void sendPosition(float x, float y);
  void sendLevelData(uint32_t width, uint32_t height, const std::vector<uint8_t> &tiles);
  void sendLevelClear();
  void sendCatalog(const std::vector<CatalogInteraction> &interactions, uint32_t earnRate,
                   uint32_t maxGain = 0);

  std::optional<IncomingMsg> popIncoming();

private:
  void sendGameInput(const std::vector<uint8_t> &innerBytes);
  void handleMessage(const std::string &data);

  std::unique_ptr<ix::WebSocket> ws_;
  ConnectionState state_ = ConnectionState::Disconnected;
  std::string api_key_;
  std::string mode_id_;
  std::string lobby_id_;
  std::string username_;
  std::string error_;
  bool force_join_available_ = false;
  bool private_ = false;
  MessageQueue<IncomingMsg> incoming_;

  // Position throttling
  std::chrono::steady_clock::time_point last_position_send_;
};
