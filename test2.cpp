#include <vector>
using namespace std;
#include <iostream>
#include <string>
#include <sstream>
int main(){
  string s = "Content-Type: text/html\n\na";
  stringstream ss(s);
  string line;
  getline(ss, line);
  cout << "1" << line << endl;
  getline(ss, line);
  
  cout << "2" << line << endl;
  getline(ss, line);
  cout << "3"<< line << endl;
  getline(ss, line);
  cout << "4" << line << endl;


  return 0;
}