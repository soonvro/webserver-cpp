#include <vector>
#include <iostream>
#include <stdlib.h>

struct timespec a, b;
std::vector<char>* foo(){
  std::cout << "foo start!!" << std::endl;

  clock_gettime(CLOCK_MONOTONIC, &a); 
  if (!b.tv_nsec)
    return 0;
  return 1;
}

int main(){

  std::vector<char> a;

  std::vector<char>& b;

  b = foo();
  std::cout << "aaaa";
  
}