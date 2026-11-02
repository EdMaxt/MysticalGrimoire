#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cctype>
#include <string>
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define INPUT_PULLUP 2
inline void pinMode(int,int){}
inline void digitalWrite(int,int){}
inline int g_bootLevel = 1;
inline int digitalRead(int){return g_bootLevel;}
inline bool g_autoRelease = false;   // test: the 'user' lets go of BOOT as soon as the firmware waits
inline void delay(unsigned){ if (g_autoRelease) g_bootLevel = 1; }
inline unsigned long g_ms = 0;
inline unsigned long millis(){ return g_ms; }
#define ESP_ARDUINO_VERSION_MAJOR 3
#define GPIO_NUM_0 0
inline int g_bri = -1; inline bool g_slept = false;
inline void ledcAttach(int,int,int){}
inline void ledcWrite(int,int d){ g_bri=d; }
inline void rtc_gpio_pullup_en(int){} inline void rtc_gpio_pulldown_dis(int){}
inline void esp_sleep_enable_ext0_wakeup(int,int){}
inline void esp_deep_sleep_start(){ g_slept=true; }
inline void delayMicroseconds(unsigned){}
inline unsigned g_batmv = 1950;
inline unsigned analogReadMilliVolts(int){ return g_batmv; }
inline long map(long x,long a,long b,long c,long d){return (x-a)*(d-c)/(b-a)+c;}
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
struct SerialStub{ void begin(int){} };
extern SerialStub Serial;
