<!-- SPDX-License-Identifier: MIT -->
<!-- Copyright (c) 2026 The Bourne Again RadioFoundry Authors -->

# Working on this fork

Bourne Again RadioFoundry is a personal fork of [RadioFoundry](https://github.com/radiofoundry/radiofoundry). This page
covers how its branches, merges and upstream syncs work. The rules for the code itself are upstream's:
[AGENTS.md](AGENTS.md) and [docs/dev/contributing.md](docs/dev/contributing.md).

## Branches

| Branch | Holds | Changes by |
|---|---|---|
| `main` | Upstream plus this fork's changes. It always builds clean, and the radios are flashed from it. | Pull request only, merged with a merge commit |
| `feat/<topic>` | One modification | Commits, then a pull request into `main` |
| `fix/<topic>` | One bug fix | The same |
| `chore/<topic>` | Fork housekeeping: docs, tooling, repository settings | The same |
| `sync/upstream-<yyyymmdd>` | `main` with upstream's `main` merged in | A pull request into `main` |

A topic branch starts from the current `main` and holds one change. It lives until its pull request is merged, and
GitHub deletes it then.

## Rules on `main`

A repository ruleset ("Protect main") enforces these:

- No direct pushes, force pushes or deletion.
- Every change arrives through a pull request. No approval is required, since one person develops here.
- Merge commits only. Squash and rebase merging are off. Either one would rewrite upstream's commits during a sync, and
  every later sync would then conflict. `git log --first-parent main` lists each change and sync in order.
- No bypass, not even for the owner. Scripts and coding agents push with the owner's account, so a bypass would apply
  to them too.

## Making a change

```
git switch main && git pull
git switch -c feat/<topic>
# edit, then build and test as in AGENTS.md, "Build and test"
git push -u origin feat/<topic>
gh pr create -R zyphost/BourneAgainRadioFoundry --base main --fill
```

Before merging:

- Every radio listed in the [README's fork notice](README.md) builds with zero warnings.
- `make -C tests/host` passes.
- For a UI change, the gallery is regenerated (`make -C tests/host ui-gallery`), with only the intended PNGs committed.
- For a behavior change, it has been tried on a radio.

Then merge on GitHub ("Create a merge commit").

## Syncing with upstream

```
git fetch upstream
git switch -c sync/upstream-$(date +%Y%m%d) origin/main
git merge upstream/main          # resolve any conflicts here, then build and test as above
git push -u origin HEAD
gh pr create -R zyphost/BourneAgainRadioFoundry --base main --title "Sync upstream $(date +%Y-%m-%d)" --body ""
```

Sync through a branch instead of GitHub's "Sync fork" button. The merge then gets built and tested before it reaches
`main`.

## Upstream

- The `upstream` remote is fetch-only: its push URL is `DISABLE`
  (`git remote set-url --push upstream DISABLE`).
- This fork sends no pull requests, issues or comments upstream for now.
- In this fork, `gh` defaults to the parent repository. Pass `-R zyphost/BourneAgainRadioFoundry` every time.

## Fork-only changes

Branding is kept small so upstream merges stay clean:

- The About page's firmware name (`core/ui/ui_menus.c`).
- The banner at the top of `README.md`.
- This file.

Internal names stay upstream's on purpose: `rfoundry_*` images, `RFOUNDRY_*` macros, the `rfoundry` tool, and the USB,
host link and protocol identity strings. Tools and other software read those.

## Fork-owned numbers

The standby blink takes numbers upstream may use for something else. They will conflict textually at an upstream sync
and must be renumbered then.

| Number | Where | Use |
|---|---|---|
| SETTING 200 | debug link, RADIO op 24 (`core/debuglink/debuglink_view_radio.c`, `tools/rfoundry_debug.py`) | `standby_blink_s`. Ids from 200 are the fork's |
| View section 39, `leds` | debug link view sections (`core/debuglink/debuglink.h`) | the LEDs and the blink count. It sits in the contiguous range, so it can collide with the next upstream section |
| Settings byte 406, `rs_blink` | settings store page 1 (`core/settings/settings.h`) | the blink interval + 1. It was `reserved11[0]`, so every record this fork writes has a non-zero byte 406, which upstream's firmware ignores as reserved |
| `UI_SET_STANDBY_BLINK_S` | `core/ui/ui_svc.h` | the id after the last upstream id, just before `UI_SET_COUNT`, so upstream's ids keep their numbers. A new upstream id lands in the same place |

## CI

GitHub Actions is turned off for this repository (Settings > Actions). The workflows in `.github/workflows` are
upstream's. They run on upstream's own runners and publish its releases and site, so none of them applies here.
Builds and tests run on the developer's machine, with the toolchains described in
[docs/dev/building.md](docs/dev/building.md). To turn Actions back on:
`gh api -X PUT repos/zyphost/BourneAgainRadioFoundry/actions/permissions -F enabled=true`.
