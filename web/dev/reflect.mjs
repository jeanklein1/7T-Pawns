#!/usr/bin/env node
// Dev-only WGSL reflector for the 7T web port (PORT_MAP §2b).
// Computes per-entry-point STATIC resource usage, transitive through helper
// functions, and emits the census table appended to PORT_MAP §2c.
//
//   node web/dev/reflect.mjs web/shaders/world.wgsl [--md]
//
// Dev tooling, not a runtime dep. Parsing is regex+brace based, which is
// sound for this codebase because world.wgsl has no pointer-typed function
// params and WGSL has no strings/macros; the one known blind spot is a local
// identifier shadowing a module-scope resource name (none observed).

import { readFileSync } from 'node:fs';

const path = process.argv[2] ?? 'web/shaders/world.wgsl';
const asMd = process.argv.includes('--md');
let src = readFileSync(path, 'utf8');

// -- strip comments (block first, then line) --------------------------------
src = src.replace(/\/\*[\s\S]*?\*\//g, ' ').replace(/\/\/[^\n]*/g, ' ');

// -- module-scope resource declarations -------------------------------------
// @group(G) @binding(B) var<addr?> name : type ;
const RES_RE = /@group\((\d+)\)\s*@binding\((\d+)\)\s*var\s*(?:<([^>]*)>)?\s*([A-Za-z_]\w*)\s*:\s*([^;]+);/g;
const resources = new Map(); // name -> {group, binding, kind, access, type}
function classify(addr, type) {
  const t = type.trim();
  if (addr) {
    const a = addr.replace(/\s/g, '');
    if (a.startsWith('storage')) {
      return { kind: 'storage', access: a.includes('read_write') ? 'read_write' : 'read' };
    }
    if (a === 'uniform') return { kind: 'uniform', access: '-' };
  }
  if (/^texture_storage/.test(t)) return { kind: 'storage_texture', access: '-' };
  if (/^texture_/.test(t)) return { kind: 'texture', access: '-' };
  if (/^sampler/.test(t)) return { kind: 'sampler', access: '-' };
  return { kind: 'other', access: '-' };
}
for (const m of src.matchAll(RES_RE)) {
  const [, group, binding, addr, name, type] = m;
  resources.set(name, { group: +group, binding: +binding, type: type.trim(), ...classify(addr, type) });
}

// -- function definitions with bodies (brace matching) ----------------------
const fns = new Map(); // name -> body text
const FN_RE = /fn\s+([A-Za-z_]\w*)\s*\(/g;
for (const m of src.matchAll(FN_RE)) {
  const name = m[1];
  let i = src.indexOf('{', m.index);
  // skip past the return-type arrow region to the first '{' after the header
  if (i === -1) continue;
  let depth = 0, start = i;
  for (; i < src.length; i++) {
    if (src[i] === '{') depth++;
    else if (src[i] === '}') { depth--; if (depth === 0) break; }
  }
  fns.set(name, src.slice(start, i + 1));
}

// -- entry points (attribute directly precedes fn in valid WGSL) ------------
const entries = []; // {name, stage, wg}
const EP_RE = /@(compute|vertex|fragment)\b((?:\s*@\w+(?:\([^)]*\))?)*)\s*fn\s+([A-Za-z_]\w*)/g;
for (const m of src.matchAll(EP_RE)) {
  const wg = /@workgroup_size\(([^)]*)\)/.exec(m[2] ?? '');
  entries.push({ name: m[3], stage: m[1], wg: wg ? wg[1].replace(/\s/g, '') : '' });
}

// -- per-fn direct references (resources + callees) -------------------------
const resNames = [...resources.keys()];
const fnNames = [...fns.keys()];
const wordRe = (w) => new RegExp(`\\b${w}\\b`);
const callRe = (w) => new RegExp(`\\b${w}\\s*\\(`);
const direct = new Map();
for (const [name, body] of fns) {
  direct.set(name, {
    res: resNames.filter((r) => wordRe(r).test(body)),
    calls: fnNames.filter((f) => f !== name && callRe(f).test(body)),
  });
}

// -- transitive closure per entry point -------------------------------------
function closure(entry) {
  const seenFns = new Set(), used = new Set();
  const stack = [entry];
  while (stack.length) {
    const f = stack.pop();
    if (seenFns.has(f) || !direct.has(f)) continue;
    seenFns.add(f);
    const d = direct.get(f);
    d.res.forEach((r) => used.add(r));
    d.calls.forEach((c) => stack.push(c));
  }
  return used;
}

// -- demo roster annotation (PORT_MAP §0) — presentation only ----------------
const CUT = new Set([
  'update_player_agent', 'update_other_agents', 'update_sphere', 'update_cube',
  'sphere_vs', 'monolith_vs', 'shadow_sphere_vs', 'shadow_monolith_vs',
]);
const UNLIFTED = new Set(['shell_vs', 'shadow_shell_vs']); // indoor stays unlifted

// -- report ------------------------------------------------------------------
const rows = [];
for (const e of entries) {
  const used = [...closure(e.name)].map((n) => ({ name: n, ...resources.get(n) }));
  const pick = (k) => used.filter((u) => u.kind === k).sort((a, b) => a.binding - b.binding);
  const sb = pick('storage'), ub = pick('uniform'), tx = pick('texture'),
        st = pick('storage_texture'), sm = pick('sampler');
  rows.push({
    name: e.name, stage: e.stage, wg: e.wg,
    sbCount: sb.length, sb: sb.map((u) => u.name).join(' '),
    ub: ub.length, tx: tx.length, st: st.length, sm: sm.length,
    flag: sb.length > 8 ? '⚠️ >8' : '',
    roster: CUT.has(e.name) ? 'CUT' : UNLIFTED.has(e.name) ? 'unlifted' : 'lifts',
  });
}
const order = { compute: 0, vertex: 1, fragment: 2 };
rows.sort((a, b) => order[a.stage] - order[b.stage] || b.sbCount - a.sbCount || a.name.localeCompare(b.name));

if (asMd) {
  console.log('| entry point | stage | storage bufs | uniforms | textures | storage tex | samplers | roster | flag |');
  console.log('|---|---|---|---|---|---|---|---|---|');
  for (const r of rows) {
    console.log(`| \`${r.name}\` | ${r.stage}${r.wg ? ` (${r.wg})` : ''} | **${r.sbCount}** | ${r.ub} | ${r.tx} | ${r.st} | ${r.sm} | ${r.roster} | ${r.flag} |`);
  }
} else {
  for (const r of rows) {
    console.log(`${r.stage.padEnd(8)} ${r.name.padEnd(28)} sb=${String(r.sbCount).padStart(2)} ub=${r.ub} tx=${r.tx} st=${r.st} sm=${r.sm} ${r.flag} ${r.roster}`);
    if (r.sbCount) console.log(`         ${''.padEnd(28)} [${r.sb}]`);
  }
}
console.error(`\n${entries.length} entry points, ${resources.size} bound resources, ${fns.size} functions.`);
const worst = rows.filter((r) => r.sbCount > 8);
console.error(worst.length
  ? `OVER 8 STORAGE: ${worst.map((r) => `${r.name}(${r.sbCount})`).join(', ')}`
  : 'All entry points ≤ 8 storage buffers per stage.');
