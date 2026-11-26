#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
constexpr int W=240;
std::mt19937 rng;
long random(long a,long b){return std::uniform_int_distribution<long>(a,b-1)(rng);}
struct Wander { float x, y, vx, vy, tx, ty; uint32_t next; int direction; };
Wander swimmers[3] = {};
uint32_t avoidedMoves = 0, edgePasses = 0;
float crabX = 178,crabY=220;
int crabDirection = -1;
float crabSpeed = 20;
uint32_t nextCrabChoice = 0;
bool fishInTank(int){return true;}
bool fishVisible(int){return true;}
bool seamControlled(int){return false;}
bool seamBlocks(int,float,float){return false;}
bool finishAtEdge(int,float){return false;}
struct Bounds { float left, right, top, bottom; };
Bounds swimBounds(int i, float x, float y) {
  return i < 2 ? Bounds{x, x + (i == 0 ? 84 : 42), y - 10, y + (i == 0 ? 27 : 19)}
               : Bounds{x - 18, x + 18, y - 17, y + 17};
}
bool swimOverlap(int i, float x, float y, int j, float ox, float oy) {
  Bounds a = swimBounds(i, x, y), b = swimBounds(j, ox, oy);
  if (a.right < 0 || a.left >= W || b.right < 0 || b.left >= W) return false;
  constexpr float gap = 6;
  return a.left < b.right + gap && a.right + gap > b.left &&
         a.top < b.bottom + gap && a.bottom + gap > b.top;
}
bool bodiesOverlap(Bounds a, Bounds b, float gap = 6) {
  if (a.right < 0 || a.left >= W || b.right < 0 || b.left >= W) return false;
  return a.left < b.right + gap && a.right + gap > b.left &&
         a.top < b.bottom + gap && a.bottom + gap > b.top;
}
Bounds crabBounds(float x) { return {x - 1, x + 37, crabY, crabY+15}; }
bool bottomRoom(Bounds body) {
  if(seamBlocks(3,body.left+1,body.top))return false;
  for (int j = 0; j < 3; ++j)
    if (fishVisible(j) && bodiesOverlap(body, swimBounds(j, swimmers[j].x, swimmers[j].y))) return false;
  return true;
}
void updateBottom(float dt, float t) {
  if (!fishInTank(3) || seamControlled(3)) return;
  uint32_t now = (uint32_t)(t * 1000);
  if (!nextCrabChoice || (int32_t)(now - nextCrabChoice) >= 0) {
    crabSpeed = random(18, 29);
    if (nextCrabChoice && crabX > 8 && crabX + 38 < W - 8 && random(0, 3) == 0) crabDirection = -crabDirection;
    nextCrabChoice = now + random(3500, 8000);
  }
  float cy=fminf(220,crabY+16*dt);
  if(bottomRoom({crabX-1,crabX+37,cy,cy+15}))crabY=cy;
  float cx = crabX + crabDirection * crabSpeed * dt;
  if (finishAtEdge(3, cx)) return;
  if (cx > W + 6) cx = -44;
  if (cx < -44) cx = W + 6;
  if (bottomRoom(crabBounds(cx))) crabX = cx;
  else crabDirection = -crabDirection;
}
bool canMove(int i, float x, float y) {
  if(seamBlocks(i,x,y))return false;
  Bounds body = swimBounds(i, x, y);
  if (fishVisible(3) && bodiesOverlap(body, crabBounds(crabX))) return false;
  for (int j = 0; j < 3; ++j) {
    if (j != i && fishVisible(j) && swimOverlap(i, x, y, j, swimmers[j].x, swimmers[j].y)) return false;
  }
  return true;
}
void updateSwimmers(float dt, uint32_t now) {
  const float speeds[] = {17, 15, 13};
  float desiredY[3] = {};
  for (int i = 0; i < 3; ++i) {
    if (!fishInTank(i) || seamControlled(i)) continue;
    Wander &m = swimmers[i];
    if (!m.direction) m.direction = 1;
    if (!m.next || (int32_t)(now - m.next) >= 0) {
      int bodyWidth = i == 0 ? 84 : i == 1 ? 42 : 36;
      if (m.next && m.x > 8 && m.x + bodyWidth < W - 8 && random(0, 3) == 0) m.direction = -m.direction;
      m.vx = speeds[i] * (0.85f + random(0, 31) / 100.0f);
      float targetY = random(i < 2 ? 24 : 28, i < 2 ? 183 : 203);
      // Vary depth freely around a preferred composition: small above, large below.
      if (i == 0) targetY = targetY * 0.4f + 145 * 0.6f;
      if (i == 1) targetY = targetY * 0.4f + 70 * 0.6f;
      m.ty = targetY;
      m.next = now + random(4500, 10000);
    }
    desiredY[i] = fmaxf(-8, fminf(8, (m.ty - m.y) * 0.35f));
  }
  // Predict approaching bodies, including the full tail and wave envelope.
  // Separate vertically early; keep horizontal motion only while there is room.
  for (int i = 0; i < 3; ++i) for (int j = i + 1; j < 3; ++j) {
    if (!fishInTank(i) || !fishInTank(j) || seamControlled(i) || seamControlled(j)) continue;
    Wander &a = swimmers[i], &b = swimmers[j];
    Bounds ba = swimBounds(i, a.x, a.y), bb = swimBounds(j, b.x, b.y);
    bool closeX = ba.left < bb.right + 52 && ba.right + 52 > bb.left;
    float centerA = a.y + (i < 2 ? 8.5f : 0), centerB = b.y + (j < 2 ? 8.5f : 0);
    float delta = centerB - centerA;
    if (closeX && fabsf(delta) < 57) {
      float sign = delta >= 0 ? 1.0f : -1.0f;
      float push = (57 - fabsf(delta)) * 0.6f;
      desiredY[i] -= sign * push;
      desiredY[j] += sign * push;
    }
  }
  for (int i = 0; i < 3; ++i) {
    if (!fishInTank(i) || seamControlled(i)) continue;
    Wander &m = swimmers[i];
    float blend = 1.0f - expf(-dt * 2);
    float target = fmaxf(-13, fminf(13, desiredY[i]));
    m.vy += (target - m.vy) * blend;
    float y = fmaxf(i < 2 ? 24 : 28, fminf(fmaxf(i < 2 ? 182 : 202,m.y), m.y + m.vy * dt));
    if (canMove(i, m.x, y)) m.y = y;
    else { m.vy = 0; avoidedMoves++; }
    int extent = i < 2 ? (i == 0 ? 84 : 42) : 18;
    int rightExtent = i < 2 ? 0 : 18;
    float x = m.x + m.direction * (m.vx > 0 ? m.vx : speeds[i]) * dt;
    if (finishAtEdge(i, x)) continue;
    bool wrapped = false;
    if (m.direction > 0 && x > W + rightExtent + 5) { x = -extent - 5; wrapped = true; }
    if (m.direction < 0 && x < -extent - 5) { x = W + rightExtent + 5; wrapped = true; }
    if (canMove(i, x, m.y)) { m.x = x; if (wrapped) edgePasses++; }
    else avoidedMoves++;
  }
}

