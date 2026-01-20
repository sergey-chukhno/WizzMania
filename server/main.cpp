#include "TcpServer.h"
#include <iostream>

int main() {
  // Default to port 8080
  wizz::TcpServer server(8080);

  try {
    // This will block until the server stops
    server.start();
  } catch (const std::exception &e) {
    std::cerr << "Server Crashed: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
