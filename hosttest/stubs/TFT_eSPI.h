#pragma once
#include "Arduino.h"
#include "FS.h"
#define TL_DATUM 0
#define TC_DATUM 1
#define MC_DATUM 4
#define TFT_BLACK 0
#define TFT_WHITE 0xFFFF
#define TFT_GREEN 0x07E0
class TFT_eSPI {
  int font=2;
public:
  void init(){} void setRotation(int){}
  void fillScreen(uint32_t){} void fillRect(int,int,int,int,uint32_t){}
  void fillRoundRect(int,int,int,int,int,uint32_t){} void drawRoundRect(int,int,int,int,int,uint32_t){}
  void drawLine(int,int,int,int,uint32_t){} void drawRect(int,int,int,int,uint32_t){} void fillTriangle(int,int,int,int,int,int,uint32_t){}
  void fillCircle(int,int,int,uint32_t){} void drawCircle(int,int,int,uint32_t){}
  void setTextFont(int f){ font=f; }
  int fontHeight(){ return font==4?26:16; }
  int textWidth(const char* s){ return (int)strlen(s)*(font==4?13:7); }
  void setTextColor(uint32_t,uint32_t){} void setTextColor(uint32_t){} void setTextDatum(int){}
  int pushes=0, badPush=0; void setSwapBytes(bool){}
  void pushImage(int x,int y,int w,int h,uint16_t* d){ pushes++; if(x<0||y<0||w<=0||h<=0||x+w>320||y+h>240||!d) badPush++; else { volatile uint16_t t=d[0]; t=d[w*h-1]; (void)t; } }
  int drawString(const char* s,int,int){ return (int)strlen(s); }
  uint16_t color565(uint8_t r,uint8_t g,uint8_t b){ return (uint16_t)(((r&0xF8)<<8)|((g&0xFC)<<3)|(b>>3)); }
  void loadFont(const char*, fs::FS&){} void unloadFont(){}
};
