// wallet.cpp — print the wallet balance and recent usage records.
//
// Reads the API key from the NOVADA_API_KEY environment variable. Build with
// -DNOVADA_BUILD_EXAMPLES=ON.

#include <iostream>

#include <novada/client.hpp>
#include <novada/errors.hpp>
#include <novada/wallet/wallet_service.hpp>

int main() {
  try {
    novada::Client client;

    auto balance = client.wallet().balance();
    std::cout << "wallet balance=" << balance.balance << "\n";

    // First page, 10 records per page (the defaults).
    auto usage = client.wallet().usage_record();
    std::cout << "usage count=" << usage.count << "\n";
    for (const auto& record : usage.list) {
      std::cout << "  id=" << record.id << " type=" << record.order_type
                << " money=" << record.money << " order_id=" << record.order_id << "\n";
    }
    return 0;
  } catch (const novada::ApiException& e) {
    std::cerr << "API error (http=" << e.http_status() << ", code=" << e.code()
              << "): " << e.message() << "\n";
    return 1;
  } catch (const novada::NovadaException& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
}
