<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 The Bourne Again RadioFoundry Authors -->

# Standby Blink Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or
> superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** The green LED blinks for 100 ms every N seconds while both LEDs would otherwise be dark. The interval is
set in Settings > Display > Standby blink: Off, 2 s, 5 s or 10 s, default 5 s.

**Architecture:** A heartbeat in the radio core's LED output (`core/radio/radio_sm.c`), applied after the existing
timed signals, so it has the lowest priority. The interval is a `radio_settings_t` field, stored in one settings-store
byte and shown as a UI list row. The debug link exposes the LED bits and a blink counter for a HIL test.

**Tech Stack:** C (firmware core, host tests in `tests/host`), Python (pytest HIL, `tools/`), gettext `.po`
catalogs.

**Spec:** `docs/superpowers/specs/2026-11-27-standby-blink-design.md`

## Global Constraints

- Every rule in `AGENTS.md` applies: clean room, US English, zero warnings in every `tools/ci.sh` configuration,
  SPDX headers on new files, no per-board code, and `core/` never tests a radio define.
- Blink length: `RADIO_LED_WAKE_MS` (100 ms). Choices: 0 (Off), 2, 5, 10 s. Default 5 s.
- `radio_settings_t.standby_blink_s` holds 0..`RADIO_BLINK_MAX_S` (60). The UI offers only the four choices.
- An older settings record (byte 406 = 0) reads as the default, 5 s.
- The blink never changes `rx_open`, the speaker, dual watch's audio hand-off, squelch, scan or battery-save timing.
- The diff stays small and self-contained: this is a fork, and it must merge cleanly with upstream syncs.
- Flash: measure `text` on the two builds in FORK.md's "Before merging" list. Before (main `5a42458b`): 702524 and
  752352. Measured after Task 4: +568 and +144 B. The feature is behind `CONFIG_STANDBY_BLINK` (Task 5, added at the
  owner's request so the site's firmware builder offers it).
- Translate the new label into every catalog (`core/ui/lang/es.po fr.po it.po zh.po`).
- Never touch a radio. HIL runs and flashing happen only when the owner says to.

## Review Focus

1. **Battery save asleep:** the blink keeps its schedule while `rx_sleeping` is 1, and it never wakes the receiver.
   Test in Task 1.
2. **Setting turned off during a blink:** green goes dark at the next `outputs()` and stays dark, never stuck on.
   Test in Task 1.
3. **DMR channel idle, or a call not played (`RDMR_RX_OTHER`):** the LED is dark, so it blinks as on FM. Test in
   Task 1.
4. **Scan running:** it blinks during scan, and the channel sequence and dwell timing are the same with the blink on
   or off. Test in Task 1.
5. **A bad stored value (`rs_blink` = 201):** it loads clamped to 60, never wraps to a short interval. Test in
   Task 2.

---

### Task 1: Heartbeat in the radio core

**Files:**
- Modify: `core/radio/radio.h`: the `radio_settings_t` field after `backlight_off_s`, `RADIO_BLINK_DEFAULT_S` and
  `RADIO_BLINK_MAX_S` next to `RADIO_DUAL_DELAY_MS`, and `uint16_t led_blinks` in `radio_view_t` next to `leds`.
- Modify: `core/radio/radio_sm.h`: `uint32_t blink_dark_ms` with the other LED fields, after `wake_seen`.
- Modify: `core/radio/radio_sm.c`: `radio_settings_default`, `clamp_settings`, `radio_sm_init`, a new
  `led_heartbeat()` called from `outputs()`.
- Modify: `docs/spec/radio-features.md`: a row in the section 1 settings table, and a heartbeat row in 5.7.
- Test: `tests/host/radioCoreTest.c`, next to `test_dmr_wake_leds`, registered in its `main` list.

**Interfaces:**
- Produces:
  - `radio_settings_t.standby_blink_s` (`uint8_t`, seconds, 0 = off).
  - `#define RADIO_BLINK_DEFAULT_S 5u` and `#define RADIO_BLINK_MAX_S 60u`.
  - `radio_view_t.led_blinks` (`uint16_t`, standby blinks shown since start-up, wrapping).
  - `radio_view_t.leds`, unchanged, now including the blink.

