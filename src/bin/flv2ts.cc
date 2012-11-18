#include <iostream>

int main(int argc, char** argv) {
  if(argc != 3) {
    std::cerr << "Usage: flv2ts INPUT_FLV_FILE OUTPUT_DIR" << std::endl;
  }

  const char* flv_file = argv[1];
  const char* output_dir = argv[2];
  
  return 0;
}
