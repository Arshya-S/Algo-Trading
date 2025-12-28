#ifndef KRAKEN_WEBSOCKET_CANDLE_STREAM_H
#define KRAKEN_WEBSOCKET_CANDLE_STREAM_H

#include "kraken_websocket_base.h"
#include <functional>

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

class KrakenCandleStream : public KrakenWebSocketBase {
 public:
  KrakenCandleStream(const std::string& ws_endpoint);
  
  void SubscribeCandles(const std::string& symbol, int interval, bool snapshot = true);
  void UnsubscribeCandles(const std::string& symbol, int interval);
  void SetCandleCallback(std::function<void(const Candle&)> callback);
  
 protected:
  void HandleMessage(const json& message) override;
  
 private:
  std::function<void(const Candle&)> candle_callback_;
  void ParseCandleData(const json& candle_data);
};

#endif