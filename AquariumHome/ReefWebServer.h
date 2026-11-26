#pragma once
#include <WebServer.h>

// Browser speculative sockets must not monopolize this single-client server.
// Only drop a socket that has supplied no request bytes; never interrupt a
// parsed request or an in-progress response.
class ReefWebServer : public WebServer {
 public:
  explicit ReefWebServer(int port) : WebServer(port) {}
  uint32_t idleDrops = 0;
  void handleClient() override {
    if (_currentStatus == HC_WAIT_READ && !_currentClient.available() &&
        uint32_t(millis() - _statusChange) > 350) {
      _currentClient.stop();
      _currentStatus = HC_NONE;
      ++idleDrops;
    }
    WebServer::handleClient();
  }
};
