#include <exception>
#include <iostream>
#include <stdexcept>
#include "io.hpp"
using namespace levkin;
int main()
{
  try {
    Data data;
    readData(std::cin, data);
    if (data.cbegin() != data.cend()) {
      printNames(std::cout, data);
      std::cout << "\n";
      printTransposed(std::cout, data);
      std::cout << "\n";
    } else {
      std::cout << "0\n";
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  return 0;
}