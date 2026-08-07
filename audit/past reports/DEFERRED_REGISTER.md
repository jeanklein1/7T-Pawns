# DEFERRED REGISTER

Durable list of intentionally-deferred work: items that are correct to postpone,
with the reason and the trigger for picking them up. "Queued" in a commit or
charter means "recorded here."

## Open


### D2 — Derive `FLOATER_EVICTION_RADIUS` from the allocation radius
- **State:** the eviction radius (GPU const, `world.wgsl`) must exceed the patch
  allocation radius (CPU quantity, `active_radius × PATCH_EXTENT`). The relation
  is real but **unassertable** — the two operands live in different rooms.
- **Why deferred:** deriving it CPU-side and uploading through `config` adds a
  `DesignConfig` field (~592 B C++/WGSL mirror) — structural work that does not
  belong in a diagnostic/fix batch.
- **Trigger:** a dedicated change that puts both values in one room, making the
  relation `const_assert`-able (COLLISION_CHARTER, the feasibility corollary + the
  containment rule).

## Closed

### D1 — Web mirror resync (`web/shaders/world.wgsl`) — **VOID**
- **Closed:** PRUNING_1 P1 Step 2, ruling R1. There is no mirror to resync:
  `web/` was deleted in full and the Mirror doctrine is dead, not suspended.
- **Consequence for the item that motivated it:** `FLOATER_EVICTION_RADIUS`
  now lives in **two** rooms, not three, and both desktop rooms already agree
  at 800. The divergence this entry existed to track is gone with its third
  room — closed by deletion, not by doing the work.
- **What survives:** `audit/WEB_PORT_LEDGER.md` — the last-good commit and the
  binding-closure lesson, which is the part a rebuild would otherwise pay for
  again. Note in particular that the ritual this entry deferred (`cp` + `gzip`
  + sidecar sha) was **unsound**: it would have failed at pipeline creation,
  because entry-point existence is not the test — binding closure is.
