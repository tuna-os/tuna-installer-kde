# tuna-installer-kde — Design

Qt6 Widgets frontend for KDE Plasma. Reads `../INSTALLER-FRONTENDS.md` for the
shared wizard flow and recipe contract; this file covers only how it looks and
feels.

## Direction

A Plasma-native installer that borrows Calamares' familiar shape (left rail +
content pane) but earns its own identity from the subject: installing an OS is
a **descent** — you go deeper with every step until the payload reaches the
disk. The left rail is a nautical **depth gauge**, not a checklist.

Everything else defers to Breeze. Boldness lives in the rail; the content pane
is disciplined KDE HIG.

## Signature element: the depth gauge rail

- Fixed 220 px left rail with a vertical gradient from `--surface` (top) to
  `--abyss` (bottom) — the water column.
- Steps are depth markers: a tick, a small-caps label, and a depth reading
  (`0 m`, `−400 m`, … `−2000 m` at Done). Completed steps sit above the
  "waterline" indicator; the current step's tick glows `--sonar`.
- A thin animated line (200 ms ease-out on step change) descends between
  ticks. `prefers-reduced-motion` (Qt: `QAccessibility`/env override) disables
  the animation, never the state change.
- During Progress (step 7), the rail's waterline maps to the 9 fisherman
  pipeline steps — the descent finishes exactly when the install does.

Do not add waves, bubbles, fish icons, or any second aquatic flourish. One
metaphor, executed precisely.

## Tokens

| Token | Hex | Use |
|---|---|---|
| `--abyss` | `#0B1B2B` | Rail bottom, dark anchor |
| `--surface` | `#1E4F6E` | Rail top |
| `--sonar` | `#2EC4B6` | Active step, focus, primary accent |
| `--catch` | `#F4A259` | Destructive/attention (wipe-disk warning, Install button) |
| `--paper` | Breeze `window` | Content pane bg (follow system light/dark) |
| `--ink` | Breeze `windowText` | Content text |

Content pane uses Breeze roles so system light/dark and accent-color settings
keep working; only the rail and the two accents are ours.

## Type

- UI text: system font (Noto Sans on Plasma) via `QApplication::font()` — no
  overrides in the content pane.
- Rail labels + depth readings: **Hack**/monospace fallback
  (`QFontDatabase::systemFont(FixedFont)`), small caps via letter-spacing +
  uppercase, 9.5 pt. The instrument-panel voice lives only in the rail.
- Page titles: system font, 1.6× base size, weight 600. No display face.

## Layout

```
┌──────────┬──────────────────────────────────────────────┐
│   0 m ─  │  Destination                                  │
│ WELCOME  │                                               │
│ −400 ─   │  ┌──────────────────────────────────────────┐ │
│ SOURCE   │  │ ⬤ Samsung SSD 990 PRO   1.0 TB   nvme0n1 │ │
│ −800 ─▶  │  │ ○ WD Blue HDD           2.0 TB   sda     │ │
│ DESTIN.  │  └──────────────────────────────────────────┘ │
│ −1200 ─  │  ⚠ Everything on the selected disk will be    │
│ SETUP    │    erased.                                    │
│ −1600 ─  │                                               │
│ CONFIRM  │                          [ Back ]  [ Next ]   │
│ −2000 ─  │                                               │
│ DONE     │                                               │
└──────────┴──────────────────────────────────────────────┘
```

- Content pane: 560–720 px column, 24 px page margins, 12 px control spacing
  (KDE HIG). Buttons bottom-right, Back left of Next, `Install` styled with
  `--catch` and a confirm-by-typing-hostname guard is *not* used — the single
  Confirm page is the guard.
- Disk rows: `QListView` with 44 px rows — model name primary, size + kernel
  name secondary in `--ink` at 70 % opacity.

## Copy

Active voice, second person, no nautical puns in body copy (the rail carries
the theme; words carry information). "Everything on this disk will be erased."
not "Prepare to make a splash!". Button labels: Next / Back / Install / Reboot.

## Quality floor

Keyboard: full tab order, Enter advances, Esc never destructive. All rail
state changes mirrored in an off-screen accessible label. Passphrase fields:
paired entry + reveal toggle, caps-lock warning. Respect Breeze dark mode in
the content pane; the rail is dark in both themes by design.
