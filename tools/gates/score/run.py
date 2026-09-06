#!/usr/bin/env python3
# ─── THE SCORE CENSUS (REBUILD-0 m2; migrated onto THE FRAME SPINE at
#     FRAME_CONDUCTOR CUT 2) ──
#
# Machine-checked bijection roster <-> score. The frame is now a declarative
# SPINE TABLE (cartridge.hpp: UPDATE_SPINE / RENDER_SPINE); each row is
# {phase id, name, member fn, driver, roster gate, face}. THIS TOOL AUDITS THE
# SPINE: the manifest IS the table, attribution IS row membership. (The CUT-1
# see-through shim is retired — the auditor audits the spine, not a flattened
# conductor.)
#
# DIRECTION A (roster -> spine): every roster piece has its obligations — its
#   per-frame work as a SPINE ROW gated by its bit (FRAME_ROWS / SHARED_ROWS),
#   and its non-frame obligations (teardown / boot / mesh-prep / delegated
#   module doors) as gated call-sites, intra-phase or at boot (GREP_MANIFEST).
#   A missing or mis-gated obligation fails loud.
# DIRECTION B (spine -> roster): every spine ROW is attributed — its gate is a
#   ROSTER bit (-> that family) or `true` (-> a FOUNDATIONAL phase carrying a
#   one-line justification). The phases <-> rows bijection holds (no phase
#   without a row, no row without a phase). An unattributed row fails loud: an
#   ungated spine phase is UNWRITABLE (this is how P1 and P2 stay dead).
#
# JURISDICTION: score STRUCTURE (presence, gating, attribution, phase<->row
# bijection). The O-#/RC ORDER laws are static_asserts in the spine itself
# (glaw1's jurisdiction now); behavior is the rig's; syntax is glaw1's.

import re, sys, os

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
TB   = os.path.join(ROOT, 'src', 'cartridges', 'the_board')

def read(rel):
    with open(os.path.join(TB, rel), 'rb') as f:
        return f.read().decode('utf-8')

CART = read('cartridge.hpp')

# ── Parse the spine tables ─────────────────────────────────────────
# A row: { XPhase::Id, "name", &Cartridge::phase_fn, Driver::D, GATE, FACE },
ROW_RE = re.compile(
    r'\{\s*[UR]Phase::(\w+),\s*"(\w+)",\s*&Cartridge::(phase_\w+),\s*'
    r'Driver::(\w+),\s*(.+?),\s*(F_[^}]+?)\s*\}', re.S)

def parse_spine(table):
    m = re.search(table + r'\[\]\s*=\s*\{(.*?)\n\s*\};', CART, re.S)
    assert m, f'{table} table not found'
    rows = []
    for r in ROW_RE.finditer(m.group(1)):
        rows.append(dict(id=r.group(1), name=r.group(2), fn=r.group(3),
                         driver=r.group(4), gate=r.group(5).strip(),
                         face=r.group(6).strip()))
    return rows

UPDATE_ROWS = parse_spine('UPDATE_SPINE')
RENDER_ROWS = parse_spine('RENDER_SPINE')
ALL_ROWS = UPDATE_ROWS + RENDER_ROWS

# Declared phase methods (bijection target).
PHASE_DEFS = set(re.findall(r'void (phase_\w+)\s*\(', CART))

# The roster bits (gate families must resolve to one of these).
KNOWN_FAMILIES = {
    'pyramid', 'arch', 'column', 'antenna', 'palm', 'cactus', 'blade',
    'sphere', 'ribbon', 'cube', 'gol', 'gallery', 'pawn_aura', 'orbs',
    'spot_lights', 'indoor_shell', 'portal', 'transitions', 'wanderers',
}

def gate_families(gate):
    """the set of ROSTER families a gate references; empty if `true`."""
    if gate == 'true':
        return set()
    return set(re.findall(r'ROSTER\.(\w+)', gate))

