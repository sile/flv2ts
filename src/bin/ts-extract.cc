#include <iostream>
#include <string.h>
#include <ts/parser.hh>
#include <ts/packet.hh>

using namespace flv2ts;

int main(int argc, char** argv) {
  if(argc != 3) {
    std::cerr << "Usage: ts-extract audio|video TS_FILE" << std::endl;
    return 1;
  }

  const char* type = argv[1];
  if(strcmp(type, "audio") != 0 &&
     strcmp(type, "video") != 0) {
    std::cerr << "Undefined type: " << type << std::endl;
    return 1;
  }
  const bool audio = strcmp(type, "audio") == 0;

  const char* filepath = argv[2];
  ts::Parser parser(filepath);
  if(! parser) {
    std::cerr << "Can't open file: " << filepath << std::endl;
    return 1;
  }

  for(int i=0; ! parser.eos(); i++) {
    ts::Packet packet;
    if(! parser.parse(packet)) {
      std::cerr << "parse packet failed " << std::endl;
      return 1;
    }

    if((audio   && ! parser.is_audio_packet(packet)) ||
       (! audio && ! parser.is_video_packet(packet))) {
      continue;
    }

    switch(parser.get_payload_type(packet)) {
    case ts::Packet::PAYLOAD_TYPE_PES: {
      const ts::PES& pes = *reinterpret_cast<ts::PES*>(packet.payload); 
      std::cout.write(reinterpret_cast<const char*>(pes.data), pes.data_size);
      break;
    }
    case ts::Packet::PAYLOAD_TYPE_DATA: {
      const ts::Data& data = *reinterpret_cast<ts::Data*>(packet.payload);
      std::cout.write(reinterpret_cast<const char*>(data.data), data.data_size);      
      break;
    }
    default:
      std::cerr << "unexpected packet type: " << parser.get_payload_type(packet) << std::endl;
      return 1;
    }
  }
  return 0;
}
