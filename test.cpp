#include <fstream>
#include <string>
#include <iostream>

int main(){
  std::ifstream file("Makefile");

  std::string line;
  file >> line;

  std::cout << line << "\n";


  
}