- [ ] **Step 1: Write the failing host tests** in `radioCoreTest.c`.
  - Helper: `static void blink_every(uint8_t s)` does `radio_get_settings`, sets `standby_blink_s`, calls
    `radio_set_settings`, then `run(20)`.
  - `test_standby_blink_idle`: `setup_codeplug(); start();` on the idle FM channel with the defaults.
    - `sm.set.standby_blink_s == RADIO_BLINK_DEFAULT_S`.
    - Track green rising edges over 21 s, `run(5)` per step. Exactly 4 edges, the first 5000 ± 10 ms after
      `start()`, then 5000 ± 10 ms apart. Each stays on 100 ± 10 ms.
    - `(sm.view.leds & RADIO_LED_GREEN) != 0` equals `hw.led[LED_GREEN]` at every step.
    - `sm.view.led_blinks == 4`. Red stays off throughout.
    - The board's `rx_open` and `audio` were never called with true. Use `hw` fields if the fake board records
      them, else add a counter to the fake board.
  - `test_standby_blink_off`: `blink_every(0)`, then 30 s idle: green never on, `led_blinks` unchanged.
  - `test_standby_blink_yields`, with `blink_every(2)`:
    - An FM signal for 3 s (as other tests open the squelch): green solid with no gap.
    - After it ends, the next rising edge is ≥ 2000 ms after the squelch closed.
    - PTT held 5 s on a TX-capable channel: green never on.
    - A deny (an RX-only channel): the 6-step red/green pattern is the same as with `blink_every(0)`, compared step
      by step.
  - Review Focus 1, `test_standby_blink_battery_save`: `rx_save` = 1, `rx_save_idle_s` = 1, `blink_every(2)`, 10 s
    idle.
    - The receiver sleeps: `sm.view.rx_sleeping` is seen as 1.
    - Blinks stay 2000 ± 30 ms apart.
    - The sleep cycle count matches a run with `blink_every(0)`, so the blink isn't activity.
  - Review Focus 2, `test_standby_blink_off_mid_blink`: `blink_every(2)`, run to 50 ms into a blink (green on),
    `blink_every(0)`. Green is off after the next `run(10)` and stays off for 10 s.
  - Review Focus 3, `test_standby_blink_dmr_idle`: `dmr_start(true)` on the DMR channel, `blink_every(2)`, 7 s idle:
    3 edges. A call on our slot to another talkgroup (`RDMR_RX_OTHER`, built as other DMR tests do) keeps the blinks
    on schedule. A played call (`RDMR_RX`) holds green solid.
  - Review Focus 4, `test_standby_blink_scan`: the same zone scan for 10 s twice, with `blink_every(0)` and then
    `blink_every(2)`. The `sm.view` channel sequence and hop times are identical. The second run shows ≥ 4 edges.

- [ ] **Step 2: Run them and see them fail**

  Run: `make -C tests/host -j4 radioCoreTest && tests/host/radioCoreTest`
  Expected: the build fails on `standby_blink_s` / `led_blinks` / `RADIO_BLINK_DEFAULT_S` not defined.

