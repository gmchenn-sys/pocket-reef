#pragma once
#include <stdint.h>
#include <string.h>

constexpr int CREATURE_COUNT = 4;
constexpr uint32_t OWNERSHIP_MAGIC = 0x52454632;
struct LegacyOwnership {
  uint32_t magic, revision;
  char owner[2][40];
  char lastOp[2][64];
  uint8_t lastTarget[2];
};
struct Ownership {
  uint32_t magic;
  uint32_t revision;
  char owner[CREATURE_COUNT][40];
  char lastOp[CREATURE_COUNT][64];
  uint8_t lastTarget[CREATURE_COUNT];
  int8_t entryDirection[CREATURE_COUNT];
  float entryHeight[CREATURE_COUNT];
};
enum class TransferResult { Changed, Unchanged, Conflict, Invalid };
inline bool validToken(const char *text, size_t capacity) {
  if (!text || !*text) return false;
  size_t i = 0;
  for (; text[i] && i < capacity; ++i) {
    char c = text[i];
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '-' || c == '_')) return false;
  }
  return i < capacity;
}
inline TransferResult applyTransfer(Ownership &state, int fish, const char *client,
                                    const char *operation, bool toPhone) {
  if (fish < 0 || fish >= CREATURE_COUNT || !validToken(client, 40) || !validToken(operation, 64))
    return TransferResult::Invalid;
  bool alreadyPhone = !strcmp(state.owner[fish], client);
  bool inTank = !state.owner[fish][0];
  if (!strcmp(state.lastOp[fish], operation)) {
    if (state.lastTarget[fish] != uint8_t(toPhone)) return TransferResult::Conflict;
    return (toPhone ? alreadyPhone : inTank) ? TransferResult::Unchanged : TransferResult::Conflict;
  }
  if (!inTank && !alreadyPhone) return TransferResult::Conflict;
  if (toPhone) strcpy(state.owner[fish], client);
  else state.owner[fish][0] = 0;
  strcpy(state.lastOp[fish], operation);
  state.lastTarget[fish] = toPhone;
  state.revision++;
  return TransferResult::Changed;
}
