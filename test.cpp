#include <vector>
#include <iostream>

struct timespec a,b;

void foo(){
  clock_gettime(CLOCK_REALTIME, &a);

  std::vector<int> v;
  v.reserve(2000000);
  v.insert(v.begin(), 1000000, 1);

  clock_gettime(CLOCK_REALTIME, &b);
  std::cout << (long long)(b.tv_sec - a.tv_sec) * 1000000000 + (b.tv_nsec - a.tv_nsec) << std::endl;
  
  clock_gettime(CLOCK_REALTIME, &a);

  std::vector<int> q;
  q = v;
  clock_gettime(CLOCK_REALTIME, &b);
  std::cout << (long long)(b.tv_sec - a.tv_sec) * 1000000000 + (b.tv_nsec - a.tv_nsec) << std::endl;

}

int main(){
  foo();

}