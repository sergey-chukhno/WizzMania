#include "TcpServer.h"
#include "../observability/AdminSentinel.h"
#include <iostream>

int main() {
  // Default to port 8080
  wizz::TcpServer server(8080);

  try {
    // Start networking in background
    server.start();

    // Start interactive Admin Dashboard (Blocks until shutdown)
    wizz::AdminSentinel sentinel(&server);
    sentinel.run();

  } catch (const std::exception &e) {
    std::cerr << "Server Crashed: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
