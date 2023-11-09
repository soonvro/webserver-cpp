#include <string>
#include <vector>
#include <iostream>

using namespace std;

int main(){

  vector<char> v;
  v.push_back('a');
  v.push_back('b');
  v.push_back('c');

  vector<char> c;
  v.push_back('1');
  v.push_back('2');
  std::cout << v.end() - v.begin()<< std::endl;
}