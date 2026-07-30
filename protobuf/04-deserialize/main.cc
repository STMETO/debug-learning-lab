#include <fstream>
#include <iostream>
#include "person.pb.h"
int main() {
  std::ifstream file("person.bin", std::ios::binary);
  std::string bytes((std::istreambuf_iterator<char>(file)), {});
  lab::people::Person person;
  if (!person.ParseFromString(bytes)) { std::cerr << "invalid protobuf\n"; return 2; }
  std::cout << person.name() << ", age=" << person.age() << "\n";
}
