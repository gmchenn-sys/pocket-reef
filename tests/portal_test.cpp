#include "../AquariumHome/Portal.h"
#include <cassert>
#include <cstdio>
int main() {
  EdgePortal p;
  assert(!p.live(100));p.update("phone-a", true, 100);auto old=p.epoch;
  assert(p.allows("phone-a",old,200));assert(!p.allows("phone-b",old,200));
  assert(!p.desktopExit(1,240,0,240,200));
  assert(!p.desktopExit(-1,250,0,240,200));
  assert(p.desktopExit(1,246,0,240,200));
  assert(!p.desktopExit(1,255,18,240,200));
  assert(p.desktopExit(1,264,18,240,200));
  p.update("phone-a",true,700);assert(p.epoch==old && p.live(1500));
  p.update("phone-a",false,1600);assert(!p.allows("phone-a",old,1600));
  assert(!p.desktopExit(1,270,0,240,1600));
  p.update("phone-a",true,1700);assert(p.epoch!=old);assert(!p.allows("phone-a",old,1750));
  auto current=p.epoch;assert(p.allows("phone-a",current,1800));
  assert(p.live(3500));p.expire(5000);assert(p.live(5000));assert(p.epoch==current);
  p.expire(8000);assert(!p.live(8000));assert(!p.allows("phone-a",current,8000));
  p.update("phone-a",true,9000);current=p.epoch;
  // A delayed heartbeat survives without changing crossing tokens.
  p.update("phone-a",true,14400);assert(p.epoch==current&&p.live(15000));
  p.update("phone-a",false,15001);assert(!p.live(15001));
  puts("PASS open edge lease, immediate close, expiry, stale return rejection, other phone isolation");
}
