#include <initializer_list>
#include "../AquariumHome/Seam.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main(){int ticks=0;for(int i=0;i<4;i++)for(int direction=0;direction<2;direction++){
 SeamTransit s;assert(s.offer(i,direction,"phone-a","preflight",2,.5,17,1000,48));float before=s.left(!direction,1600);s.run(1600);assert(s.phase==SeamTransit::Running);assert(fabsf(s.left(!direction,1600)-before)<.001f);assert(s.started==1000);
 auto contact=1000+uint32_t(48.f/17*1000);float last=s.left(!direction,contact-500);for(uint32_t t=contact-490;t<contact+500;t+=10){float x=s.left(!direction,t);assert(fabsf(fabsf(x-last)-.17f)<.003f);last=x;ticks++;}
 SeamTransit blocked;blocked.offer(i,direction,"phone-a","blocked",2,.5,17,1000,48);auto late=1000+uint32_t(41.f/17*1000);blocked.run(late);assert(blocked.phase==SeamTransit::Rollback);assert(blocked.progress(late)<0);assert(blocked.progress(late+blocked.rollbackDuration)<=blocked.origin()+.001f);
 }printf("PASS %d constant-velocity samples around edge contact; no wait/jump at acknowledgement; occupied approach retreats before contact\n",ticks);}
