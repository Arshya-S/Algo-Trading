#include "kraken_base.h"
#include "kraken_websocket.h"
#include <iostream>
#include <cstdlib>
#include <csignal>
#include <iomanip>

// ANSI color codes
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define GREEN   "\033[32m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"

// Global pointer to WebSocket for signal handler
KrakenWebSocket* g_ws = nullptr;

void signalHandler(int signal) {
    std::cout << "\n" << YELLOW << "Shutting down gracefully..." << RESET << std::endl;
    if (g_ws) {
        g_ws->Stop();
    }
    exit(0);
}

void printCandle(const Candle& candle) {
    static double last_close = 0;
    bool is_up = (last_close == 0 || candle.close >= last_close);
    
    std::cout << BOLD << CYAN << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << RESET << std::endl;
    std::cout << BOLD << BLUE << candle.symbol << RESET 
              << " | " << MAGENTA << candle.interval_begin << RESET << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "  Open:   " << YELLOW << "$" << candle.open << RESET << std::endl;
    std::cout << "  High:   " << GREEN << "$" << candle.high << RESET << std::endl;
    std::cout << "  Low:    " << RED << "$" << candle.low << RESET << std::endl;
    std::cout << "  Close:  " << (is_up ? GREEN : RED) << "$" << candle.close 
              << " " << (is_up ? "▲" : "▼") << RESET << std::endl;
    std::cout << "  Volume: " << CYAN << candle.volume << " ETH" << RESET << std::endl;
    std::cout << "  VWAP:   " << YELLOW << "$" << candle.vwap << RESET << std::endl;
    std::cout << "  Trades: " << candle.trades << std::endl;
    
    last_close = candle.close;
}

int main(int argc, char* argv[]) {
    // Register signal handler
    std::signal(SIGINT, signalHandler);
    
    // Check for symbol argument
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <SYMBOL> [INTERVAL]" << std::endl;
        std::cerr << "Example: " << argv[0] << " BTC/USD 1" << std::endl;
        std::cerr << "Example: " << argv[0] << " ETH/USD 60" << std::endl;
        return 1;
    }
    
    std::string symbol = argv[1];
    int interval = (argc >= 3) ? std::stoi(argv[2]) : 1;
    
    // Get environment variables
    const char* api_key = std::getenv("KRAKEN_API_KEY");
    const char* api_secret = std::getenv("KRAKEN_PRIVATE_KEY");
    const char* base_endpoint = std::getenv("BASE_ENDPOINT");
    const char* ws_endpoint = std::getenv("BASE_WS_ENDPOINT");
    
    if (!base_endpoint || !ws_endpoint) {
        std::cerr << RED << "Error: Missing BASE_ENDPOINT or BASE_WS_ENDPOINT" << RESET << std::endl;
        return 1;
    }
    
    // REST API - Get balance (optional)
    if (api_key && api_secret) {
        KrakenBase kraken(api_key, api_secret, base_endpoint);
        std::cout << BOLD << "Fetching balance..." << RESET << std::endl;
        std::string balance = kraken.GetAccountBalance();
        std::cout << balance << "\n" << std::endl;
    }
    
    // WebSocket - Stream candle data
    std::cout << BOLD << GREEN << "Connecting to Kraken WebSocket..." << RESET << std::endl;
    KrakenWebSocket ws(ws_endpoint);
    g_ws = &ws;
    
    // Set callback for candle updates
    ws.SetCandleCallback(printCandle);
    
    ws.Connect();
    ws.SubscribeCandles(symbol, interval);
    
    std::cout << BOLD << CYAN << "\n📊 Streaming " << symbol << " candles (" 
              << interval << " min intervals)" << RESET << std::endl;
    std::cout << YELLOW << "Press Ctrl+C to stop...\n" << RESET << std::endl;
    
    ws.Run();
    
    return 0;
}