- [ ] **Step 3: Implement.**
  - `radio_settings_default`: `standby_blink_s = RADIO_BLINK_DEFAULT_S`.
  - `clamp_settings`: values above `RADIO_BLINK_MAX_S` become `RADIO_BLINK_MAX_S`.
  - `radio_sm_init`: `blink_dark_ms = now_ms`, so the first blink comes one interval after start-up.
  - In `outputs()`, call `led_heartbeat(sm, red, &green)` right after `led_signals(sm, &red, &green)`:

  ```c
  // Standby blink (radio-features.md 5.7): green for RADIO_LED_WAKE_MS once per interval while both LEDs would be
  // dark, the lowest of the LED signals. The interval counts from when they last showed something, so the end of a
  // call is never followed by a blink at once
  static void led_heartbeat(radio_sm_t *sm, bool red, bool *green)
  {
  	uint32_t now = sm->led_now;
  	uint32_t every = (uint32_t)sm->set.standby_blink_s * 1000u;
  	if (every == 0 || red || *green || sm->led_timed || sm->view.state == RS_TX ||
  	    sm->view.state == RS_TX_DENIED) {
  		sm->blink_dark_ms = now;
  		return;
  	}
  	uint32_t dark = now - sm->blink_dark_ms;
  	*green = dark >= every && dark % every < RADIO_LED_WAKE_MS;
  	if (*green && sm->green_out != 1) {
  		sm->view.led_blinks++;
  	}
  	sm->led_timed = true;	// the tick and the poll keep calling outputs() to keep time
  }
  ```

  The excluded states, RS_TX and RS_TX_DENIED, cover the dark gaps between repeater wake-up bursts. `led_signals()`
  assigns `led_timed` on every call, so the check here sees only the other signals.

