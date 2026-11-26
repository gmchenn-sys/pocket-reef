#pragma once
#include <stdint.h>
#include <string.h>
#include <math.h>
struct SeamTransit {
  enum Phase { None, Offer, Running, Rollback, Done, Cancelled };
  Phase phase=None;
  int fish=-1;
  bool toPhone=false;
  char client[40]={}, operation[64]={};
  uint32_t epoch=0, started=0, duration=2400, changed=0;
  float height=.5f, rollbackFrom=0,lead=0,speed=20;
  uint32_t rollbackDuration=500;
  static int width(int i){return i==0?84:i==1?42:i==2?36:38;}
  static float clamp(float p){return fmaxf(0,fminf(1,p));}
  bool active()const{return phase==Offer||phase==Running||phase==Rollback;}
  float origin()const{return -lead/width(fish);}
  float raw(uint32_t now)const{return origin()+float(int32_t(now-started))/duration*(1-origin());}
  float progress(uint32_t now)const{
    if(phase==Done)return 1;
    if(phase==Running)return fmaxf(origin(),fminf(1,raw(now)));
    if(phase==Offer&&lead>0)return fmaxf(origin(),fminf(-8.f/width(fish),raw(now)));
    if(phase==Rollback){float t=clamp(float(uint32_t(now-started))/rollbackDuration);return rollbackFrom+(origin()-rollbackFrom)*(t*t*(3-2*t));}
    return origin();
  }
  bool offer(int i,bool phone,const char*c,const char*op,uint32_t token,float y,float velocity,uint32_t now,float approach=0){
    if(active())return false;
    fish=i;toPhone=phone;strcpy(client,c);strcpy(operation,op);epoch=token;height=y;
    lead=fmaxf(0,fminf(60,approach));speed=fmaxf(8,fminf(35,velocity));started=now;
    duration=lead>0?uint32_t((width(i)+lead)/speed*1000):uint32_t(fmaxf(1800,fminf(4200,width(i)/speed*1000)));
    phase=Offer;changed=now;return true;
  }
  void run(uint32_t now){if(phase==Offer){if(lead>0&&raw(now)>=-8.f/width(fish)){cancel(now);return;}phase=Running;if(lead<=0)started=now+220;changed=now;}}
  void cancel(uint32_t now){if(phase==Offer||phase==Running){rollbackFrom=progress(now);rollbackDuration=lead>0?uint32_t((fabsf(rollbackFrom-origin())*width(fish)<1?1:fmaxf(250,fminf(1600,fabsf(rollbackFrom-origin())*width(fish)/speed*1000)))):500;phase=Rollback;started=now;changed=now;}}
  float left(bool phone,uint32_t now,int screenWidth=240)const{
    float u=progress(now),w=width(fish);
    return toPhone?(phone?-w+u*w:screenWidth-w+u*w):(phone?-u*w:screenWidth-u*w);
  }
  float anchor(bool phone,uint32_t now)const{return left(phone,now)+(fish==3?1:(!phone&&fish==2?18:0));}
};
