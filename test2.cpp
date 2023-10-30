#include <vector>
using namespace std;
#include <iostream>
#include <string>
int main(){
  vector<char> a;
  a.push_back('1');
  a.push_back('2');
  a.push_back('3');
  a.push_back('4');
  a.push_back('5');
  a.push_back('6');
  a.push_back('7');
  std::string s = &(a[0]);
  std::cout << s;

  return 0;
}