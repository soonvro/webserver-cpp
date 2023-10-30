#include <vector>
#include <string>
#include <iostream>


void a(std::vector<std::string&> v){
  std::string tmp = "tmp";
  v.push_back(tmp);
}

int main(){
std::vector<std::string&> v;
  a(v);
  for(size_t i = 0;  i < v.size(); i++){
    std::cout << v[i] << std::endl;
  }
  return 0;
}