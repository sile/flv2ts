#include <iostream>
#include "flv/parser.hh"

int main(int argc, char** argv) {
  if(argc != 2) {
    std::cerr << "Usage: parse-flv FLV_FILE" << std::endl;
    return 1;
  }

  return 0;
}

