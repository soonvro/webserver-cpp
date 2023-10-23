#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
int main(int argc, char **argv) {
  std::ifstream i("file");
  std::string line;
  while (i >> line) {
    if (line == "#") {
      std::cout << "comment ! : " << line;
      std::getline(i, line);
      std::cout << line << std::endl;
    } else {
      std::cout << "no comment : " << line << std::endl;
    }
  }

  return 0;
}