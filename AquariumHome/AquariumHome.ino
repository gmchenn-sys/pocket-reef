#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <math.h>

// Confirmed wiring: SCL12 SDA11 RST8 DC9 CS10; VCC/BL=3V3, GND=GND.
struct Bounds;
struct Wander;
constexpr int W = 240, H = 240;
constexpr uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
constexpr uint16_t EYE_INK = rgb(226, 217, 139);
constexpr uint16_t FIN_INK = rgb(195, 222, 214);
Adafruit_ST7789 screen(&SPI, 10, 9, 8);
GFXcanvas16 canvas(W, H);

struct Fish {
  float x, y, speed, phase;
  int direction;
  uint8_t scale;
  uint16_t color;
  uint32_t nextSpit;
  uint8_t burstRemaining;
};
Fish fish[] = {
  { 12, 145, 17, 0.1f,  1, 2, rgb(169, 220, 215), 0, 0},
  {116,  70, 15, 4.8f, -1, 1, rgb(208, 197, 232), 0, 0}
};
struct Bubble { float x, y, speed, phase; uint8_t radius; };
Bubble bubbles[3];
struct MouthBubble { float x, y, vx, speed, age, phase; bool active; };
MouthBubble mouthBubbles[24] = {};
uint32_t emittedBubbles = 0;
uint32_t lastFrame = 0, lastLog = 0, frames = 0;
struct Wander { float x, y, vx, vy, tx, ty; uint32_t next; int direction; };
Wander swimmers[3] = {};
uint32_t avoidedMoves = 0, edgePasses = 0;
float crabX = 178, crabY = 220;
int crabDirection = -1;
float crabSpeed = 20;
uint32_t nextCrabChoice = 0;
bool canMove(int i, float x, float y);
#include "PhoneControl.h"
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

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("REEF_LINK_STABLE_V30 boot: large fish below, small fish above, simple octopus, white crab; fish, octopus and crab bubbles; avoidance and wrap");
  SPI.begin(12, -1, 11, 10);
  screen.init(W, H, SPI_MODE0);
  screen.setSPISpeed(10000000);
  screen.setRotation(0);
  screen.fillScreen(ST77XX_BLACK);
  if (!canvas.getBuffer()) {
    Serial.println("REEF_LINK_STABLE_V30 framebuffer allocation failed");
    screen.setTextColor(ST77XX_WHITE);
    screen.setCursor(10, 30);
    screen.print("Framebuffer error");
    while (true) delay(1000);
  }
  canvas.setTextWrap(false);
  for (auto &b : bubbles) {
    b = {(float)random(10, 231), (float)random(12, 221),
         (float)random(9, 19), random(0, 628) / 100.0f, (uint8_t)random(1, 3)};
  }
  swimmers[0].x = fish[0].x; swimmers[0].y = fish[0].y; swimmers[0].direction = 1;
  swimmers[1].x = fish[1].x; swimmers[1].y = fish[1].y; swimmers[1].direction = -1;
  swimmers[2].x = 113; swimmers[2].y = 204;
  lastFrame = millis();
  fish[0].nextSpit = lastFrame + 900;
  fish[1].nextSpit = lastFrame + 2200;
  phoneSetup();
}

void drawPlants(float t) {
  const int roots[] = {15, 32, 62, 164, 199, 220};
  for (int i = 0; i < 6; ++i) {
    int height = 46 + (i * 17) % 44;
    int px = roots[i], py = 228;
    for (int h = 3; h <= height; h += 3) {
      float sway = sinf(h * 0.24f + t * 1.1f + i) * (0.7f + h * 0.035f);
      int x = roots[i] + (int)sway, y = 228 - h;
      canvas.drawLine(px, py, x, y, i % 3 ? rgb(45, 94, 76) : rgb(39, 76, 72));
      if (h % 12 == 0) canvas.drawLine(x, y, x + ((h / 12) % 2 ? 3 : -3), y - 4, rgb(45, 94, 76));
      px = x; py = y;
    }
  }
  for (int i = 0; i < 36; ++i)
    canvas.drawPixel((i * 37 + 11) % W, 229 + (i * 7) % 10, i % 3 ? 0x2104 : 0x4208);
}

