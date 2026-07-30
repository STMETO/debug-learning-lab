#include <fstream>
#include <iostream>
#include "person.pb.h"
int main() {
  lab::people::Person person;
  person.set_name("Ada"); person.set_age(36); person.set_email("ada@example.test");
  std::string bytes;
  if (!person.SerializeToString(&bytes)) return 1;
  std::ofstream("person.bin", std::ios::binary).write(bytes.data(), bytes.size());
  std::cout << "serialized " << bytes.size() << " bytes\n";
}
