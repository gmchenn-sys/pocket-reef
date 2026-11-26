#include "../AquariumHome/Ownership.h"
#include <cassert>
#include <cstdio>
int main() {
  Ownership s = {}; s.magic = OWNERSHIP_MAGIC;
  assert(applyTransfer(s, 0, "phone-A", "take-1", true) == TransferResult::Changed);
  assert(!strcmp(s.owner[0], "phone-A") && !s.owner[1][0]);
  auto revision = s.revision;
  assert(applyTransfer(s, 0, "phone-A", "take-1", true) == TransferResult::Unchanged);
  assert(s.revision == revision);
  assert(applyTransfer(s, 0, "phone-B", "steal-1", true) == TransferResult::Conflict);
  assert(applyTransfer(s, 0, "phone-B", "return-foreign", false) == TransferResult::Conflict);
  assert(applyTransfer(s, 0, "phone-A", "take-1", false) == TransferResult::Conflict);
  Ownership restored; memcpy(&restored, &s, sizeof(s));
  assert(applyTransfer(restored, 0, "phone-A", "take-1", true) == TransferResult::Unchanged);
  assert(applyTransfer(restored, 0, "phone-A", "return-1", false) == TransferResult::Changed);
  assert(!restored.owner[0][0]);
  assert(applyTransfer(restored, 1, "phone-B", "take-small", true) == TransferResult::Changed);
  assert(!restored.owner[0][0] && !strcmp(restored.owner[1], "phone-B"));
  assert(applyTransfer(restored, 2, "phone-A", "take-octopus", true) == TransferResult::Changed);
  assert(applyTransfer(restored, 3, "phone-A", "take-crab", true) == TransferResult::Changed);
  assert(applyTransfer(restored, 3, "phone-B", "steal-crab", true) == TransferResult::Conflict);
  assert(applyTransfer(restored, 4, "phone-B", "invalid", true) == TransferResult::Invalid);
  assert(applyTransfer(restored, 0, "", "invalid", true) == TransferResult::Invalid);
  assert(applyTransfer(restored, 0, "bad\"token", "invalid", true) == TransferResult::Invalid);
  puts("PASS ownership transfer, retry after lost response, persisted reload, separate fish, foreign phone rejection, invalid input");
}
