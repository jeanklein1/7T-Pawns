# TRUEBAND_CONTACT_1 — CAMPAIGN LOG

Campaign: TRUEBAND_CONTACT_1 (campaign v2 Stages 6 + 7, designed on
AUDIT-3; handoffs src/docs/HANDOFFS/T 0-3/). Branch: `TRUEBAND_CONTACT_1`.

---

## T0 — INDEX + PREFLIGHT

### Base

Cut from `732b1a90a051ba1bbfcdd0b1bb3a191dbcc232c1` — the
UNIFIED_GROUND_1_AUDIT3 tip (Jean's designation by push placement: the
T handoffs landed there; src/cartridges identical to the U6 closeout).
Anchor source AUDIT3_REPORT.md §A3-3/§A3-4 in-tree.

### Anchor table

| # | Anchor | Expect | Found | Verdict |
|---|--------|--------|-------|---------|
| a | `fn write_live_card` | 1 | 1 | PASS |
| b | `fn terrain_band_contribution` | 1 def | 1 | PASS |
| c | `fn evaluate_lattice_wave` | 1 def | 1 | PASS |
| d | `PAWN_GOL_GROUND_ENABLED` (the fossil) | count ALL | **exactly 1** — world.wgsl:2086 `const PAWN_GOL_GROUND_ENABLED: bool = false;    // Pawn walks on GoL extrusions` — REFERENCE-FREE ⇒ the T2a tombstone path | PASS |
| e | `const OVERLAY_WAVES` | 1 | 1 | PASS |
| f | `fn get_band_blend` | 1 def | 1 | PASS |
| g | `fn behavior_flock2d` | 1 | 1 | PASS |
| h | agents.hpp `AGENT_TIER_GAINS[AGENT_TIER_COUNT]` | 1 | 1 | PASS |
| i | state.hpp `float _pad[2];         // 24-31` | 1 | 1 | PASS |
| j | `fn update_sphere` / `fn update_cube` | 1 each | 1 / 1 | PASS |

All PASS → T1. Language law noted: `active` reserved (A3-5); new
fields follow the house `is_active` shape.

### Baseline gate

glaw1 at base 732b1a9, before any edit: `G-LAW 1: GREEN`.
