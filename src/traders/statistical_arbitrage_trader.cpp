#include "statistical_arbitrage_trader.h"
#include <iostream>
#include <iomanip>

StatisticalArbitrageTrader::StatisticalArbitrageTrader(size_t lookback, double z_entry, double z_exit)
    : model_(lookback, z_entry, z_exit), pnl_(0.0) {}

void StatisticalArbitrageTrader::OnCandle(const std::string& symbol, double close_price, const std::string& timestamp) {
    latest_prices_[symbol] = close_price;
    
    if (latest_prices_.count("BTC/USD") && latest_prices_.count("ETH/USD")) {
        auto signal = model_.GenerateSignal(
            latest_prices_["BTC/USD"],
            latest_prices_["ETH/USD"]
        );
        
        if (signal == Signal::LONG_SPREAD) {
            std::cout << "🟢 SIGNAL: Long BTC, Short ETH" << std::endl;
            LogTrade("LONG_SPREAD", timestamp);
        } else if (signal == Signal::SHORT_SPREAD) {
            std::cout << "🔴 SIGNAL: Short BTC, Long ETH" << std::endl;
            LogTrade("SHORT_SPREAD", timestamp);
        } else if (signal == Signal::EXIT) {
            std::cout << "⚪ SIGNAL: Exit positions" << std::endl;
            LogTrade("EXIT", timestamp);
        }
    }
}

void StatArbTradingSystem::LogTrade(const std::string& action, const std::string& timestamp) {
    Trade trade;
    trade.timestamp = timestamp;
    trade.action = action;
    trade.btc_price = latest_prices_["BTC/USD"];
    trade.eth_price = latest_prices_["ETH/USD"];
    trade.hedge_ratio = model_.GetCurrentHedgeRatio();
    trade.z_score = model_.GetCurrentZScore();
    
    trade_log_.push_back(trade);
}

const std::vector<Trade>& StatisticalArbitrageTrader::GetTradeLog() const {
    return trade_log_;
}

void StatisticalArbitrageTrader::PrintTradeLog() const {
    std::cout << "\n=== TRADE LOG ===" << std::endl;
    for (const auto& trade : trade_log_) {
        std::cout << std::fixed << std::setprecision(2)
                  << trade.timestamp << " | " << trade.action 
                  << " | BTC: " << trade.btc_price 
                  << " | ETH: " << trade.eth_price 
                  << " | Hedge: " << std::setprecision(4) << trade.hedge_ratio 
                  << " | Z: " << std::setprecision(3) << trade.z_score << std::endl;
    }
}

void StatisticalArbitrageTrader::SaveTradesToCSV(const std::string& filename) const {
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    
    file << "timestamp,action,btc_price,eth_price,hedge_ratio,z_score\n";
    
    for (const auto& trade : trade_log_) {
        file << trade.timestamp << ","
             << trade.action << ","
             << std::fixed << std::setprecision(2) << trade.btc_price << ","
             << trade.eth_price << ","
             << std::setprecision(4) << trade.hedge_ratio << ","
             << std::setprecision(3) << trade.z_score << "\n";
    }
    
    file.close();
    std::cout << "Trades saved to " << filename << std::endl;
}