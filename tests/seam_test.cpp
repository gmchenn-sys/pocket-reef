#include <initializer_list>
#include "../AquariumHome/Seam.h"
#include "../AquariumHome/Ownership.h"
#include <cassert>
#include <cmath>
#include <cstdio>
float visible(float left,float width){return fmaxf(0,fminf(240,left+width)-fmaxf(0,left));}
int main(){int checks=0;for(int i=0;i<4;i++)for(bool toPhone:{false,true}){
 SeamTransit s;assert(s.offer(i,toPhone,"phone-a","op-a",5,.5,20,1000));assert(s.progress(1000)==0);s.run(1000);Ownership state={};state.magic=OWNERSHIP_MAGIC;if(!toPhone)strcpy(state.owner[i],"phone-a");auto original=state;
 for(int t=0;t<100;t++){uint32_t now=s.started+uint32_t(s.duration*t/100);float u=s.progress(now),a=visible(s.left(false,now),s.width(i)),b=visible(s.left(true,now),s.width(i));assert(fabsf(a+b-s.width(i))<.001f);if(t>0){assert(a>0&&b>0);}assert(state.revision==original.revision);checks++;}
 assert(s.progress(s.started+s.duration)==1);assert(applyTransfer(state,i,"phone-a","op-a",toPhone)==TransferResult::Changed);assert(state.revision==original.revision+1);assert(applyTransfer(state,i,"phone-a","op-a",toPhone)==TransferResult::Unchanged);
 SeamTransit cancelled;assert(cancelled.offer(i,toPhone,"phone-a","op-b",7,.6,18,1000));cancelled.run(1000);auto now=cancelled.started+cancelled.duration/2;cancelled.cancel(now);float previous=cancelled.progress(now);for(int n=1;n<=10;n++){float p=cancelled.progress(now+n*50);assert(p<=previous);previous=p;}assert(previous==0);checks++;
 }printf("PASS %d complementary slice checks, bidirectional overlap for all four bodies, one ownership commit, idempotence and rollback\n",checks);}
