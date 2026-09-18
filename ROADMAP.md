# TunaOS KDE Installer — Roadmap

**Last updated**: 2026-09-17 | **Maintainer**: tuna-os (hanthor)

---

## Mission

Ship the KDE desktop's install experience: a thin Qt 6 / Kirigami wizard
(built the way KDE's own KISS initial-setup is built — self-contained
`SetupModule` steps) that drives the fisherman bootc backend, so a first-time
Plasma user gets a native install from first boot to desktop.

---

## Current Status

> ⚠️ **NOTICE**: The KDE installer source has been migrated into the monorepo at [`tuna-os/bootc-installer` → `frontends/kde/`](https://github.com/tuna-os/bootc-installer/tree/dev/frontends/kde). This standalone repository is undergoing sunset and archive reconciliation.

- **App**: Qt 6 / Kirigami (Plasma 6) frontend for fisherman — modular
  steps under `modules/<name>/contents/ui/main.qml`; CI-rendered walkthrough
  in docs/gui-walkthrough.md.
- **Distribution**: image-baked flatpak (`org.tunaos.InstallerKde`) — no
  standalone GitHub Releases (by design, not yet documented as policy).
- **Parity**: covered by `installer-smoke.yml` + `docs/INSTALLER-FRONTENDS.md`
  checks (readiness stamp, non-blank, advances, per-screen OCR).
- **Health**: Monorepo migration complete (#88); standalone repo read-only archive pending.

### Priorities

| Priority | Item | Tracking | Status |
|----------|------|----------|--------|
| P0 | Monorepo migration to bootc-installer | #88 | ✅ Complete |
| P1 | Issue and PR re-homing to bootc-installer monorepo | #88 | 🟡 In Progress |
| P1 | Standalone repo read-only archive & deprecation notice | #88 | ⬜ Planned |
| P2 | Org-wide installer roadmap tally reconciliation | #1295 | ⬜ Planned |

---

## Quarterly Goals

### Current Quarter (2026 Q3)

**Theme**: monorepo consolidation & installer hardening

| Goal | Owner | Tracking | Status |
|------|-------|----------|--------|
| Green recipe-secrets handling (QTemporaryDir + perms) | hanthor | #34/#35 | ✅ Complete |
| Monorepo migration to `bootc-installer` | tuna-os | #88 | ✅ Complete |

### Next Quarter (2026 Q4)

**Theme**: archive sunset & monorepo issue re-homing

| Goal | Owner | Tracking | Status |
|------|-------|----------|--------|
| Complete issue/PR re-homing to `bootc-installer` | hanthor | #88 | ⬜ Planned |
| Archive standalone `tuna-installer-kde` repository | tuna-os | #88 | ⬜ Planned |

---

*ROADMAP updated by strategist agent (ACMM L6 — full mode).*

