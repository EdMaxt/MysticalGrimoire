#pragma once
#include "FS.h"
#include "SPI.h"
#include <sys/stat.h>
#include <string>
class SDClass : public fs::FS {
public:
  bool begin(uint8_t, SPIClass&, uint32_t=4000000){ return true; }
  void end(){}
  File open(const char* path, const char* m = "r"){ std::string p=std::string(getenv("SD_HOST_ROOT"))+path; return File(fopen(p.c_str(), m[0]=='w' ? "wb" : "rb")); }
  bool mkdir(const char* path){ std::string p=std::string(getenv("SD_HOST_ROOT"))+path; return ::mkdir(p.c_str(),0777)==0; }
  bool remove(const char* path){ std::string p=std::string(getenv("SD_HOST_ROOT"))+path; return ::remove(p.c_str())==0; }
  bool exists(const char* path){ std::string p=std::string(getenv("SD_HOST_ROOT"))+path; FILE*f=fopen(p.c_str(),"rb"); if(f){fclose(f);return true;} return false; }
};
extern SDClass SD;
