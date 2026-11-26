#pragma once
#include <stdint.h>
#include <string.h>
struct EdgePortal {
  static constexpr uint32_t LEASE_MS = 6000;
  char client[40] = {};
  bool open = false;
  uint32_t expires = 0, epoch = 1;
  bool live(uint32_t now) const { return open && int32_t(expires - now) > 0; }
  bool allows(const char *phone, uint32_t token, uint32_t now) const {
    return live(now) && epoch == token && !strcmp(client, phone);
  }
  bool desktopExit(int direction, float x, int rightExtent, int width, uint32_t now) const {
    return live(now) && direction > 0 && x > width + rightExtent + 5;
  }
  void expire(uint32_t now) { if (open && !live(now)) { open = false; epoch++; } }
  bool update(const char *phone, bool nearby, uint32_t now) {
    expire(now);
    if (open && strcmp(client, phone)) return false;
    if (!nearby) {
      if (!strcmp(client, phone) && open) { open = false; epoch++; }
      return true;
    }
    if (!open) { strcpy(client, phone); epoch++; open = true; }
    expires = now + LEASE_MS;
    return true;
  }
};