# ── DIRECTION A (frame): per-family per-frame work as SPINE ROWS ────
# family -> [(phase fn, {required gate families})]
FRAME_ROWS = {
    'pawn_aura':   [('phase_motion_bodies', {'pawn_aura'}), ('phase_pawn_aura', {'pawn_aura'})],
    'gallery':     [('phase_witness_photographer', {'gallery'})],
    'transitions': [('phase_portal_trigger', {'transitions'})],
    'wanderers':   [('phase_respawn_agents', {'wanderers'})],
    'ribbon':      [('phase_ribbon_tick', {'ribbon'})],
    'gol':         [('phase_gol_derive_flush', {'gol'}), ('phase_gol_zone_compute', {'gol'})],
    'orbs':        [('phase_orb_sky', {'orbs'})],
}
# rows whose gate is a SHARED (multi-family) organ.
SHARED_ROWS = [('phase_promotion_drain', {'gallery', 'indoor_shell'})]

# ── DIRECTION B: foundational phases — gate MUST be `true`, justified ──
FOUNDATIONAL_PHASES = {
    'phase_pilot':                 "the driver's third hand — intent authored before FillSignal copies the channel (VISIT_0)",
    'phase_fill_signal':           'the clock+signal fill is unconditional (every frame)',
    'phase_advance_clock':         'the tempo follower is unconditional',
    'phase_motion_drivers':        'the music driver + fog stage — atmosphere foundational (K4)',
    'phase_stage_world':           'world seed + finite bounds — surface foundational (S2)',
    'phase_transition_machine':    'the transition machine is spine-owned (SEAM[spine:transitions])',
    'phase_stage_fade_and_upload': 'the O-5b/c DRAIN — the sole signal/config uploader',
    'phase_clear_input_deltas':    'driver bookkeeping, dead-last (O-5e)',
    'phase_witness_harvest':       'the witness harvest (P5) — spine-owned readback',
    'phase_stream_patches':        'S2 streaming conductor (SEAM[patch:spawn-trigger])',
    'phase_census_dumps':          'spine diagnostics (constitution §5; gol residue proof at G3)',
    'phase_entity_mesh_gen':       'the mesh-gen pass — per-family gating is intra-phase',
    'phase_upload_portal_lights':  'flag/count-guarded uploads (portals_dirty; light count=0 disables)',
    'phase_dispatch_compute':      'REALIZATION — the frame compute dispatch',
    'phase_witness_capture':       'the witness capture (O-2) — spine-owned readback',
    'phase_live_card_write':       'the live-card field write — spine-owned, rest-skipped by its own clean flag',
    'phase_ground_entries':        'flag-driven realization (ground_entries_dirty cascade, O-4)',
    'phase_placement_correction':  'flag-driven realization (placement_dirty, O-4)',
    'phase_frustum_cull':          'REALIZATION pass (piece-gating lives at gate a-prime)',
    'phase_shadow_pass':           'REALIZATION pass (piece-gating lives at gate a-prime)',
    'phase_main_pass':             'REALIZATION pass (piece-gating lives at gate a-prime)',
    'phase_snapshot_pass':         'REALIZATION pass; inert without a pending snapshot',
}

# ── DIRECTION A (non-frame): teardown / boot / mesh-prep / delegated ──
# These obligations are NOT frame rows — they live inside a phase body
# (teardown inside phase_transition_machine; mesh-prep inside
# phase_entity_mesh_gen) or at boot (initialize) or at a module door. They
# grep as gated call-sites, comment-tolerant. piece -> [(file, name, regex)].
C = r'(?:\s|//[^\n]*\n|/\*.*?\*/)*'
def imm(bit, call):
    return rf'if constexpr \({bit}\){C}{call}\('
def blk(bit, *calls):
    inner = r'[^}}]*?'.join(re.escape(c) + r'\(' for c in calls)
    return rf'if constexpr \({bit}\){C}(?:\{{|if \([^\n]*\{{)?{C}[^}}]*?{inner}'