int main(){unsigned long checks=0;for(int seed=0;seed<30;++seed){rng.seed(seed);for(auto &m:swimmers)m={};crabX=178;crabY=220;crabDirection=-1;nextCrabChoice=0;
 swimmers[0].x=12;swimmers[0].y=145;swimmers[0].direction=1;swimmers[1].x=116;swimmers[1].y=70;swimmers[1].direction=-1;swimmers[2].x=113;swimmers[2].y=204;
 int dirs[4]={};uint32_t now=0;
 for(int frame=0;frame<20000;++frame){float dt=frame%83?0.105f:.25f;now+=(uint32_t)(dt*1000);updateBottom(dt,now/1000.f);updateSwimmers(dt,now);
  Bounds b[4];for(int i=0;i<3;++i){auto &m=swimmers[i];b[i]=i==2?Bounds{m.x-18,m.x+18,m.y-17,m.y+17}:Bounds{m.x,m.x+(i==0?84:42),m.y-10,m.y+(i==0?27:19)};dirs[i]|=m.direction>0?1:2;}b[3]={crabX-1,crabX+37,220,235};dirs[3]|=crabDirection>0?1:2;
  for(int i=0;i<4;++i)for(int j=i+1;j<4;++j){auto a=b[i],c=b[j];if(a.right>=0&&a.left<W&&c.right>=0&&c.left<W&&a.left<c.right+5.9f&&a.right+5.9f>c.left&&a.top<c.bottom+5.9f&&a.bottom+5.9f>c.top){printf("FAIL overlap seed %d frame %d pair %d %d\n",seed,frame,i,j);return 1;}checks++;}
 }for(int i=0;i<4;++i)if(dirs[i]!=3){printf("FAIL one-way creature %d seed %d\n",i,seed);return 1;}
 }printf("PASS %lu physical-body separation checks and bidirectional random motion, 30 seeds\n",checks);}
