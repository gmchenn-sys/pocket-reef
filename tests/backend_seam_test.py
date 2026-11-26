from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1];s=(root/'AquariumHome/PhoneControl.h').read_text();code=s[s.index('bool seamBlocks('):s.index('int requestRssi()')]+s[s.index('TransferResult commitTransfer('):s.index('void beginHotspot()')]
for value in ['millis()','index','ownership.revision']:code=code.replace('String('+value+')','std::to_string('+value+')')
header=r'''
#include <cstdio>
#include <cmath>
#include <cstring>
#include <cassert>
#include <string>
#include <cstdarg>
#include "Ownership.h"
#include "Portal.h"
#include "Seam.h"
using String=std::string;
constexpr int W=240,H=240;uint32_t clockMs=1000;uint32_t millis(){return clockMs;}
struct Wander{float x=0,y=120,vx=17,vy=0;int direction=1;uint32_t next=0;}swimmers[3];
float crabX=178,crabY=220,crabSpeed=20;int crabDirection=1;uint32_t nextCrabChoice=0;
Ownership ownership={};EdgePortal portal;SeamTransit seam;bool arrivalClear=true;
const char* creatureIds[]={"large","small","octopus","crab"};
bool fishInTank(int i){return !ownership.owner[i][0];}bool fishVisible(int i){return fishInTank(i)||(seam.active()&&seam.fish==i);}
bool canMove(int,float,float){return arrivalClear;}
struct Preferences{bool fail=false;int writes=0;size_t putBytes(const char*,const void*,size_t n){if(fail)return 0;writes++;return n;}}preferences;
struct SerialStub{void printf(const char*,...){}}Serial;
'''
main=r'''
void reset(){ownership={};ownership.magic=OWNERSHIP_MAGIC;portal={};seam={};preferences={};arrivalClear=true;clockMs=1000;for(auto&m:swimmers){m={};m.x=-200;m.y=0;}crabX=-100;crabY=220;crabDirection=1;portal.update("phone-a",true,clockMs);}
void tick(uint32_t t){clockMs=t;portal.update("phone-a",true,t);updateSeam(t);}
int main(){reset();seam.offer(0,false,"phone-a","reserve",portal.epoch,.5,20,clockMs);assert(!seamBlocks(1,200,120));seam.run(clockMs);assert(seamBlocks(1,200,120));assert(!seamBlocks(1,0,24));assert(!seamBlocks(0,156,120));for(int i=0;i<4;i++){
 reset();float anchor=240-SeamTransit::width(i)+(i==2?18:i==3?1:0);if(i<3){swimmers[i].x=anchor;swimmers[i].y=120;swimmers[i].direction=1;}else{crabX=anchor;crabY=160;}
 assert(finishAtEdge(i,anchor));assert(seam.phase==SeamTransit::Offer);assert(preferences.writes==0);assert(fishInTank(i));seam.run(clockMs);
 auto end=seam.started+seam.duration;tick(seam.started+seam.duration/2);assert(fishInTank(i)&&preferences.writes==0);assert(seam.progress(clockMs)>.49);
 tick(end);assert(seam.phase==SeamTransit::Done);assert(!fishInTank(i));assert(preferences.writes==1);tick(end+100);assert(preferences.writes==1);
 assert(seam.offer(i,false,"phone-a","return-a",portal.epoch,.5,20,clockMs));updateSeam(clockMs);assert(seam.phase==SeamTransit::Running);end=seam.started+seam.duration;
 tick(seam.started+seam.duration/2);assert(!fishInTank(i));tick(end);assert(fishInTank(i));assert(preferences.writes==2);float actual=i==3?crabX:swimmers[i].x;assert(fabsf(actual-(240-SeamTransit::width(i)+(i==2?18:i==3?1:0)))<.01);
 reset();if(i<3){swimmers[i].x=anchor;swimmers[i].y=120;}else{crabX=anchor;crabY=160;}assert(finishAtEdge(i,anchor));seam.run(clockMs);tick(seam.started+seam.duration/2);clockMs+=1;portal.update("phone-a",false,clockMs);updateSeam(clockMs);assert(seam.phase==SeamTransit::Rollback);clockMs+=500;updateSeam(clockMs);assert(seam.phase==SeamTransit::Cancelled);assert(fishInTank(i)&&preferences.writes==0);
 reset();if(i<3){swimmers[i].x=anchor;swimmers[i].y=120;}else{crabX=anchor;crabY=160;}assert(finishAtEdge(i,anchor));seam.run(clockMs);preferences.fail=true;tick(seam.started+seam.duration);assert(fishInTank(i)&&seam.phase==SeamTransit::Rollback&&preferences.writes==0);
 }
 reset();swimmers[0].x=156;swimmers[0].y=120;assert(finishAtEdge(0,156));seam.run(clockMs);clockMs=seam.started+seam.duration+1;portal.update("phone-a",false,clockMs);updateSeam(clockMs);assert(seam.phase==SeamTransit::Rollback&&fishInTank(0)&&preferences.writes==0);
 reset();swimmers[0].x=108;swimmers[0].y=120;assert(finishAtEdge(0,108));assert(seam.lead==48);tick(1600);assert(swimmers[0].x>118&&swimmers[0].x<119);float saved=swimmers[0].x;seam.run(clockMs);updateSeam(clockMs);assert(fabsf(swimmers[0].x-saved)<.001f);auto contact=seam.started+uint32_t(seam.lead/seam.speed*1000);tick(contact-100);float first=swimmers[0].x;tick(contact);float second=swimmers[0].x;tick(contact+100);assert(second>first+1.6f&&swimmers[0].x>second+1.6f);
 reset();swimmers[0].x=108;swimmers[0].y=120;assert(finishAtEdge(0,108));swimmers[1].direction=1;assert(finishAtEdge(1,168));assert(swimmers[1].direction==-1&&seam.fish==0);
 reset();strcpy(ownership.owner[0],"phone-a");arrivalClear=false;seam.offer(0,false,"phone-a","ahead",portal.epoch,.5,17,clockMs,48);updateSeam(clockMs);tick(3000);assert(seam.phase==SeamTransit::Offer&&seam.progress(clockMs)<0);tick(3500);assert(seam.phase==SeamTransit::Rollback&&seam.progress(clockMs)<0);assert(!fishInTank(0)&&preferences.writes==0);
 reset();strcpy(ownership.owner[0],"phone-a");arrivalClear=false;seam.offer(0,false,"phone-a","wait",portal.epoch,.5,20,clockMs);updateSeam(clockMs);assert(seam.phase==SeamTransit::Offer);arrivalClear=true;updateSeam(clockMs);assert(seam.phase==SeamTransit::Running);
 puts("PASS actual backend seam/commit functions: two-way final placement, single durable write, duplicate tick, mid-exit retreat, failed storage rollback and occupied-edge waiting");}
'''
p=Path('/tmp/reef-backend-seam.cpp');p.write_text(header+code+main)
subprocess.run(['c++','-std=c++17','-I'+str(root/'AquariumHome'),str(p),'-o','/tmp/reef-backend-seam'],check=True)
subprocess.run(['/tmp/reef-backend-seam'],check=True)