GREP_MANIFEST = {
    'pyramid':  [('cartridge.hpp', 'mesh prep', imm(r'ROSTER\.pyramid', r'[^;]*?\.prepare_mesh'))],
    'arch':     [('cartridge.hpp', 'mesh prep', imm(r'ROSTER\.arch',    r'[^;]*?\.prepare_mesh'))],
    'column':   [('cartridge.hpp', 'mesh prep', imm(r'ROSTER\.column',  r'[^;]*?\.prepare_mesh'))],
    'antenna':  [('cartridge.hpp', 'mesh prep', imm(r'ROSTER\.antenna', r'[^;]*?\.prepare_mesh'))],
    'palm':     [('cartridge.hpp', 'mesh prep', imm(r'ROSTER\.palm',    r'[^;]*?\.prepare_mesh'))],
    'cactus':   [('cartridge.hpp', 'mesh prep', imm(r'ROSTER\.cactus',  r'[^;]*?\.prepare_mesh'))],
    'blade':    [('cartridge.hpp', 'mesh prep', imm(r'ROSTER\.blade',   r'[^;]*?\.prepare_mesh'))],
    'sphere':   [('cartridge.hpp', 'mirror',    imm(r'ROSTER\.sphere',  r'reconcile_sphere_mirror')),
                 ('cartridge.hpp', 'teardown',  imm(r'ROSTER\.sphere',  r'clear_spheres')),
                 ('cartridge.hpp', 'mesh prep', imm(r'ROSTER\.sphere',  r'[^;]*?\.prepare_mesh'))],
    'ribbon':   [('cartridge.hpp', 'teardown',  imm(r'ROSTER\.ribbon',  r'teardown_ribbon')),
                 ('cartridge.hpp', 'finite rel', imm(r'ROSTER\.ribbon', r'release_finite_ribbons')),
                 ('cartridge.hpp', 'mesh prep', imm(r'ROSTER\.ribbon',  r'[^;]*?\.prepare_mesh'))],
    'cube':     [('cartridge.hpp', 'mirror',    imm(r'ROSTER\.cube',    r'reconcile_cube_mirror')),
                 ('cartridge.hpp', 'teardown',  imm(r'ROSTER\.cube',    r'clear_cubes')),
                 ('cartridge.hpp', 'mesh prep', imm(r'ROSTER\.cube',    r'[^;]*?\.prepare_mesh'))],
    'gol':      [('cartridge.hpp', 'teardown',  imm(r'ROSTER\.gol',     r'teardown_gol')),
                 ('cartridge.hpp', 'mesh prep', imm(r'ROSTER\.gol',     r'[^;]*?\.prepare_mesh'))],
    # OVERTURE_0 U4a moved the authored fill's ONE home from initialize()'s
    # eager boot call to the conductor's deferred-hang head, so the gate that
    # kills it structurally moved with it. Same property, new site: with
    # gallery off the conductor never wakes the verb, the verb is the fill's
    # only caller, and the authored-staging textures stay pristine.
    'gallery':  [('surface/patch_system.hpp', 'deferred hang (P2 dead)', imm(r'ROSTER\.gallery', r'tick_gallery_deferred_hang')),
                 ('cartridge.hpp', 'teardown (shared organ)', imm(r'ROSTER\.gallery \|\| ROSTER\.indoor_shell', r'teardown_gallery')),
                 ('cartridge.hpp', 'mesh prep', imm(r'ROSTER\.gallery', r'[^;]*?\.prepare_mesh'))],
    'pawn_aura':[('cartridge.hpp', 'teardown',  imm(r'ROSTER\.pawn_aura', r'teardown_pawn_aura'))],
    'orbs':     [('direction/mood.hpp', 'boot config', blk(r'ROSTER\.orbs', 'configure_orbs')),
                 ('organ_boundary.inc', 'organ re-speak', imm(r'ROSTER\.orbs', r'configure_orbs')),
                 ('cartridge.hpp', 'teardown',   imm(r'ROSTER\.orbs',  r'teardown_orbs'))],
    # DELEGATED pieces: the gate lives at the module door (cited), not the spine.
    'spot_lights': [('direction/mood.hpp', 'apply door', rf'if constexpr \(ROSTER\.spot_lights\)')],
    'indoor_shell':[('direction/mood.hpp', 'apply door', rf'if constexpr \(ROSTER\.indoor_shell\)')],
    'portal':      [('bodies/grounded.hpp', 'force-spawn door', rf'if constexpr \(!ROSTER\.portal\)')],
    'transitions': [('direction/mood.hpp', 'request door #1', rf'if constexpr \(!ROSTER\.transitions\)')],
    'wanderers':   [('cartridge.hpp', 'boot population', imm(r'ROSTER\.wanderers', r'spawn_population_for_mood'))],
}

