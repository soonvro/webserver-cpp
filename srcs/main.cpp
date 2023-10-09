#include "Server.hpp"

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cout << "need argument" << std::endl;
    return 1;
  }

  //configure file 읽고 server init 방식으로 나중에 수정하기
  Server  server(argv[1]);
  
  server.init();
  server.run();

  return 0;
}