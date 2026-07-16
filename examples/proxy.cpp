// proxy.cpp — list a product's IP whitelist and proxy sub-accounts.
//
// Reads the API key from the NOVADA_API_KEY environment variable (pass "" to
// the Client constructor to use it). Build with -DNOVADA_BUILD_EXAMPLES=ON.

#include <cstdlib>
#include <iostream>

#include <novada/client.hpp>
#include <novada/errors.hpp>
#include <novada/proxy/proxy_service.hpp>

int main() {
  try {
    // Empty key falls back to the NOVADA_API_KEY environment variable.
    novada::Client client;

    // List the residential whitelist.
    novada::proxy::ListWhitelistParams wl_params;
    wl_params.product = novada::proxy::Product::kResidential;
    auto whitelist = client.proxy().whitelist().list(wl_params);
    std::cout << "whitelist total=" << whitelist.total << "\n";
    for (const auto& entry : whitelist.list) {
      std::cout << "  ip=" << entry.mark_ip << " remark=" << entry.mark << "\n";
    }

    // List residential proxy sub-accounts (page 1, 10 per page by default).
    novada::proxy::ListAccountParams acct_params;
    acct_params.product = novada::proxy::Product::kResidential;
    auto accounts = client.proxy().account().list(acct_params);
    std::cout << "accounts total=" << accounts.total << "\n";
    for (const auto& account : accounts.list) {
      std::cout << "  account=" << account.account << " status=" << account.status << "\n";
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
