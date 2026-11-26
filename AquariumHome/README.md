# Implementation notes · V30

Build and flash instructions are in the [project README](../README.md) and [FLASHING.md](../FLASHING.md).

## Layout

- `AquariumHome.ino`: native ST7789 rendering, randomized movement, collision avoidance and decorative bubbles.
- `PhoneControl.h`: Wi-Fi AP, HTTP protocol, NVS ownership and seam coordination.
- `Ownership.h`: four durable creature identities, owner isolation and idempotent transfer commit.
- `Portal.h`: one-phone edge lease and epoch invalidation; network freshness is bounded to6000ms.
- `Seam.h` / `Seam.js`: complementary clipped body slices, negative approach progress, shared clock and rollback. Preparation starts about48 logical pixels before contact; normal acceptance preserves cruising speed. A blocked corridor retreats before contact. Tail completion commits once.
- `Glyphs.js`: Adafruit-compatible5×7 glyph subset; BSD notice included.
- `Motion.js`: phone movement, spatial avoidance and individually identified food grains.
- `Proximity.js`: median/EMA signal filter, near/far hysteresis and retry/offline policy. RSSI is not precise ranging.
- `ReefWebServer.h`: bounds empty accepted sockets to350ms so speculative browser connections cannot monopolize the single-client server.
- `app.js`: canvas UI, foreground-aware heartbeats, seam handshake, taps and local storage.
- `bundle_page.py`: embeds scripts into `index.html` and `WebPage.h`; rerun after editing JavaScript.
- `preview_server.py`: loopback demonstration backend; stops after10minutes. Test endpoints never ship in firmware.

## State and failure handling

Persistent creature IDs: large, small, octopus, crab. Ownership is stored in NVS; the browser identity is stored in `reef-client`. Clearing site data loses that identity. Logical ownership changes only at successful tail completion, never merely because proximity opens. The phone displays only its own residents plus clipped transit previews. In-flight uncertainty freezes until authoritative state resolves it. Operation IDs, portal epochs, per-boot tokens and snapshot sequences reject duplicates and old replies. Already completed transfers stay where they are after departure.

The device is left of the phone: native-right corresponds to phone-left. This is a fixed convention, not directional radio sensing. Coordinate space is240×240 on both sides. Web and native creature glyph geometry, colors and plants have parity regression fixtures.

Heartbeat deadline1800ms, bounded signal/ack/lease freshness6000ms. Missing one request triggers retry, not a false physical departure. Sustained weak filtered RSSI closes independently; hiding the tab or explicitly pausing closes immediately. A sole AP station can supply RSSI when DHCP MAC/IP lookup temporarily lags, but multiple stations never share a fallback reading. UART `STATE` / `RSSI` show request counts/age and diagnostic counters.

Original brownout protection stays enabled. A radio-start trial is persisted before enabling Wi-Fi and cleared after20stable seconds; an interrupted trial can fall back to local aquarium mode. Holding BOOT for2seconds inside the running app retries radio; USB serial diagnostics themselves can cause a reset, so avoid repeated open/close during startup.

## Feeding and animation

Tap a resident to reverse heading. Tap water to scatter12unique grains with lateral momentum, drag, drift and varied settling; a short swallow removes one selected grain ID. Crab can rise to eat and returns to the floor. The behavior is stylized; not all real crab species swim this way. Suction disturbances use small local strokes, not automatic bubble streams or concentric underwater waves. Idle bubbles are decorative. Uneaten grains fade before45s cleanup, with a48-grain display cap.

Animation research references: [Hikari slow-sinking feed](https://www.hikariusa.com/tropical_folder/micro_pellets.html), [fish suction feeding](https://elifesciences.org/articles/73621), [mechanical review](https://doi.org/10.1242/jeb.250567). These inform visual choices rather than claim physical fluid simulation.
