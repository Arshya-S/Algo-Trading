#ifndef KRAKEN_WEBSOCKET_BASE_H
#define KRAKEN_WEBSOCKET_BASE_H

#include <string>
#include <libwebsockets.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class KrakenWebSocketBase {
 public:
  KrakenWebSocketBase(const std::string& ws_endpoint);
  virtual ~KrakenWebSocketBase();
  
  void Connect();
  void Run();
  void Stop();
  
 protected:
  virtual void HandleMessage(const json& message) = 0;
  
  void Subscribe(const json& subscription);
  void SendMessage(const std::string& message);
  
  std::string ws_endpoint_;
  struct lws_context* context_;
  struct lws* wsi_;
  bool running_;
  bool connected_;
  std::string pending_subscription_;
  
 private:
  void SendPendingSubscription();
  static int WebSocketCallback(struct lws* wsi, enum lws_callback_reasons reason,
                               void* user, void* in, size_t len);
};

#endif