#include <string>
#include <vector>

class KrakenBase {
 public:
  KrakenBase(const std::string& api_key, const std::string& private_key, const std::string& base_endpoint);
  virtual ~KrakenBase() = default;
  std::string GetAccountBalance();
  

 private:
  std::string api_key_;
  std::string api_secret_;
  std::string base_endpoint_;
};