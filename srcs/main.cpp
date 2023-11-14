#include "Server.hpp"

int main(int argc, char** argv) {
  const char *config_file = (argc == 2 ? argv[1] : "webserv.config");
  try {
    Server  server(config_file);
    if (argc > 2) {
      std::cout << "usage: ./webserv [config_file]" << std::endl;
      return 1;
    }
    server.init();
    server.run();
  } catch (std::exception& e) { std::cout << e.what() << std::endl; }
  return 0;
}