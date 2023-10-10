#include "Server.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "need one configure file" << std::endl;
    return 1;
  }
  Server  server(argv[1]);
  server.init();
  server.run();
  return 0;
}