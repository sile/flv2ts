#include <iostream>

int main(int argc, char** argv) {
  if(argc != 3) {
    std::cerr << "Usage: getts HLS_INDEX_PREFIX SEQUENCE" << std::endl;
    return 1;
  }
  return 0;
}
