#include "kraken_websocket.h"
#include <iostream>
#include <cstring>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

KrakenWebSocket::KrakenWebSocket(const std::string& ws_endpoint)
    : ws_endpoint_(ws_endpoint), context_(nullptr), wsi_(nullptr), running_(false), connected_(false) {}

KrakenWebSocket::~KrakenWebSocket() {
  Stop();
}

int KrakenWebSocket::WebSocketCallback(struct lws* wsi, enum lws_callback_reasons reason,
                                       void* user, void* in, size_t len) {
  KrakenWebSocket* ws = static_cast<KrakenWebSocket*>(lws_context_user(lws_get_context(wsi)));
  
  switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      std::cout << "✓ WebSocket connected" << std::endl;
      ws->connected_ = true;
      ws->SendPendingSubscription();
      break;
      
    case LWS_CALLBACK_CLIENT_RECEIVE: {
      std::string message((char*)in, len);
      ws->ParseCandleMessage(message);
      break;
    }
    
    case LWS_CALLBACK_CLIENT_CLOSED:
      std::cout << "WebSocket closed" << std::endl;
      ws->running_ = false;
      ws->connected_ = false;
      break;
      
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      std::cerr << "WebSocket connection error";
      if (in) {
        std::cerr << ": " << (char*)in;
      }
      std::cerr << std::endl;
      ws->running_ = false;
      ws->connected_ = false;
      break;
      
    default:
      break;
  }
  
  return 0;
}

void KrakenWebSocket::SendPendingSubscription() {
  if (pending_subscription_.empty() || !wsi_) {
    return;
  }
  
  std::cout << "✓ Subscribed to channel" << std::endl;
  unsigned char buf[LWS_PRE + 2048];
  memcpy(&buf[LWS_PRE], pending_subscription_.c_str(), pending_subscription_.length());
  lws_write(wsi_, &buf[LWS_PRE], pending_subscription_.length(), LWS_WRITE_TEXT);
  pending_subscription_.clear();
}

void KrakenWebSocket::Connect() {
  // Define protocols
  static struct lws_protocols protocols[] = {
    {
      "kraken-protocol",
      WebSocketCallback,
      0,
      4096,
    },
    { NULL, NULL, 0, 0 }
  };
  
  struct lws_context_creation_info info;
  memset(&info, 0, sizeof(info));
  
  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = protocols;
  info.gid = -1;
  info.uid = -1;
  info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
  info.user = this;
  
  context_ = lws_create_context(&info);
  if (!context_) {
    throw std::runtime_error("Failed to create WebSocket context");
  }
  
  struct lws_client_connect_info ccinfo;
  memset(&ccinfo, 0, sizeof(ccinfo));
  
  ccinfo.context = context_;
  ccinfo.address = "ws.kraken.com";
  ccinfo.port = 443;
  ccinfo.path = "/v2";
  ccinfo.host = ccinfo.address;
  ccinfo.origin = ccinfo.address;
  ccinfo.ssl_connection = LCCSCF_USE_SSL;
  ccinfo.protocol = protocols[0].name;
  
  wsi_ = lws_client_connect_via_info(&ccinfo);
  if (!wsi_) {
    lws_context_destroy(context_);
    throw std::runtime_error("Failed to connect to WebSocket");
  }
  
  running_ = true;
}

void KrakenWebSocket::SubscribeCandles(const std::string& symbol, int interval, bool snapshot) {
  json subscribe_msg = {
    {"method", "subscribe"},
    {"params", {
      {"channel", "ohlc"},
      {"symbol", {symbol}},
      {"interval", interval},
      {"snapshot", snapshot}
    }}
  };
  
  pending_subscription_ = subscribe_msg.dump();
  
  if (connected_) {
    SendPendingSubscription();
  }
}

void KrakenWebSocket::Unsubscribe(const std::string& symbol, int interval) {
  json unsubscribe_msg = {
    {"method", "unsubscribe"},
    {"params", {
      {"channel", "ohlc"},
      {"symbol", {symbol}},
      {"interval", interval}
    }}
  };
  
  std::string msg = unsubscribe_msg.dump();
  unsigned char buf[LWS_PRE + 1024];
  memcpy(&buf[LWS_PRE], msg.c_str(), msg.length());
  lws_write(wsi_, &buf[LWS_PRE], msg.length(), LWS_WRITE_TEXT);
}

void KrakenWebSocket::Run() {
  while (running_) {
    lws_service(context_, 50);
  }
}

void KrakenWebSocket::Stop() {
  running_ = false;
  if (context_) {
    lws_context_destroy(context_);
    context_ = nullptr;
  }
}

void KrakenWebSocket::SetCandleCallback(std::function<void(const Candle&)> callback) {
  candle_callback_ = callback;
}

void KrakenWebSocket::ParseCandleMessage(const std::string& message) {
  try {
    json j = json::parse(message);
    
    // Check if it's a candle update or snapshot
    if (j.contains("channel") && j["channel"] == "ohlc") {
      if (j.contains("data")) {
        for (const auto& candle_data : j["data"]) {
          Candle candle;
          candle.symbol = candle_data["symbol"];
          candle.open = candle_data["open"];
          candle.high = candle_data["high"];
          candle.low = candle_data["low"];
          candle.close = candle_data["close"];
          candle.vwap = candle_data["vwap"];
          candle.volume = candle_data["volume"];
          candle.trades = candle_data["trades"];
          candle.interval_begin = candle_data["interval_begin"];
          candle.interval = candle_data["interval"];
          
          if (candle_callback_) {
            candle_callback_(candle);
          }
        }
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error parsing message: " << e.what() << std::endl;
  }
}