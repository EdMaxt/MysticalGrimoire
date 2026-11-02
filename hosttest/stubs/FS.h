#pragma once
#include "Arduino.h"
#define FILE_READ "r"
#define FILE_WRITE "w"
namespace fs { class FS{}; }
class File {
  FILE* fp=nullptr; long sz=0;
public:
  File(){}
  File(FILE* f):fp(f){ if(fp){ fseek(fp,0,SEEK_END); sz=ftell(fp); fseek(fp,0,SEEK_SET);} }
  operator bool() const { return fp!=nullptr; }
  size_t size(){ return (size_t)sz; }
  int available(){ return fp ? (int)(sz-ftell(fp)) : 0; }
  size_t readBytes(char* b,size_t n){ return fp ? fread(b,1,n,fp) : 0; }
  size_t readBytesUntil(char t,char* b,size_t n){ size_t i=0; int c; while(i<n && (c=fgetc(fp))!=EOF){ if(c==t) break; b[i++]=(char)c; } return i; }
  size_t print(const char* t){ return fp ? fwrite(t,1,strlen(t),fp) : 0; }
  bool seek(size_t p){ return fp && fseek(fp,(long)p,SEEK_SET)==0; }
  void close(){ if(fp){ fclose(fp); fp=nullptr; } }
};
