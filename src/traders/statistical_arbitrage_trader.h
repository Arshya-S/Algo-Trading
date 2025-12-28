#ifndef STATISTICAL_ARBITRAGE_TRADER_H
#define STATISTICAL_ARBITRAGE_TRADER_H

#include "../models/statistical_arbitrage_model.h"
#include <string>
#include <map>
#include <vector>
#include <fstream>

struct Trade {
    std::string timestamp;
    std::string action;
    double btc_price;
    double eth_price;
    double hedge_ratio;
    double z_score;
};

class StatisticalArbitrageTrader {
private:
    StatArbModel model_;
    std::map<std::string, double> latest_prices_;
    std::vector<Trade> trade_log_;
    double pnl_;
    
    void LogTrade(const std::string& action, const std::string& timestamp);
    
public:
    StatisticalArbitrageTrader(size_t lookback = 100, double z_entry = 2.0, double z_exit = 0.5);
    
    void OnCandle(const std::string& symbol, double close_price, const std::string& timestamp);
    
    const std::vector<Trade>& GetTradeLog() const;
    void PrintTradeLog() const;
    void SaveTradesToCSV(const std::string& filename = "trades.csv") const;
};

#endif