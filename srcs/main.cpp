#include "Server.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "need argument" << std::endl;
    return 1;
  }
  Server  server(atoi(argv[1]));

  server.init();
  server.run();

  return 0;
}