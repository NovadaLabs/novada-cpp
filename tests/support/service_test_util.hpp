#pragma once

#include <chrono>

#include "novada/detail/transport.hpp"
#include "support/mock_server.hpp"

namespace novada::test {

// make_test_transport builds a Transport whose three base URLs all point at
// the given MockServer, with a short timeout and the default retry count. It
// centralizes the boilerplate shared by every service test.
inline detail::Transport make_test_transport(const MockServer& server, int max_retries = 2) {
  return {"test-key",
          server.base_url(),
          server.base_url(),
          server.base_url(),
          std::chrono::milliseconds(2000),
          max_retries,
          "novada-cpp-test/0.0"};
}

}  // namespace novada::test
