#include <cassert>
#include <cstdio>
#include <cstdint>
constexpr int W=240,CREATURE_COUNT=4;
struct Passage {bool active; char client[40], operation[64]; int direction; uint32_t started;};
Passage passages[4]={};int commits=0;
bool departing(int i){return passages[i].active;}
void commitTransfer(int,const char*,const char*,bool,int){commits++;}
bool finishAtEdge(int index, float x) {
  if (!departing(index)) return false;
  int leftExtent = index == 0 ? 84 : index == 1 ? 42 : index == 2 ? 18 : 38;
  int rightExtent = index == 2 ? 18 : 0;
  bool gone = passages[index].direction > 0 ? x > W + rightExtent + 5 : x < -leftExtent - 5;
  if (!gone) return false;
  Passage pass = passages[index]; passages[index].active = false;
  commitTransfer(index, pass.client, pass.operation, true, pass.direction);
  return true;
}
int main(){for(int i=0;i<4;++i){int left=i==0?84:i==1?42:i==2?18:38;int right=i==2?18:0;
 passages[i].active=true;passages[i].direction=-1;int before=commits;
 assert(!finishAtEdge(i,-left+2));assert(commits==before);assert(finishAtEdge(i,-left-6));assert(commits==before+1);assert(!finishAtEdge(i,-left-7));
 passages[i].active=true;passages[i].direction=1;before=commits;
 assert(!finishAtEdge(i,W+right-2));assert(commits==before);assert(finishAtEdge(i,W+right+6));assert(commits==before+1);
 }puts("PASS no ownership commit until complete body exits; both directions; all four creatures; exactly-once completion");}