- [ ] **Step 4: Run the whole host suite**

  Run: `make -C tests/host -j4`
  Expected: the new tests pass.
  - Some existing tests may now fail because green blinks after 5 s idle where they assert it dark. Fix each such
    test by turning the blink off in that test only (`blink_every(0)`, or the same in that file's style), with a
    one-line comment.
  - If more than 5 tests need it, turn it off in that file's `start()` instead, with a comment. The new tests then
    turn it on themselves.
  - Fix nothing else this way: any other failure is a bug in the change.

- [ ] **Step 5: Update `docs/spec/radio-features.md`.**
  - Section 1 table: `| standby_blink_s | 5 | the green LED's standby blink (5.7): every so many seconds, 0 = off, at most 60 |`.
  - 5.7 table: `| Standby blink (standby_blink_s, 0 = off) | green for RADIO_LED_WAKE_MS (100 ms) once per interval while both LEDs would be dark and the radio isn't transmitting or refusing PTT; the lowest signal, counted from when the LEDs last showed something. view.led_blinks counts them |`.
  - Then run `python3 tools/check_doc_links.py`. Expected: 0 broken links.

- [ ] **Step 6: Commit**

  ```bash
  git add core/radio/radio.h core/radio/radio_sm.h core/radio/radio_sm.c tests/host/radioCoreTest.c docs/spec/radio-features.md
  git commit -m "Radio core: standby blink, a green heartbeat while the LEDs are dark"
  ```

### Task 2: Store the interval

**Files:**
- Modify: `core/settings/settings.h:257`: `reserved11[2]` becomes `rs_blink` plus `reserved11[1]`.
- Modify: `core/radio/radio_sm.c`: `settings_to_radio`, `radio_to_settings`.
- Test: `tests/host/settingsTest.c:962` (the layout table), and `tests/host/radioCoreTest.c`.

**Interfaces:**
- Consumes: `radio_settings_t.standby_blink_s`, `RADIO_BLINK_DEFAULT_S`, `RADIO_BLINK_MAX_S` (Task 1).
- Produces: `settings_t.rs_blink` (`uint8_t`, offset 406): `standby_blink_s + 1`, 0 = never set (the default).
  `sizeof(settings_t)` stays 412.

- [ ] **Step 1: Write the failing tests.**
  - `settingsTest.c` layout table: replace `{ offsetof(settings_t, reserved11), 406 }` with
    `{ offsetof(settings_t, rs_blink), 406 }, { offsetof(settings_t, reserved11), 407 }`.
  - `radioCoreTest.c` `test_standby_blink_stored`, following how other tests reboot with RAM settings (`boot()`):
    - `blink_every(10)`, let the save run, `boot()`: `sm.set.standby_blink_s == 10`.
    - `blink_every(0)`, save, `boot()`: it is 0, so Off survives a restart and doesn't come back as the default.
    - `settings_begin()->rs_blink = 0` (an older record), `boot()`: it is 5.
    - Review Focus 5: `rs_blink = 201`, `boot()`: it is 60.

- [ ] **Step 2: Run and see them fail**

  Run: `make -C tests/host -j4 settingsTest radioCoreTest && tests/host/settingsTest && tests/host/radioCoreTest`
  Expected: the build fails because `rs_blink` isn't a member.

- [ ] **Step 3: Implement.**
  - `settings.h`: `uint8_t rs_blink; // radio_settings_t.standby_blink_s + 1, 0 = never set: the default (was
    reserved11[0]: an older record reads 0)`, then `uint8_t reserved11[1]; // 0`.
  - `settings_to_radio`: before the `rs_stored != 1` return, as `rs_bcast_step` does it, `if (st->rs_blink != 0)`
    take `rs_blink - 1u` and clamp.
  - `radio_to_settings`: store `standby_blink_s + 1u`.

- [ ] **Step 4: Run and see them pass**

  Run: `make -C tests/host -j4`
  Expected: all pass.

- [ ] **Step 5: Commit**

  ```bash
  git add core/settings/settings.h core/radio/radio_sm.c tests/host/settingsTest.c tests/host/radioCoreTest.c
  git commit -m "Settings store: keep the standby blink interval in reserved11[0]"
  ```

### Task 3: Settings > Display > Standby blink

**Files:**
- Modify: `core/ui/ui_svc.h`: `UI_SET_STANDBY_BLINK_S`, inserted just before `UI_SET_RAM_COUNT`.
- Modify: `core/ui/ui_svc_stub.c`: `[UI_SET_STANDBY_BLINK_S] = RADIO_BLINK_DEFAULT_S` in `settings[]`.
- Modify: `core/ui/ui_svc_storage.c`: `setting_get_radio` (return `rs.standby_blink_s`) and the radio-routed case
  list in `ui_svc_setting_set`.
- Modify: `core/ui/ui_svc_radio.c`: the set case. Clamp to `RADIO_BLINK_MAX_S`, as the radio core does.
- Modify: `core/ui/ui_menus.c`: `v_blink`, `n_blink`, and the `s_display` row.
- Modify: `core/ui/lang/es.po fr.po it.po zh.po` (`tools/i18n.py --sync`, then the translations).
- Modify: `tests/host/uiFlowTest.c`: renumber the shifted ids and add the new flows.
- Modify: `docs/user/17-settings.md`, `docs/i18n/zh/user/17-settings.md`.
- Regenerate: `docs/ux/screens` (only the Settings > Display images that change).

**Interfaces:**
- Consumes: `radio_settings_t.standby_blink_s`, `RADIO_BLINK_DEFAULT_S`, `RADIO_BLINK_MAX_S` (Task 1).
- Produces: `UI_SET_STANDBY_BLINK_S` (`ui_setting_t`), its value in seconds.

- [ ] **Step 1: Write the failing UI flows** in `uiFlowTest.c`, in the style of the `S language` flows next to it.
  Settings > Display is `ok 7`, and `2 up` lands on Language, so `2 up up` is the new row.
  - `S standby blink: default 5 s`: open the row's list; expect `shows=Standby_blink` and `setNN=5`, where `NN` is
    `UI_SET_STANDBY_BLINK_S`'s number.
  - `S standby blink: Off`: pick Off; expect `setNN=0`.
  - `S standby blink: 2 s`: pick 2 s; expect `setNN=2`.

- [ ] **Step 2: Run and see them fail**

  Run: `make -C tests/host -j4 uiFlowTest && tests/host/uiFlowTest`
  Expected: the build fails on `UI_SET_STANDBY_BLINK_S`, or the flows fail on the missing row.

- [ ] **Step 3: Implement.**
  - The id goes before `UI_SET_RAM_COUNT`, so it gets a stub RAM slot and the UI tests can set it. That moves every
    id from `UI_SET_RAM_COUNT` on up by one: today 85 and above (`UI_SET_LANGUAGE` is 78; confirm by counting).
    Renumber every `setNN=` check in `tests/host/*.c` with `NN` at or above the old `UI_SET_RAM_COUNT`, and change
    nothing else in those flows.
  - Menu tables:
    - `static const int16_t v_blink[] = { 0, 2, 5, 10 };`
    - `static const tr_id_t n_blink[] = { TRN(VALUE_OFF, "Off"), TRX(VALUE_2_S, "2 s"), TRX(VALUE_5_S, "5 s"), TRX(VALUE_10_S, "10 s") };`
    - These reuse existing keys. The only new key is the label.
  - Row, last before the Language `ACTION`, after the `CONFIG_DMR` Talkgroup name first row:
    `LIST(TRN(SET_LABEL_STANDBY_BLINK, "Standby blink"), "LED heartbeat light on", UI_SET_STANDBY_BLINK_S, v_blink, n_blink),`
  - Find how a `LIST` row shows a stored value that isn't in its list (60 through the debug link), and make sure it
    doesn't crash. If it does, map an off-list value to the nearest choice in the getter.
  - Run `python3 tools/i18n.py --sync`, then fill in `msgstr` for `SET_LABEL_STANDBY_BLINK`:

    | Catalog | Translation |
    |---|---|
    | es | "Parpadeo en espera" |
    | fr | "Clignotement veille" |
    | it | "Lampeggio in attesa" |
    | zh | "待机亮光" (the CJK face lacks 闪 and 烁; see the ledger ruling) |

    Each must fit the row on the smallest display in the gallery. Shorten it if it is cut off.

- [ ] **Step 4: Run and see them pass**

  Run: `make -C tests/host -j4`, then `python3 tools/i18n.py --check`
  Expected: all pass, and no missing translation.

- [ ] **Step 5: Regenerate the gallery and docs.**
  - Run `make -C tests/host ui-gallery`. Stage only the Settings > Display PNGs that changed, and check with
    `git status docs/ux/screens` that nothing else moved.
  - `docs/user/17-settings.md`: after the Talkgroup name first row, add
    `| Standby blink | Off, 2 s, 5 s, 10 s | 5 s | The green LED blinks briefly at this interval while nothing else lights it, so you can see the radio is on with the screen dark | Untested |`.
  - Use that table's own word for untested if it has one. Add the same row in Chinese in
    `docs/i18n/zh/user/17-settings.md`.
  - Run `python3 tools/check_doc_links.py` and `python3 tools/check_generic.py`. Expected: clean.

- [ ] **Step 6: Commit**

  ```bash
  git add core/ui core/ui/lang tests/host/uiFlowTest.c docs/user/17-settings.md docs/i18n/zh/user/17-settings.md <the changed PNGs>
  git commit -m "Settings > Display > Standby blink: Off, 2, 5 or 10 s"
  ```

### Task 4: Debug link view, SETTING id and HIL test

**Files:**
- Modify: `core/debuglink/debuglink_view_radio.c`:
  - The `features` section: `leds=%u` (the `radio_view_t.leds` bits: 1 green, 2 red) and `led.blinks=%u`, on the
    line with `rx_sleeping`.
  - `set.standby_blink_s=%u`, with `set.rx_save`.
  - `set_one` case 38.
- Modify: `tools/rfoundry_debug.py`: `"standby_blink_s": 38` in `SETTINGS`.
- Modify: `docs/spec/debuglink.md`: the `features` section keys, and RADIO op 24's id list.
- Modify: `tests/hil/features.toml`: the `[feature.standby_blink]` entry.
- Test: `tests/hil/test_radio_features.py`.

**Interfaces:**
- Consumes: `radio_settings_t.standby_blink_s`, `RADIO_BLINK_MAX_S`, `radio_view_t.leds` and `led_blinks` (Task 1).
- Produces: view keys `leds`, `led.blinks` and `set.standby_blink_s`. SETTING id 38: 0..60, above that a bad argument.

- [ ] **Step 1: Confirm id 38 is free.** Grep the SETTING dispatch for every id handled outside `set_one`
  (29, 31, 32, 35, 36, 37 are). If 38 is taken, use the next free id, and use it in every later step.

- [ ] **Step 2: Write the failing host check.** Find the host test that reads the `features` section text (grep
  `tests/host` for `set.rx_save=`). Add checks for `set.standby_blink_s=5`, `leds=`, `led.blinks=`, and that
  SETTING 38 with 61 answers a bad argument.

- [ ] **Step 3: Run and see it fail**

  Run: `make -C tests/host -j4`
  Expected: the new checks fail.

- [ ] **Step 4: Implement** the view keys, `case 38: if (v > RADIO_BLINK_MAX_S) return false; s->standby_blink_s = (uint8_t)v; break;`
  and the `rfoundry_debug.py` entry. Then run `make -C tests/host -j4`. Expected: all pass.

- [ ] **Step 5: Write the HIL test `test_standby_blink(radio)`** in `test_radio_features.py`, marked
  `@pytest.mark.feature("standby_blink")`.
  - `on_channel(radio, 1)`.
  - Skip with `skip("not_built", "...")` when `led.blinks` isn't in `features(radio)`.
  - Skip with `skip("state", "...")` unless `state` in the `core` view is `idle`.
  - Under `settings(radio, standby_blink_s=2)`: read `led.blinks`, wait 6.5 s, read it again. The difference is 3
    (accept 2..4). If `state` left idle during the wait, skip with `state`.
  - Under `standby_blink_s=0`: the difference over 5 s is 0.
  - It counts instead of sampling `leds`, because a 100 ms blink falls between debug link reads.

- [ ] **Step 6: Add the features.toml entry** in the Radio and voice part:

  ```toml
  [feature.standby_blink]
  spec = "docs/spec/radio-features.md"
  source = ["core/radio/radio_sm.c", "core/ui/ui_menus.c", "core/debuglink/debuglink_view_radio.c"]
  ```

  Then run `python3 tools/bringup.py check` and `python3 -m pytest tests/hil -q -rs -k standby_blink`.
  Expected: the check passes, and the test skips because no radio is connected.

- [ ] **Step 7: Update `docs/spec/debuglink.md`.**
  - The `features` row: `leds` (bit 0 green, bit 1 red, what the core drives), `led.blinks` (standby blinks since
    start-up, `radio-features.md` 5.7) and `set.standby_blink_s`.
  - RADIO op 24: `38 standby_blink_s (0-60, 0 off; radio-features.md 5.7)`.
  - In `radio-features.md` 5.7, change "debug link `leds`" to "debug link `leds` in the `features` section".
  - Run `python3 tools/check_doc_links.py`. Expected: clean.

- [ ] **Step 8: Commit**

  ```bash
  git add core/debuglink/debuglink_view_radio.c tools/rfoundry_debug.py docs/spec/debuglink.md docs/spec/radio-features.md tests/hil tests/host
  git commit -m "Debug link: LED state, blink count and SETTING 38 for the standby blink; HIL test"
  ```

### Task 5: Build switch `CONFIG_STANDBY_BLINK`

Added 2026-11-27 at the owner's request: the site's firmware builder offers features by their build switches, and
this one should be selectable the same way. Spec: the "Build switch" section of the design note.

**Files:**
- Modify: `config/features.mk` (a commented `CONFIG_STANDBY_BLINK ?= y` near `BATTERY_SAVE`, and the name in
  `FEATURES` next to it), `core/config.h` (`#ifndef CONFIG_STANDBY_BLINK` / `#define CONFIG_STANDBY_BLINK 1`),
  `core/hostlink/build_info.c` (`"STANDBY_BLINK,"` under `#if CONFIG_STANDBY_BLINK`).
- Modify: `core/radio/radio_sm.c` (the heartbeat and its call), `core/ui/ui_menus.c` (the row and its tables),
  `core/ui/ui_svc_storage.c` / `ui_svc_radio.c` (get/set cases), `core/debuglink/debuglink_view_radio.c`, all under
  `#if CONFIG_STANDBY_BLINK`.
- Keep unconditional: `radio_settings_t.standby_blink_s`, its default, clamp, load and save, `settings_t.rs_blink`,
  `UI_SET_STANDBY_BLINK_S`, the stub default, and the view section number of `leds`.
- Modify: `docs/build-config.md` (a row), `docs/feature-matrix.md` (a row in Built by default on each radio, from
  the tool that generates it if there is one, and a row in Radio and voice with bytes measured as its header says),
  `tests/hil/features.toml` (`switch = "STANDBY_BLINK"`), `docs/spec/radio-features.md` 5.7 and `docs/spec/debuglink.md`
  (what a build without it shows).
- Test: guard the host tests that need the feature with `#if CONFIG_STANDBY_BLINK`, as other switch tests do.

**Interfaces:**
- Consumes: everything from Tasks 1-4.
- Produces: `CONFIG_STANDBY_BLINK` (0/1). With it 0:
  - the green LED shows only what it showed before this branch.
  - SETTING 38 answers unavailable, as `sfr_cwid` does without its switch.
  - the `leds` section still prints `leds` and adds `blink.built=0` in place of the blink keys.
  - the HIL test skips as `not_built` when the radio's VERSION `features=` lacks `STANDBY_BLINK`.

- [ ] **Step 1: Register the switch** in the three build files, then run `make -C tests/host -j4` (switch on).
  Expected: all pass, as before.
- [ ] **Step 2: Guard the code.** Build each radio with the switch off and confirm 0 warnings:
  `make BOARD=<b> CONFIG_STANDBY_BLINK=n BUILD=build/t-blink-off` for each board in FORK.md's "Before merging"
  list, then `tools/ci.sh --board <b> --matrix --steps firmware` for one of them. The off image's `text` must be
  within a few bytes of main's (702524 / 752352), which shows nothing of the feature remains. Remove `build/t-*`.
- [ ] **Step 3: Host tests with the switch off.** Find how the host tests build a configuration with a switch off.
  If `tools/ci.sh` or the host Makefile has a pattern, use it to build the radio-core and UI flow tests with
  `CONFIG_STANDBY_BLINK=0` once, and fix whatever fails to build or pass. If there is no such pattern, say so in the
  report instead of inventing one.
- [ ] **Step 4: Docs and the feature checklist.**
  - Add the `build-config.md` row: `| CONFIG_STANDBY_BLINK | y | the green LED's standby blink and its Settings > Display row (docs/spec/radio-features.md 5.7) |`.
  - Add the `feature-matrix.md` rows.
  - Add `switch` to `[feature.standby_blink]`.
  - Run `python3 tools/bringup.py check`, `python3 tools/boards.py check`, `python3 tools/check_doc_links.py`,
    and the site's own checks: `cd site && python3 -m pytest -q`, if it runs offline, and the site i18n check if the
    feature title is translated there.
  - Expected: all clean.
- [ ] **Step 5: Commit**

  ```bash
  git add config/features.mk core docs tests/hil/features.toml tests/host
  git commit -m "Build switch CONFIG_STANDBY_BLINK, so the firmware builder can leave the standby blink out"
  ```

### Task 6: Size, full CI, pull request

**Files:** none new. Builds go under `build/`; remove them when done.

- [ ] **Step 1: Build both radios and measure** the default images (the two builds in FORK.md's "Before merging"
  list) with `arm-none-eabi-size`. Expected: 0 warnings, and `text` within about 1 KB of main's on each.
- [ ] **Step 2: Run the full CI.** Run `JOBS=4 tools/ci.sh`. Expected: it ends with "all checks passed". Then
  `rm -rf build/ci`.
- [ ] **Step 3: Push and open the PR in the fork** (the owner merges it; never merge it yourself).
  - Check first with `git diff --cached --stat -- third_party` (empty) and `git status` (clean).
  - Run `git push -u origin feat/standby-blink`.
  - Run `gh pr create -R zyphost/BourneAgainRadioFoundry --base main`.
  - The body lists: what it does, the switch, the size growth per radio, the translations (`- [x] es, fr, it, zh`),
    "HIL test written, untested on the radio", and the spec and plan paths.
  - It ends with the Claude Code attribution line.
