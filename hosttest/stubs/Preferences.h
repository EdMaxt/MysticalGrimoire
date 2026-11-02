#pragma once
#include <map>
#include <string>
// In-memory stand-in for the ESP32 NVS Preferences class (persists via a file for the host test).
class Preferences {
  std::map<std::string, unsigned> kv;
public:
  bool begin(const char*, bool=false){ FILE* f=fopen("/tmp/prefs_host.txt","r"); if(f){ char k[64]; unsigned v; while(fscanf(f,"%63s %u",k,&v)==2) kv[k]=v; fclose(f);} return true; }
  unsigned char getUChar(const char* k, unsigned char d=0){ return kv.count(k)?(unsigned char)kv[k]:d; }
  size_t putUChar(const char* k, unsigned char v){ kv[k]=v; FILE* f=fopen("/tmp/prefs_host.txt","w"); for(auto&p:kv) fprintf(f,"%s %u\n",p.first.c_str(),p.second); fclose(f); return 1; }
  unsigned short getUShort(const char* k, unsigned short d=0){ return kv.count(k)?(unsigned short)kv[k]:d; }
  size_t putUShort(const char* k, unsigned short v){ kv[k]=v; FILE* f=fopen("/tmp/prefs_host.txt","w"); for(auto&p:kv) fprintf(f,"%s %u\n",p.first.c_str(),p.second); fclose(f); return 2; }
};
