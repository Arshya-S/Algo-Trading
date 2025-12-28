#include "kraken_websocket_candle_stream.h"
#include <iostream>

KrakenCandleStream::KrakenCandleStream(const std::string& ws_endpoint)
    : KrakenWebSocketBase(ws_endpoint) {}

void KrakenCandleStream::SubscribeCandles(const std::string& symbol, int interval, bool snapshot) {
  json subscribe_msg = {
    {"method", "subscribe"},
    {"params", {
      {"channel", "ohlc"},
      {"symbol", {symbol}},
      {"interval", interval},
      {"snapshot", snapshot}
    }}
  };
  
  Subscribe(subscribe_msg);
}

void KrakenCandleStream::UnsubscribeCandles(const std::string& symbol, int interval) {
  json unsubscribe_msg = {
    {"method", "unsubscribe"},
    {"params", {
      {"channel", "ohlc"},
      {"symbol", {symbol}},
      {"interval", interval}
    }}
  };
  
  SendMessage(unsubscribe_msg.dump());
}

void KrakenCandleStream::SetCandleCallback(std::function<void(const Candle&)> callback) {
  candle_callback_ = callback;
}

void KrakenCandleStream::HandleMessage(const json& message) {
  if (!message.contains("channel") || message["channel"] != "ohlc") {
    return;
  }
  
  if (message.contains("data")) {
    for (const auto& candle_data : message["data"]) {
      ParseCandleData(candle_data);
    }
  }
}

void KrakenCandleStream::ParseCandleData(const json& candle_data) {
  try {
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
  } catch (const std::exception& e) {
    std::cerr << "Error parsing candle data: " << e.what() << std::endl;
  }
}