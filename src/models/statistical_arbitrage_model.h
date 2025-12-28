#ifndef STATISTICAL_ARBITRAGE_MODEL_H
#define STATISTICAL_ARBITRAGE_MODEL_H

#include <vector>
#include <deque>
#include <cmath>
#include <Eigen/Dense>

enum class Signal {
    NONE,
    LONG_SPREAD,
    SHORT_SPREAD,
    EXIT
};

enum class Position {
    NONE,
    LONG_SPREAD,
    SHORT_SPREAD
};

class StatisticalArbitrageModel {
private:
    size_t lookback_;
    double z_entry_;
    double z_exit_;
    
    std::deque<double> btc_prices_;
    std::deque<double> eth_prices_;
    std::deque<double> spreads_;
    
    double current_hedge_ratio_;
    Position current_position_;
    
    void MaintainWindowSize(std::deque<double>& data);
    double CalculateHedgeRatio();
    double CalculateSpread(double btc_price, double eth_price, double hedge_ratio);
    double CalculateZScore();
    
public:
    StatisticalArbitrageModel(size_t lookback, double z_entry, double z_exit);
    
    Signal GenerateSignal(double btc_price, double eth_price);
    double GetCurrentHedgeRatio() const;
    double GetCurrentZScore();
};

#endif