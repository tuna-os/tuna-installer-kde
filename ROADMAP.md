# TunaOS KDE Installer — Roadmap

**Last updated**: 2026-08-24 | **Maintainer**: tuna-os (hanthor)

---

## Mission

Ship the KDE desktop's install experience: a thin Qt 6 / Kirigami wizard
(built the way KDE's own KISS initial-setup is built — self-contained
`SetupModule` steps) that drives the fisherman bootc backend, so a first-time
Plasma user gets a native install from first boot to desktop.

---

## Current Status

- **App**: Qt 6 / Kirigami (Plasma 6) frontend for fisherman — modular
  steps under `modules/<name>/contents/ui/main.qml`; CI-rendered walkthrough
  in docs/gui-walkthrough.md.
- **Distribution**: image-baked flatpak (`org.tunaos.InstallerKde`) — no
  standalone GitHub Releases (by design, not yet documented as policy).
- **Parity**: covered by `installer-smoke.yml` + `docs/INSTALLER-FRONTENDS.md`
  checks (readiness stamp, non-blank, advances, per-screen OCR).
- **Health**: active (pushed 08-24); open issues concentrate on install-recipe
  secrets handling (#34/#35) and the unpinned privileged backend (#33).

### Priorities

| Priority | Item | Tracking | Status |
|----------|------|----------|--------|
| P0 | Install-recipe secrets — LUKS passphrase in QTemporaryDir | #34/#35 | 🟡 Open |
| P1 | Unpin privileged install backend embedded in flatpak | #33 | 🟡 Open |
| P1 | Backend test coverage preserved | #29 | 🟡 Open |
| P2 | ROADMAP-coverage entry in org ROADMAP tally | #1295 | ⬜ Not started |

---

## Quarterly Goals

### Current Quarter (2026 Q3)

**Theme**: harden the install path

| Goal | Owner | Tracking | Status |
|------|-------|----------|--------|
| Green recipe-secrets handling (QTemporaryDir + perms) | hanthor | #34/#35 | ⬜ Not started |
| Unpin the privileged backend | hanthor | #33 | ⬜ Not started |

### Next Quarter (2026 Q4)

**Theme**: parity and cadence

| Goal | Owner | Tracking | Status |
|------|-------|----------|--------|
| Backend test coverage | hanthor | #29 | ⬜ Not started |
| Document release/versioning model (image-baked vs tagged) | tuna-os | (org #2020) | ⬜ Not started |

---

*ROADMAP added by strategist agent (ACMM L6 — full mode). Signed-off-by: hanthor-hive-agent[bot] <290068839+hanthor-hive-agent[bot]@users.noreply.github.com>*