void drawOctopus(float t) {
  int cx = (int)swimmers[2].x;
  int cy = (int)(swimmers[2].y + sinf(t * 1.1f) * 2);
  // A few character strokes, with no mascot face or filled body.
  canvas.setTextSize(1);
  canvas.setTextColor(rgb(144, 203, 212));
  canvas.setCursor(cx - 15, cy - 12); canvas.print(" ___ ");
  canvas.setCursor(cx - 15, cy - 4); canvas.print("(~~~)");
  for (int arm = 0; arm < 4; ++arm) {
    int root = cx - 9 + arm * 6;
    int px = root, py = cy + 4;
    for (int seg = 1; seg <= 4; ++seg) {
      int x = root + (int)(sinf(t * 2.2f - seg * 0.7f + arm) * seg * 0.45f);
      int y = cy + 4 + seg * 2;
      canvas.drawLine(px, py, x, y, rgb(196, 221, 218));
      px = x; py = y;
    }
  }
}

void creatureBubbles(uint32_t now, float t) {
  static uint32_t nextOctopus = 2700, nextCrab = 1400;
  static uint8_t crabBurst = 0;
  bool crab = (int32_t)(now - nextCrab) >= 0;
  bool octopus = (int32_t)(now - nextOctopus) >= 0;
  for (int kind = 0; kind < 2; ++kind) {
    if (!(kind == 0 ? crab : octopus) || !fishInTank(kind == 0 ? 3 : 2)) continue;
    float x = kind == 0 ? crabX + 17 : swimmers[2].x + 16;
    float y = kind == 0 ? crabY+4 : swimmers[2].y + sinf(t * 1.1f) * 2 - 1;
    bool visible = x > 5 && x < W - 5;
    if (visible) for (auto &b : mouthBubbles) {
      if (b.active) continue;
      b = {x, y, 2.0f, (float)random(18, 25), 0, t + kind, true};
      emittedBubbles++;
      break;
    }
    if (kind == 0) {
      if (visible) { crabBurst = (crabBurst + 1) % 2; nextCrab = now + (crabBurst ? 500 : random(4000, 6500)); }
      else nextCrab = now + 800;
    } else nextOctopus = now + (visible ? random(6000, 9500) : 800);
  }
}

void drawCrab(float t) {
  int x = (int)crabX;
  int y = (int)crabY;
  canvas.setTextSize(1);
  const char *claws = sinf(t * 4.5f) > 0 ? "Y(oo)Y" : ">(oo)<";
  for (int c = 0; c < 6; ++c) {
    canvas.setCursor(x + c * 6, y);
    canvas.setTextColor(ST77XX_WHITE);
    canvas.write(claws[c]);
  }
  // Six tiny angled legs, with alternating sideways steps.
  for (int leg = 0; leg < 3; ++leg) {
    int step = sinf(t * 6 + leg * 2) > 0 ? 1 : -1;
    int root = x + 9 + leg * 6;
    canvas.drawLine(root, y + 8, root - 3 + step, y + 12, ST77XX_WHITE);
    canvas.drawLine(root + 2, y + 8, root + 4 + step, y + 13, ST77XX_WHITE);
  }
}

