#ifndef KRAKEN_WEBSOCKET_H
#define KRAKEN_WEBSOCKET_H

#include <string>
#include <functional>
#include <libwebsockets.h>

struct Candle {
  std::string symbol;
  double open;
  double high;
  double low;
  double close;
  double vwap;
  double volume;
  int trades;
  std::string interval_begin;
  int interval;
};

class KrakenWebSocket {
 public:
  KrakenWebSocket(const std::string& ws_endpoint);
  ~KrakenWebSocket();
  
  void Connect();
  void SubscribeCandles(const std::string& symbol, int interval, bool snapshot = true);
  void Unsubscribe(const std::string& symbol, int interval);
  void Run();
  void Stop();
  
  void SetCandleCallback(std::function<void(const Candle&)> callback);
  
 private:
  std::string ws_endpoint_;
  struct lws_context* context_;
  struct lws* wsi_;
  bool running_;
  bool connected_;
  std::function<void(const Candle&)> candle_callback_;
  std::string pending_subscription_;
  
  void ParseCandleMessage(const std::string& message);
  void SendPendingSubscription();
  static int WebSocketCallback(struct lws* wsi, enum lws_callback_reasons reason,
                               void* user, void* in, size_t len);
};

#endif // KRAKEN_WEBSOCKET_H