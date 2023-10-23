#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
int main(int argc, char **argv) {
  std::ifstream i("README.md");
  std::string line;

  // while (i >> line) {
  //   std::cout << line << std::endl;
  // }
  int a = 1;
  switch (a) {
    case 1:
      std::cout << "1";
      break;
    case 2:
      std::cout << "2";
      break;
    case 3:
      std::cout << "3";
      break;

    default:
      std::cout << "a";
      break;
  }

  return 0;
}