void loop() {
  uint32_t now = millis();
  phoneTick(now);
  now = millis();
  if (now - lastFrame < 75) { delay(1); return; }
  float dt = fminf((now - lastFrame) / 1000.0f, 0.25f);
  lastFrame = now;
  float t = now / 1000.0f;
  canvas.fillScreen(ST77XX_BLACK);
  drawPlants(t);
  updateBottom(dt, t);
  updateSwimmers(dt, now);
  if (fishVisible(2)) drawOctopus(t);
  if (fishVisible(3)) drawCrab(t);
  creatureBubbles(now, t);
  for (auto &b : bubbles) {
    b.y -= b.speed * dt;
    if (b.y < -4) { b.y = 225; b.x = random(10, 231); }
    int x = (int)(b.x + sinf(t * 1.4f + b.phase) * 3);
    canvas.drawCircle(x, (int)b.y, b.radius, b.radius > 1 ? 0x5AEB : 0x3186);
  }
  for (auto &b : mouthBubbles) {
    if (!b.active) continue;
    b.age += dt;
    b.y -= b.speed * dt;
    b.x += b.vx * expf(-b.age * 0.8f) * dt;
    if (b.y < -5 || b.age > 9) { b.active = false; continue; }
    int radius = b.age < 0.7f ? 1 : (b.age < 2.2f ? 2 : 3);
    int x = (int)(b.x + sinf(b.age * 2.2f + b.phase) * fminf(b.age, 2.0f));
    canvas.drawCircle(x, (int)b.y, radius, b.age < 3.5f ? 0xBDF7 : 0x6B6D);
  }
  for (auto &f : fish) {
    if (!fishVisible(&f - fish)) continue;
    constexpr int chars = 7;
    int width = chars * 6 * f.scale;
    Wander &motion = swimmers[&f - fish];
    f.x = motion.x; f.y = motion.y;
    f.direction = motion.direction;
    float baseY = f.y + sinf(t * 0.7f + f.phase) * 5
                      + sinf(t * 1.4f + f.phase * 1.3f) * 1.5f;
    const char *body = f.direction > 0 ? "><(((o>" : "<o)))><";
    canvas.setTextColor(f.color);
    canvas.setTextSize(f.scale);
    // A travelling wave runs from head towards tail; the head stays steadier.
    for (int c = 0; c < chars; ++c) {
      int fromHead = f.direction > 0 ? chars - 1 - c : c;
      float amplitude = 0.4f + fromHead * 0.32f;
      float wave = sinf(t * 4.0f - fromHead * 0.8f + f.phase) * amplitude;
      canvas.setCursor((int)(f.x + c * 6 * f.scale), (int)(baseY + wave));
      uint16_t ink = body[c] == 'o' ? EYE_INK : (fromHead >= 5 ? FIN_INK : f.color);
      canvas.setTextColor(ink);
      canvas.write(body[c]);
    }
    int finX = (int)f.x + width / 2;
    int finY = (int)(baseY + sinf(t * 4.0f - 3 * 0.8f + f.phase) * 1.36f) + 7 * f.scale;
    canvas.drawLine(finX, finY, finX + (sinf(t * 5 + f.phase) > 0 ? 2 : -2), finY + 2, FIN_INK);
    float mouthX = f.direction > 0 ? f.x + width - 2 * f.scale : f.x;
    float mouthY = baseY + sinf(t * 4.0f + f.phase) * 0.4f + 3 * f.scale;
    if ((int32_t)(now - f.nextSpit) >= 0) {
      if (mouthX > 5 && mouthX < W - 5) {
        if (!f.burstRemaining) f.burstRemaining = random(2, 4);
        for (auto &b : mouthBubbles) {
          if (b.active) continue;
          b = {mouthX + f.direction * 3, mouthY, f.direction * 7.0f,
               (float)random(18, 26), 0, f.phase, true};
          emittedBubbles++;
          break;
        }
        --f.burstRemaining;
        f.nextSpit = now + (f.burstRemaining ? 420 : random(3500, 6000));
      } else {
        f.nextSpit = now + 400;
      }
    }
  }
  screen.drawRGBBitmap(0, 0, canvas.getBuffer(), W, H);
  frames++;
  if (now - lastLog > 5000) {
    lastLog = now;
    Serial.printf("REEF_LINK_STABLE_V30 alive frames=%lu uptime=%lu freeheap=%lu emitted=%lu avoided=%lu wraps=%lu\n",
                  (unsigned long)frames, (unsigned long)now, (unsigned long)ESP.getFreeHeap(),
                  (unsigned long)emittedBubbles, (unsigned long)avoidedMoves, (unsigned long)edgePasses);
  }
}
