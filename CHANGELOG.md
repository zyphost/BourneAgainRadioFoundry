<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 The Bourne Again RadioFoundry Authors -->

# Changelog

What Bourne Again RadioFoundry adds or changes on top of [RadioFoundry](https://github.com/radiofoundry/radiofoundry).
Upstream's own changes are in its [GitHub releases](https://github.com/radiofoundry/radiofoundry/releases) and are not
repeated here.

- **Based on:** upstream `main` at `3ebdbe50` (2026-11-26).
- Newest first. Each pull request into `main` adds its own entry ([FORK.md](FORK.md#making-a-change)).
- An upstream sync adds one line: "Synced with upstream at `<commit>`".
- Entries stay under **Unreleased** until the fork tags a release.

## Unreleased

### Standby blink (#4)

2026-11-27. All radios.

- The green LED blinks for 500 ms at a set interval while the radio is on and both LEDs would otherwise be dark:
  idle on a channel, scanning, or the receiver in battery save, with the screen on or off. It shows the radio is on
  even with the screen dark.
- Every other LED signal wins: a call being received, transmitting, a TX deny, a repeater wake-up, power-off. The
  first blink after a call comes a full interval later.
- **Settings > Display > Standby blink:** Off, 2 s, 5 s, 10 s. Default 5 s. Saved with the radio settings, so it
  survives a power cycle and a reflash.
- Build switch `CONFIG_STANDBY_BLINK`, on by default and offered by the site's firmware builder.
- Debug link: view section `leds` and SETTING 200, both fork-owned numbers (listed in FORK.md).
- Translated: es, fr, it, zh.

### Sample name in the screenshots (#5)

2026-11-27. No firmware change.

- The manual's screenshots show the sample boot text `Q0TEST` / `Bart Simpson` instead of `Q0TEST` /
  `Jamie Thompson`. Both are made-up samples. A radio shows the boot text from its own codeplug.

### Fork name (#3)

2026-11-27. All radios.

- **About:** "Firmware: Bourne Again RadioFoundry", with the fork's copyright line below upstream's.
- The README opens with a note that this is a personal fork of RadioFoundry.
- Unchanged on purpose: the license line, `AUTHORS`, and the names tools and protocols read (USB, host link, MMDVM,
  APRS-IS, mDNS, the `rfoundry` tool and the image names).

### Fork workflow (#2)

2026-11-27. No firmware change.

- `FORK.md`: the fork's branches, the rules on `main` (pull requests and merge commits only), and how upstream syncs
  work.
