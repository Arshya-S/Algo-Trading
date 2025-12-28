#include <Eigen/Dense>
#include <vector>
#include <numeric>
#include <cmath>
#include <iostream>
#include <map>
#include <deque>

class StatisticalArbitrageModel {
    
private:
    size_t lookback_;
    double z_entry_;
    double z_exit_;
    
    std::deque<double> btc_prices_;
    std::deque<double> eth_prices_;
    std::deque<double> spreads_;
    
    double current_hedge_ratio_;
    
    enum class Position {
        NONE,
        LONG_SPREAD,
        SHORT_SPREAD
    };
    
    Position current_position_;
    
    void MaintainWindowSize(std::deque<double>& data) {
        if (data.size() > lookback_) {
            data.pop_front();
        }
    }
    
public:
    StatisticalArbitrageModel(size_t lookback, double z_entry, double z_exit)
        : lookback_(lookback), z_entry_(z_entry), z_exit_(z_exit),
          current_hedge_ratio_(1.0), current_position_(Position::NONE) {}
    
    double CalculateHedgeRatio() {
        if (btc_prices_.size() < 2) return 1.0;
        
        size_t n = btc_prices_.size();
        
        Eigen::VectorXd y(n);
        Eigen::VectorXd x(n);
        
        for (size_t i = 0; i < n; i++) {
            y(i) = btc_prices_[i];
            x(i) = eth_prices_[i];
        }
        
        Eigen::MatrixXd X(n, 2);
        X.col(0) = Eigen::VectorXd::Ones(n);
        X.col(1) = x;
        
        Eigen::VectorXd beta = (X.transpose() * X).ldlt().solve(X.transpose() * y);
        
        return beta(1);
    }
    
    double CalculateSpread(double btc_price, double eth_price, double hedge_ratio) {
        return btc_price - hedge_ratio * eth_price;
    }
    
    double CalculateZScore() {
        if (spreads_.size() < 2) return 0.0;
        
        double sum = 0.0;
        for (const auto& s : spreads_) {
            sum += s;
        }
        double mean = sum / spreads_.size();
        
        double sq_sum = 0.0;
        for (const auto& s : spreads_) {
            sq_sum += (s - mean) * (s - mean);
        }
        double std = std::sqrt(sq_sum / spreads_.size());
        
        if (std < 1e-10) return 0.0;
        
        return (spreads_.back() - mean) / std;
    }
    
    enum class Signal {
        NONE,
        LONG_SPREAD,
        SHORT_SPREAD,
        EXIT
    };
    
    Signal GenerateSignal(double btc_price, double eth_price) {
        btc_prices_.push_back(btc_price);
        eth_prices_.push_back(eth_price);
        
        MaintainWindowSize(btc_prices_);
        MaintainWindowSize(eth_prices_);
        
        if (btc_prices_.size() < lookback_) {
            return Signal::NONE;
        }
        
        current_hedge_ratio_ = CalculateHedgeRatio();
        double spread = CalculateSpread(btc_price, eth_price, current_hedge_ratio_);
        
        spreads_.push_back(spread);
        MaintainWindowSize(spreads_);
        
        double z_score = CalculateZScore();
        
        std::cout << "BTC: " << btc_price 
                  << " | ETH: " << eth_price
                  << " | Spread: " << spread 
                  << " | Z-Score: " << z_score 
                  << " | Hedge: " << current_hedge_ratio_ << std::endl;
        
        if (current_position_ == Position::NONE) {
            if (z_score > z_entry_) {
                current_position_ = Position::SHORT_SPREAD;
                return Signal::SHORT_SPREAD;
            } else if (z_score < -z_entry_) {
                current_position_ = Position::LONG_SPREAD;
                return Signal::LONG_SPREAD;
            }
        } else {
            if (std::abs(z_score) < z_exit_) {
                current_position_ = Position::NONE;
                return Signal::EXIT;
            }
        }
        
        return Signal::NONE;
    }
    
    double GetCurrentHedgeRatio() const {
        return current_hedge_ratio_;
    }
};
