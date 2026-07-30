#include <iostream>
#include <string>

namespace lab {

std::string decorate(const std::string &name, int level)
{
    return "[" + std::to_string(level) + "] " + name;
}

}  // namespace lab

int main()
{
    std::cout << lab::decorate("C++ symbols", 7) << '\n';
    return 0;
}
