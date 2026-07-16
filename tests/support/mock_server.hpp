#pragma once

#include <chrono>
#include <map>
#include <string>
#include <thread>

#include <httplib.h>

namespace novada::test {

// MockServer runs a cpp-httplib server on a background thread bound to an
// OS-assigned loopback port. Tests register handlers via server() and then
// point a Transport at base_url(), exercising the real HTTP encoding
// produced by detail::Transport against a real (local) HTTP server rather
// than a mocked-out transport abstraction.
class MockServer {
 public:
  MockServer() {
    // The pre-routing handler fires before cpp-httplib reads/parses the
    // request body. Endpoints that send a zero-field (empty) multipart body
    // -- e.g. balance/countries/list-all -- would otherwise be rejected with
    // HTTP 400 by the built-in MultipartFormDataParser (which requires at
    // least one opening boundary line). Routes registered via on_post_json()
    // are answered here, bypassing that parser; the real Novada backend
    // accepts these empty bodies, so this is purely a mock-server workaround.
    server_.set_pre_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
      if (req.method == "POST") {
        auto it = json_routes_.find(req.path);
        if (it != json_routes_.end()) {
          res.set_content(it->second, "application/json");
          return httplib::Server::HandlerResponse::Handled;
        }
      }
      return httplib::Server::HandlerResponse::Unhandled;
    });

    port_ = server_.bind_to_any_port("127.0.0.1");
    thread_ = std::thread([this] { server_.listen_after_bind(); });
    while (!server_.is_running()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  ~MockServer() {
    server_.stop();
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  MockServer(const MockServer&) = delete;
  MockServer& operator=(const MockServer&) = delete;

  httplib::Server& server() { return server_; }

  // on_post_json registers a fixed JSON response for POST path, answered
  // before body parsing. Use this for endpoints that send no fields (the body
  // is an empty multipart form that cpp-httplib's request parser cannot
  // accept). Routes must be registered before the request is issued.
  void on_post_json(const std::string& path, std::string json_body) {
    json_routes_[path] = std::move(json_body);
  }

  std::string base_url() const { return "http://127.0.0.1:" + std::to_string(port_); }

 private:
  httplib::Server server_;
  std::map<std::string, std::string> json_routes_;
  int port_ = 0;
  std::thread thread_;
};

}  // namespace novada::test
