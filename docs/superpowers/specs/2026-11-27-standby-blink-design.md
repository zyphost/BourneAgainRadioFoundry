<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 The Bourne Again RadioFoundry Authors -->

# Standby blink: design

Status: proposed, 2026-11-27. Size: a small change.

## Intent

The green LED gives a short blink at a set interval while the radio is on and nothing else has the LED. The user can
then see that the radio is on even with the screen dark. The owner uses another handheld that does this and wants the
same here. Some users won't want it, so it has a setting.

## Behavior

- Every interval, green lights for 250 ms (`RADIO_LED_BLINK_MS`). The first build used 100 ms (`RADIO_LED_WAKE_MS`,
  the repeater wake-up's length). On the radio that was too quick to catch, so the owner chose 250 ms (2026-11-27).
- It blinks only while both LEDs would otherwise be dark: idle on a channel, scanning, or the receiver in battery
  save. That holds whether the screen is on or off.
- Everything the LEDs show today wins over it: solid green while a reception plays, red while transmitting, and the
  timed signals (stuck PTT, TX deny, repeater wake-up).
- The interval counts from the moment the LEDs last went dark. The first blink after a call comes a full interval
  later, so the end of a call never looks like a stutter.
- Every radio that has a green LED gets it. Nothing is radio-specific.

## Setting

| | |
|---|---|
| Where | Settings > Display, a new row after Hint row and before Language (Language stays last) |
| Label | "Standby blink" |
| Choices | Off, 2 s, 5 s, 10 s |
| Default | 5 s (on out of the box) |
| Saved | with the radio settings, so it survives a power cycle and a reflash |

## Build switch (added 2026-11-27, owner's request)

The feature is a build switch, like the other features the site's firmware builder offers: `CONFIG_STANDBY_BLINK`.
The builder reads `config/features.mk`, `docs/feature-matrix.md` and `core/hostlink/build_info.c`, so registering
the switch there is what makes it selectable.

| | |
|---|---|
| Switch | `CONFIG_STANDBY_BLINK` in `config/features.mk` (FEATURES list) and `core/config.h` |
| Default | on (`y`) for every radio: no hardware beyond the green LED every radio has |
| Off | no heartbeat code, no Settings > Display row, SETTING 200 answers unavailable, the debug link shows the blink as not built, and the build stays clean with zero warnings |
| Kept with it off | the `radio_settings_t` field, the settings-store byte and the `UI_SET_` id, so a setting chosen in one build survives a reflash to a build without the switch, and no id or layout depends on the switch |
| Registered in | `config/features.mk`, `core/config.h`, `core/hostlink/build_info.c` (VERSION `features=`), `docs/build-config.md`, `docs/feature-matrix.md` (both tables), `tests/hil/features.toml` (`switch = "STANDBY_BLINK"`) |

## Constraints

- Hard rules from AGENTS.md: one stack (no per-board code), US English, zero warnings in every `tools/ci.sh`
  configuration, an SPDX header on new files, translated user text.
- Fork hygiene: keep the diff small and self-contained, so upstream syncs merge cleanly.
- Flash: the fullest image is at about 95.5 %. Measure `text` before and after on each built radio. Measured before
  the switch: +568 B and +144 B on the owner's two radios. The feature sits behind `CONFIG_STANDBY_BLINK` (above)
  anyway, so a build can leave it out.
- Settings store compatibility: a record written before this change must read as the default (5 s on). Appended
  fields and `reserved` bytes read 0 in an older record, so a stored 0 must mean "the default", and Off needs its
  own non-zero code.
- The blink must not change the receive path. `rx_open`, the speaker and dual watch's audio hand-off all take green
  before the timed signals are applied, and must stay that way.

## Approaches considered

| Approach | Trade-off | Verdict |
|---|---|---|
| The lowest-priority timed signal in `led_signals()` (`core/radio/radio_sm.c`) | One place, every radio, runs in the host tests, follows the existing pattern for timed signals | **Chosen** |
| Blink from the UI, only while the backlight is off | The UI owns the backlight but not the LEDs. It would fight `outputs()`, which writes only on change, and needs the backlight state passed into the radio core | Dropped: blinking any time idle meets the goal |
| A timer in each board's power driver | A copy per board, against the one-stack rule | Dropped |

## What it touches

- `core/radio/radio.h`: a field in `radio_settings_t`, with its default and range check in `radio_sm.c`.
- `core/radio/radio_sm.c`: the heartbeat in `led_signals()`. `led_timed` stays true while it is enabled, so the
  tick keeps calling `outputs()` when nothing else changes. `view.leds` shows the blink, because it reports what is
  driven.
- `core/settings/settings.h`: one byte, taken from `reserved11` (the pattern `satellite_index` used), with its
  load and save next to the other `rs_` fields.
- `core/ui/ui_svc.h`, `ui_svc_radio.c`, `ui_svc_stub.c`, `ui_menus.c`: the `UI_SET_` id, its get and set, and the
  Display row.
- The debug link: the setting under `set.` in the radio view and settable, so a HIL test can turn it on and watch
  `leds`.
- Translations: the label and any new value names, synced with `tools/i18n.py --sync` and translated into every
  catalog (`core/ui/lang/*.po`, plus `site/i18n/*.json` if the site shows it).
- Docs: `docs/spec/radio-features.md` 5.7 (a row for the heartbeat), the settings list in `docs/user/17-settings.md`,
  and its translated copies under `docs/i18n/` if they carry that list.

## Tests

- Host (`tests/host/radioCoreTest.c`, next to `test_dmr_wake_leds`):
  - It blinks on schedule while idle.
  - It never blinks while green or red is solid, or during a deny or wake-up signal.
  - The first blink after a reception comes a full interval later.
  - Off never blinks.
  - An older settings record reads 5 s.
  - `rx_open` and the audio never follow the blink.
- UI: regenerate the gallery (`make -C tests/host ui-gallery`) and commit only the Settings > Display images that
  changed. No new screen, so no new `uiFlowTest` flow.
- HIL: an opt-in test that sets 2 s through the debug link and watches `leds` toggle while idle. Plus a
  `tests/hil/features.toml` entry and a `@pytest.mark.feature` marker.
- `JOBS=8 tools/ci.sh` ends with "all checks passed". `python3 tools/check_doc_links.py` is clean.

## Non-goals

- No red heartbeat, no per-channel option, no brightness control.
- No "only with the screen off" mode.
- No change to any existing LED signal.

## Anti-goals

- A blink that looks like a call.
- Any effect on receive audio, squelch or scan timing.
- A noticeable battery cost. At 250 ms every 5 s the LED is lit 5 % of the time.

## Left to the builder

- The stored encoding for Off and the three intervals (0 must mean the default).
- Whether the value names reuse existing translation keys, such as `VALUE_OFF`.
- Where in `led_signals()` the "last went dark" time is recorded.