def strip_comments_strings(s):
    s = re.sub(r'/\*.*?\*/', ' ', s, flags=re.S)
    s = re.sub(r'//[^\n]*', ' ', s)
    s = re.sub(r'"(?:[^"\\]|\\.)*"', '""', s)
    return s

def main():
    failures = []

    # ── DIRECTION A: roster -> spine ──
    print('── DIRECTION A: roster -> spine (obligations present + gated) ──')
    row_by_fn = {r['fn']: r for r in ALL_ROWS}

    # A.1 per-family FRAME rows (the spine is the manifest)
    for fam, entries in FRAME_ROWS.items():
        for (fn, req) in entries:
            r = row_by_fn.get(fn)
            if r is None:
                failures.append(f'A: {fam} — {fn} MISSING from the spine')
                print(f'  {fam:13s} {fn:28s} [spine]  ** FAIL (no row) **')
            elif gate_families(r['gate']) != req:
                failures.append(f'A: {fam} — {fn} gate is `{r["gate"]}` (want {sorted(req)})')
                print(f'  {fam:13s} {fn:28s} [spine]  ** FAIL (gate {r["gate"]}) **')
            else:
                print(f'  {fam:13s} {fn:28s} [spine]  OK')
    for (fn, req) in SHARED_ROWS:
        r = row_by_fn.get(fn)
        if r is None or gate_families(r['gate']) != req:
            failures.append(f'A: shared row {fn} MISSING or mis-gated (want {sorted(req)})')
            print(f'  {"shared":13s} {fn:28s} [spine]  ** FAIL **')
        else:
            print(f'  {"shared":13s} {fn:28s} [spine]  OK')

    # A.2 non-frame obligations (teardown / boot / mesh-prep / delegated)
    for piece, entries in GREP_MANIFEST.items():
        for (rel, name, pat) in entries:
            text = CART if rel == 'cartridge.hpp' else read(rel)
            if re.search(pat, text, flags=re.S):
                print(f'  {piece:13s} {name:26s} [{rel}]  OK')
            else:
                failures.append(f'A: {piece} — {name} [{rel}] MISSING or UNGATED')
                print(f'  {piece:13s} {name:26s} [{rel}]  ** FAIL **')

    # ── DIRECTION B: spine -> roster (every row attributed) ──
    print('── DIRECTION B: spine -> roster (row attribution + bijection) ──')
    # B.1 phases <-> rows bijection
    row_fns = {r['fn'] for r in ALL_ROWS}
    for fn in sorted(PHASE_DEFS - row_fns):
        failures.append(f'B: phase {fn}() has no spine row (an unrowed phase is invisible to the frame)')
        print(f'  ** ORPHAN PHASE ** {fn}() — no row')
    for fn in sorted(row_fns - PHASE_DEFS):
        failures.append(f'B: spine row {fn} names no phase method')
        print(f'  ** DANGLING ROW ** {fn} — no phase def')
    # B.2 every row's gate is attributed
    for r in ALL_ROWS:
        fams = gate_families(r['gate'])
        if not fams:  # `true` -> must be foundational, justified
            if r['fn'] not in FOUNDATIONAL_PHASES:
                failures.append(f'B: row {r["fn"]} is ungated (`true`) but not a FOUNDATIONAL phase')
                print(f'  ** UNATTRIBUTED ** {r["fn"]} (true, no justification)')
        else:
            unknown = fams - KNOWN_FAMILIES
            if unknown:
                failures.append(f'B: row {r["fn"]} gate references unknown families {sorted(unknown)}')
                print(f'  ** UNKNOWN GATE ** {r["fn"]}: {sorted(unknown)}')
    if not failures:
        print(f'  {len(ALL_ROWS)} rows attributed ({len(PHASE_DEFS)} phases, bijection holds; '
              f'{len(FOUNDATIONAL_PHASES)} foundational)')

    # ── DIRECTION W (m5): the witness sole-author law ──
    print('── DIRECTION W: the witness sole-author law ──')
    GUARDS = [
        (r'readback_(?:x|z|portal_trigger)\s*=[^=]', {'cartridge.hpp', 'contracts/spine_state.hpp'},
         'readback trio: P5 harvest + teardown reset + portal consume (spine only; the record declares its own defaults)'),
        (r'player_\.possessed_slot\s*=[^=]', {'bodies/agents.hpp'},
         'possession: the agents door only (re-anchoring, v3 §9 Act III)'),
        (r'player_\.aura_presence\s*[+]?=[^=]', {'bodies/pawn.hpp'},
         'aura presence: P8, pawn-owned'),
    ]
    import glob as _glob
    module_files = [f for f in _glob.glob(os.path.join(TB, '**', '*.inl'), recursive=True)
                    + _glob.glob(os.path.join(TB, '**', '*.hpp'), recursive=True)]
    for pat, allowed, law in GUARDS:
        offenders = []
        for f in module_files:
            rel = os.path.relpath(f, TB).replace(os.sep, '/')
            if rel in allowed or rel == 'cartridge.hpp' and 'cartridge.hpp' in allowed:
                continue
            body = strip_comments_strings(open(f, encoding='utf-8').read())
            if re.search(pat, body):
                offenders.append(rel)
        if offenders:
            for o in offenders:
                failures.append(f'W: {o} writes a guarded witness field ({law})')
                print(f'  ** VIOLATION ** {o}: {law}')
        else:
            print(f'  {law}: HOLDS')

    # ── DIRECTION W (deps, DISSOLVE-1 d2): witness reaches deps const-encoded ──
    print('── DIRECTION W (deps): witness reaches deps const-encoded ──')
    W_DOORS = {'bodies/pawn.hpp', 'bodies/agents.hpp', 'direction/input.hpp'}
    dep_offenders = []
    for f in module_files:
        rel = os.path.relpath(f, TB).replace(os.sep, '/')
        if not rel.endswith('.hpp'):
            continue
        body = strip_comments_strings(open(f, encoding='utf-8').read())
        for m in re.finditer(r'PlayerState&\s+\w+_;', body):
            preceding = body[max(0, m.start() - 8):m.start()]
            if 'const' in preceding:
                continue
            if rel not in W_DOORS:
                dep_offenders.append(rel)
    if dep_offenders:
        for o in sorted(set(dep_offenders)):
            failures.append(f'W-deps: {o} exposes a writable PlayerState& outside a declared door')
            print(f'  ** VIOLATION ** {o}: writable witness in deps')
    else:
        print('  writable-witness deps confined to the three doors (pawn/agents/input): HOLDS')

    print('──')
    if failures:
        for f in failures:
            print('FAIL:', f)
        print(f'SCORE CENSUS: RED ({len(failures)} violation(s))')
        return 1
    print(f'SCORE CENSUS: GREEN — the spine holds ({len(UPDATE_ROWS)} update + '
          f'{len(RENDER_ROWS)} render rows), bijection both directions')
    return 0

if __name__ == '__main__':
    sys.exit(main())
