import { useState, useEffect, useRef, useCallback } from "react";

/* ═══ HASH — exact port from cartridge.hpp ═══ */
function cpuH(seed, prop) {
  let h = (Math.imul(seed >>> 0, 747796405) + Math.imul(prop >>> 0, 2891336453) + 1) >>> 0;
  h = Math.imul(((h >>> 16) ^ h) >>> 0, 2654435769) >>> 0;
  h = Math.imul(((h >>> 16) ^ h) >>> 0, 2654435769) >>> 0;
  return ((h >>> 16) ^ h) >>> 0;
}
function hf(s, p) { return cpuH(s, p) / 0xFFFFFFFF; }
function tseed(m, gx, gz) {
  let h = m >>> 0;
  h = (h ^ Math.imul((gx & 0xFFFFFFFF) >>> 0, 73856093)) >>> 0;
  h = (h ^ Math.imul((gz & 0xFFFFFFFF) >>> 0, 19349663)) >>> 0;
  h = Math.imul(((h >>> 16) ^ h) >>> 0, 2654435769) >>> 0;
  return (Math.imul(((h >>> 16) ^ h) >>> 0, 2654435769)) >>> 0;
}
function latticeNodeSeed(master, nx, nz, band) {
  let h = master >>> 0;
  h = (h ^ Math.imul((nx & 0xFFFFFFFF) >>> 0, 73856093)) >>> 0;
  h = (h ^ Math.imul((nz & 0xFFFFFFFF) >>> 0, 19349663)) >>> 0;
  h = (h ^ Math.imul(band >>> 0, 83492791)) >>> 0;
  h = Math.imul(((h >>> 16) ^ h) >>> 0, 2654435769) >>> 0;
  return (Math.imul(((h >>> 16) ^ h) >>> 0, 2654435769)) >>> 0;
}
function gauss(seed, prop, mu, sig) {
  const u1 = Math.max(hf(seed, prop), 1e-6);
  const u2 = hf(seed, prop + 1000);
  const z = Math.max(-3, Math.min(3, Math.sqrt(-2 * Math.log(u1)) * Math.cos(2 * Math.PI * u2)));
  return mu + z * sig;
}
function selW(seed, prop, wts) {
  const roll = hf(seed, prop); let tot = 0; for (const w of wts) tot += w;
  let c = 0; for (let i = 0; i < wts.length; i++) { c += wts[i] / tot; if (roll < c) return i; }
  return wts.length - 1;
}

/* ═══ CONSTANTS — mirrors real cartridge.hpp (7-family) ═══ */
const PE = 50;
const SPAWN_RADIUS = 7;
const RENDER_RADIUS = 5;
const EVICT_RADIUS = SPAWN_RADIUS + 2;
const NF = 9;
const MAX_FP = 128;
const BASE_WEIGHT = 10;

const FN = ["Pyramid", "Arch", "Column", "Antenna", "Palm", "Cactus", "Blade", "Floating", "Ribbon"];
const FK = ["p", "a", "c", "n", "palm", "cact", "blad", "float", "ribn"];
const FCOL = ["#d4713b", "#4a90c4", "#6aaa5c", "#b07acc", "#2d8a4e", "#8ab050", "#90c870", "#c0a0d0", "#e8a060"];

const TN = [
  ["Obelisk", "Temple", "Colossus"],
  ["Doorway", "Standard", "Monumental"],
  ["Pillar", "Doric", "Ornate"],
  ["Antenna", "Squat", "Colossal"],
  ["Sapling", "Coastal", "Royal"],
  ["Finger", "Saguaro", "Candelabra"],
  ["Sprout", "Clump", "Thicket"],
  ["SmCube", "MdCube", "LgCube", "Monolith", "Sentinel", "Anomaly"],
  ["Serpentine", "Helix", "Streamer"],
];
const TS = [
  [.35, .6, 1], [.1, .55, .95], [.15, .3, .5], [.6, .45, .85],
  [.2, .45, .75], [.15, .4, .7], [.3, .3, .3], [.15, .35, .60, .40, .25, .20],
  [.70, .40, .55],
];

const THN = ["Transition", "Monumental", "Colonnade", "Antenna", "Barren"];
const THC = ["rgba(180,180,170,.18)", "rgba(200,140,80,.22)", "rgba(100,160,200,.22)", "rgba(160,200,100,.22)", "rgba(220,210,190,.15)"];
const AC = ["#c49a6c", "#b8b090", "#a0b8a0", "#8ab0c8"];
const AN = ["Mount", "Varied", "Basin", "Pool"];

const SPAWN_PROP = [800, 600, 700, 900, 950, 1000, 1100, 100, 400];
const TIER_PROP  = [804, 604, 703, 903, 954, 1004, 1104, 103, 403];
const POS_X_PROP = [801, 601, 701, 901, 951, 1001, 1101, 101, 401];
const POS_Z_PROP = [802, 602, 702, 902, 952, 1002, 1102, 102, 402];
const ROT_PROP   = [803, 603, 355, 355, 953, 1003, 1103, 126, 405];
const JITTER     = [.25, .35, .35, .35, .45, .35, .30, .40, .35];
const MAX_SLOTS  = [8, 16, 16, 16, 24, 20, 32, 32, 1];

const MOOD_COUNT = 6;
const MOOD_NAMES = ["open_default", "open_sunset", "indoor_flat", "indoor_vault", "finite_outdoor", "finite_outdoor_ref"];
const MOOD_MULT = [
  [1,1,1,1,1,1,1,1,1], [1,1,1,1,1,1,1,1,1], [1,1,1,1,1,1,1,0,0],
  [1,1,1,1,1,1,1,0,0], [1,1,1,1,1,1,1,1,1], [0,1,0,0,0,0,0,0,0],
];

const THEME_LATTICE_SPACING = 500;
const THEME_SEED_BAND = 170;
const TK = ["tp", "ta", "tc", "tn", "tpalm", "tcact", "tblad", "tfloat", "tribn"];

function initState() {
  return {
    seed: 42,
    mood: 0,
    aw: [1.8, 1.3, 1, 0],
    sc: { p: .030, a: .030, c: .030, n: .025, palm: .200, cact: .100, blad: .025, float: .050, ribn: .100 },
    tw: {
      p: [.5, .25, .25], a: [.5, .15, .15], c: [.05, .2, .18], n: [.1, .22, .13],
      palm: [.5, .35, .15], cact: [.5, .35, .15], blad: [.5, .35, .15],
      float: [.35, .28, .18, .04, .02, .01], ribn: [.45, .30, .25],
    },
    sep: [
      [15, 10,  5,  5,  5,  5, 0, 0, 0],
      [10, 20, 10, 10,  8,  5, 0, 0, 0],
      [ 5, 10,  8,  6,  5,  5, 0, 0, 0],
      [ 5, 10,  6, 12,  5,  5, 0, 0, 0],
      [ 5,  8,  5,  5,  8,  5, 0, 0, 0],
      [ 5,  5,  5,  5,  5,  8, 0, 0, 0],
      [ 0,  0,  0,  0,  0,  0, 0, 0, 0],
      [ 0,  0,  0,  0,  0,  0, 0, 20, 0],
      [ 0,  0,  0,  0,  0,  0, 0, 0, 0],
    ],
    mute: new Array(NF).fill(false),
    solo: -1,
    density: { spacing: 250, seedBand: 160, exponent: 0.6, min: 1.0, max: 1.0 },
    prox: {
      radius:       [0, 0, 60, 0, 150, 120, 120, 0, 0],
      maxBoost:     [1, 1,  2, 1,   3,   3,   3, 1, 1],
      threshold:    [0, 0,  2, 0,   1,   1,   1, 0, 0],
      gapReduction: [0, 0, .3, 0,  .6,  .6,  .6, 0, 0],
      aff: [
        [0, 0, 0,  0, 0,  0,  0,  0, 0],
        [0, 0, 0,  0, 0,  0,  0,  0, 0],
        [0, 0, .4, 0, 0,  0,  0,  0, 0],
        [0, 0, 0,  0, 0,  0,  0,  0, 0],
        [0, 0, 0,  0, .5, .3, .3, 0, 0],
        [0, 0, 0,  0, .3, .5, .3, 0, 0],
        [0, 0, 0,  0, .3, .3, .5, 0, 0],
        [0, 0, 0,  0, 0,  0,  0,  0, 0],
        [0, 0, 0,  0, 0,  0,  0,  0, 0],
      ],
    },
    globalDensity: 1.0,
    batch: {
      size: 16, typeStr: 0.0, scaleStr: 0.0,
      affCh: 0.0, repCh: 0.0, minObs: 1,
      crossAff: [
        [0.5, 2.0, 1.5, 1.5, 1.0, 1.0, 1.0, 1.0, 1.0],
        [0.8, 1.5, 2.0, 2.0, 1.0, 1.0, 1.0, 1.0, 1.0],
        [0.3, 1.2, 1.8, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0],
        [0.3, 1.2, 1.0, 1.8, 1.0, 1.0, 1.0, 1.0, 1.0],
        [0.3, 1.0, 1.0, 1.0, 1.5, 1.0, 1.0, 1.0, 1.0],
        [1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 1.0, 1.0, 1.0],
        [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0],
        [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0],
        [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0],
      ],
    },
    th: [
      // 0: TRANSITION
      { sp: [.4, .3, .7, .3, .3, .3, .5, .3, .3],
        tp: [1, 1, 1], ta: [1, .3, 1], tc: [.1, .2, .3], tn: [.1, 2, .7],
        tpalm: [1, 1, 1], tcact: [1, 1, 1], tblad: [1, 1, 1], tfloat: [1, 1, 1, 1, 1, 1], tribn: [1, 1, 1],
        spike: 150, sustain: 20, decay: 3, cooldown: 0, weight: .21 },
      // 1: MONUMENTAL
      { sp: [1.5, 1, 1, .5, .2, .2, .5, .3, .3],
        tp: [.2, .5, 3], ta: [2, .1, 3], tc: [.01, .01, 1], tn: [.5, 1.5, .5],
        tpalm: [1, 1, 1], tcact: [1, 1, 1], tblad: [1, 1, 1], tfloat: [1, 1, 1, 1, 1, 1], tribn: [1, 1, 1],
        spike: 150, sustain: 10, decay: 10, cooldown: 8, weight: .30 },
      // 2: COLONNADE
      { sp: [.3, 1, 4, .5, .3, .3, .5, .3, .3],
        tp: [1, 1, 1], ta: [3, .5, 1], tc: [.3, 3, 5], tn: [.2, .1, .1],
        tpalm: [1, 1, 1], tcact: [1, 1, 1], tblad: [1, 1, 1], tfloat: [1, 1, 1, 1, 1, 1], tribn: [1, 1, 1],
        spike: 150, sustain: 15, decay: 6, cooldown: 6, weight: .31 },
      // 3: ANTENNA
      { sp: [.5, .5, 1, 4, .5, .5, .5, .3, .3],
        tp: [1, .05, 2], ta: [1, .2, .8], tc: [.1, .3, .3], tn: [.5, 3.5, 1],
        tpalm: [1, 1, 1], tcact: [1, 1, 1], tblad: [1, 1, 1], tfloat: [1, 1, 1, 1, 1, 1], tribn: [1, 1, 1],
        spike: 180, sustain: 10, decay: 5, cooldown: 5, weight: .18 },
      // 4: BARREN
      { sp: [.4, .3, .5, .3, .2, .2, .1, .3, .3],
        tp: [2, .5, .2], ta: [1, 1, 1], tc: [.2, .5, .5], tn: [1, 1, 1],
        tpalm: [1, 1, 1], tcact: [1, 1, 1], tblad: [1, 1, 1], tfloat: [1, 1, 1, 1, 1, 1], tribn: [1, 1, 1],
        spike: 100, sustain: 12, decay: 3, cooldown: 4, weight: .04 },
    ],
  };
}

/* ═══ SIMULATION CORE ═══ */

function createWorld(P) {
  return {
    patches: new Map(),
    tileCache: new Map(),
    env: { active: -1, elapsed: 0, cooldowns: P.th.map(() => 0) },
    footprints: [],
    slotCounts: new Array(NF).fill(0),
    batch: { tc: new Array(NF).fill(0), ss: 0, sn: 0, pe: 0, mode: 2 },
    batchCtr: 0,
    patchSeq: 0, entSeq: 0, initSeq: -1,
  };
}

function getTile(world, P, gx, gz) {
  const key = `${gx},${gz}`;
  if (world.tileCache.has(key)) return world.tileCache.get(key);

  // Archetype selection
  const sd = tseed(P.seed, gx, gz), roll = hf(sd, 300);
  const nc = [0, 0, 0, 0];
  for (let dz = -1; dz <= 1; dz++) for (let dx = -1; dx <= 1; dx++) {
    if (!dx && !dz) continue;
    const t = world.tileCache.get(`${gx + dx},${gz + dz}`);
    if (t) nc[t.ar]++;
  }
  const wts = P.aw.map((aw, i) => {
    let w = aw; const n = nc[i];
    if (n >= 4) w *= .2; else if (n >= 2) w *= 1.5; else if (n >= 1) w *= 2;
    return w;
  });
  let tot = 0; for (const w of wts) tot += w;
  let cum = 0, ar = 3;
  for (let a = 0; a < 4; a++) { cum += wts[a] / tot; if (roll < cum) { ar = a; break; } }

  // Entity density field — bilinear lattice blend
  const d = P.density;
  const pcx = (gx + 0.5) * PE, pcz = (gz + 0.5) * PE;
  let ed = 1.0;
  {
    const dlx = pcx / d.spacing, dlz = pcz / d.spacing;
    const dbx = Math.floor(dlx), dbz = Math.floor(dlz);
    const dfx = dlx - dbx, dfz = dlz - dbz;
    const dwx = dfx * dfx * (3 - 2 * dfx), dwz = dfz * dfz * (3 - 2 * dfz);
    let dens = 0;
    for (let dz2 = 0; dz2 <= 1; dz2++) for (let dx2 = 0; dx2 <= 1; dx2++) {
      const ns = latticeNodeSeed(P.seed, dbx + dx2, dbz + dz2, d.seedBand);
      const shaped = Math.pow(hf(ns, 350), d.exponent);
      const w = (dx2 ? dwx : 1 - dwx) * (dz2 ? dwz : 1 - dwz);
      dens += shaped * w;
    }
    ed = d.min + dens * (d.max - d.min);
  }

  // Theme lattice blend — spatial spawn multiplier
  const ts = new Array(NF).fill(0);
  let blD = 0, bestW = -1, domTh = 0;
  {
    const tlx = pcx / THEME_LATTICE_SPACING, tlz = pcz / THEME_LATTICE_SPACING;
    const tbx = Math.floor(tlx), tbz = Math.floor(tlz);
    const tfx = tlx - tbx, tfz = tlz - tbz;
    const twx = tfx * tfx * (3 - 2 * tfx), twz = tfz * tfz * (3 - 2 * tfz);
    for (let dz2 = 0; dz2 <= 1; dz2++) for (let dx2 = 0; dx2 <= 1; dx2++) {
      const ns = latticeNodeSeed(P.seed, tbx + dx2, tbz + dz2, THEME_SEED_BAND);
      const nodeRoll = hf(ns, 370);
      let nTot = 0; for (const t of P.th) nTot += (t.weight ?? 0.2);
      let nCum = 0, tidx = P.th.length - 1;
      for (let i = 0; i < P.th.length; i++) {
        nCum += (P.th[i].weight ?? 0.2) / nTot;
        if (nodeRoll < nCum) { tidx = i; break; }
      }
      const w = (dx2 ? twx : 1 - twx) * (dz2 ? twz : 1 - twz);
      for (let f = 0; f < NF; f++) ts[f] += P.th[tidx].sp[f] * w;
      blD += w;
      if (w > bestW) { bestW = w; domTh = tidx; }
    }
    ed *= blD;
  }

  const tile = { ar, ef: 0, ed, ts, ti: domTh };
  world.tileCache.set(key, tile);
  return tile;
}

/* ─── Single-pass spatial check (separation + overlap) ─── */

function checkPosition(world, P, px, pz, placingR, fam) {
  for (const fp of world.footprints) {
    const dx = px - fp.x, dz = pz - fp.z;
    let effMin = placingR + fp.r;
    if (fp.fam !== undefined && fp.fam < NF) {
      let minGap = P.sep[fam]?.[fp.fam] ?? 0;
      if (minGap > 0) {
        const aff = P.prox.aff[fam]?.[fp.fam] ?? 0;
        if (aff > 0) minGap *= (1 - aff * (P.prox.gapReduction[fam] ?? 0));
        effMin += minGap;
      }
    }
    if (dx * dx + dz * dz < effMin * effMin) return false;
  }
  return true;
}

function regFp(world, x, z, r, gx, gz, fam, tier) {
  if (world.footprints.length >= MAX_FP) return false;
  world.footprints.push({ x, z, r, gx, gz, fam, tier, time: world.patchSeq });
  return true;
}

/* ─── Proximity affinity (nearby entities boost spawn chance) ─── */

function proxBoost(world, P, cx, cz, fam) {
  const row = P.prox.aff[fam];
  if (!row || !row.some(v => v > 0)) return 1.0;
  const radius = P.prox.radius[fam] ?? 0;
  if (radius <= 0) return 1.0;
  const r2 = radius * radius;
  let weighted = 0, count = 0;
  for (const fp of world.footprints) {
    if (fp.fam === undefined || fp.fam >= NF) continue;
    const a = row[fp.fam];
    if (a <= 0) continue;
    const dx = cx - fp.x, dz = cz - fp.z;
    if (dx * dx + dz * dz < r2) { weighted += a; count++; }
  }
  if (count < (P.prox.threshold[fam] ?? 0)) return 1.0;
  return Math.min(1.0 + weighted, P.prox.maxBoost[fam] ?? 1);
}

function envWeight(th, el) {
  if (el < th.sustain) return th.spike;
  if (el < th.sustain + th.decay) {
    return th.spike + (BASE_WEIGHT - th.spike) * ((el - th.sustain) / th.decay);
  }
  return BASE_WEIGHT;
}

/* ─── Population batch system ─── */

function popTypeAffinity(world, P, family) {
  const b = world.batch;
  if (b.mode === 2) return 1.0; // NEUTRAL
  let total = 0; for (let f = 0; f < NF; f++) total += b.tc[f];
  if (total < P.batch.minObs) return 1.0;
  let influence = 0;
  for (let obs = 0; obs < NF; obs++) {
    const fraction = b.tc[obs] / total;
    let aff = (P.batch.crossAff[obs] || [])[family] ?? 1;
    if (b.mode === 1) aff = aff > 0.01 ? 1 / aff : 10; // REPULSION inverts
    influence += fraction * aff;
  }
  return 1.0 + (influence - 1.0) * P.batch.typeStr;
}

function popScaleTendency(world, P) {
  const b = world.batch;
  if (b.mode === 2 || b.sn < P.batch.minObs) return 0.5;
  let raw = b.ss / b.sn;
  if (b.mode === 1) raw = 1.0 - raw; // REPULSION inverts
  return raw;
}

function selectTierBiased(world, P, seed, prop, baseWts, count, family) {
  const b = world.batch;
  if (b.mode === 2 || b.sn < P.batch.minObs) return selW(seed, prop, baseWts);
  const tendency = popScaleTendency(world, P);
  const wts = [];
  for (let t = 0; t < count; t++) {
    const scale = (TS[family] || [])[t] || 0.5;
    const proximity = 1.0 - Math.abs(scale - tendency);
    wts.push(baseWts[t] * (1.0 + proximity * P.batch.scaleStr));
  }
  return selW(seed, prop, wts);
}

function advanceBatch(world, P) {
  world.batch.pe++;
  if (world.batch.pe >= P.batch.size) {
    world.batch = { tc: new Array(NF).fill(0), ss: 0, sn: 0, pe: 0, mode: 2 };
    world.batchCtr++;
    const ms = cpuH(P.seed ^ world.batchCtr, 330);
    const mr = hf(ms, 331);
    if (mr < P.batch.affCh) world.batch.mode = 0;       // AFFINITY
    else if (mr < P.batch.affCh + P.batch.repCh) world.batch.mode = 1; // REPULSION
    else world.batch.mode = 2;                            // NEUTRAL
  }
}

/* ─── Spawn pipeline (per patch) ─── */

function spawnPatch(world, P, gx, gz) {
  const key = `${gx},${gz}`;
  if (world.patches.has(key)) return;

  for (let dz = -1; dz <= 1; dz++) for (let dx = -1; dx <= 1; dx++)
    getTile(world, P, gx + dx, gz + dz);
  const tile = getTile(world, P, gx, gz);
  const sd = tseed(P.seed, gx, gz);

  // Theme envelope — selects tier bias
  let thIdx;
  if (P.solo >= 0) {
    thIdx = P.solo;
  } else {
    const env = world.env;
    const w = P.th.map((th, i) => {
      if (env.cooldowns[i] > 0) return 0;
      if (i === env.active) return envWeight(th, env.elapsed);
      return BASE_WEIGHT;
    });
    let wTot = 0; for (const v of w) wTot += v;
    if (wTot < .001) wTot = 1;
    const thRoll = hf(sd, 370);
    thIdx = P.th.length - 1; let thCum = 0;
    for (let i = 0; i < P.th.length; i++) {
      thCum += w[i] / wTot;
      if (thRoll < thCum) { thIdx = i; break; }
    }
    if (thIdx !== env.active) {
      if (env.active >= 0) env.cooldowns[env.active] = P.th[env.active].cooldown;
      env.active = thIdx; env.elapsed = 0;
    } else { env.elapsed++; }
    if (env.active >= 0) {
      const th = P.th[env.active];
      if (env.elapsed >= th.sustain + th.decay) {
        env.cooldowns[env.active] = th.cooldown;
        env.active = -1; env.elapsed = 0;
      }
    }
    for (let i = 0; i < env.cooldowns.length; i++)
      if (env.cooldowns[i] > 0) env.cooldowns[i]--;
  }

  const theme = P.th[thIdx];
  const patch = { gx, gz, ar: tile.ar, ti: thIdx, ents: [], seq: world.patchSeq++ };
  world.patches.set(key, patch);

  // Spawn gate per family
  for (let fam = 0; fam < NF; fam++) {
    if (P.mute[fam]) continue;
    if (world.slotCounts[fam] >= MAX_SLOTS[fam]) continue;

    // Spawn chance cascade
    let adjMod = (MOOD_MULT[P.mood]?.[fam] ?? 1) * tile.ed * tile.ts[fam];
    adjMod *= popTypeAffinity(world, P, fam);
    adjMod *= P.globalDensity;
    // Proximity affinity boost (nearby entities attract)
    {
      const pcx = (gx + 0.5) * PE, pcz = (gz + 0.5) * PE;
      adjMod *= proxBoost(world, P, pcx, pcz, fam);
    }
    const baseChance = P.sc[FK[fam]] ?? 0;
    const chance = Math.min(baseChance * adjMod, 1);
    if (hf(sd, SPAWN_PROP[fam]) >= chance) continue;

    // Tier selection (theme tier bias + batch scale tendency)
    const bw = [...(P.tw[FK[fam]] || [1, 1, 1])];
    const tb = theme[TK[fam]] || [1, 1, 1];
    for (let i = 0; i < bw.length; i++) bw[i] *= (tb[i] || 1);
    const tier = selectTierBiased(world, P, sd, TIER_PROP[fam], bw, bw.length, fam);

    // Position negotiation (jitter → separation → footprint)
    const cx = (gx + .5) * PE + (hf(sd, POS_X_PROP[fam]) - .5) * PE * JITTER[fam];
    const cz = (gz + .5) * PE + (hf(sd, POS_Z_PROP[fam]) - .5) * PE * JITTER[fam];
    const rot = hf(sd, ROT_PROP[fam]) * Math.PI * 2;
    const sc = (TS[fam] || [])[tier] || .5;
    const fpR = sc * 5;

    if (!checkPosition(world, P, cx, cz, fpR, fam)) continue;
    if (!regFp(world, cx, cz, fpR, gx, gz, fam, tier)) continue;

    // Commit
    patch.ents.push({
      x: cx, z: cz, rot, fam, ti: tier,
      tn: (TN[fam] || [])[tier] || "?", sc,
      thm: thIdx, seq: world.entSeq++
    });
    world.slotCounts[fam]++;
    world.batch.tc[fam]++;
    world.batch.ss += sc;
    world.batch.sn++;
    tile.ef |= (1 << fam);
  }
  advanceBatch(world, P);
}

/* ─── World update ─── */

function evictBeyond(world, cx, cz) {
  for (const [key, p] of [...world.patches]) {
    if (Math.abs(p.gx - cx) > EVICT_RADIUS || Math.abs(p.gz - cz) > EVICT_RADIUS) {
      for (const e of p.ents) world.slotCounts[e.fam] = Math.max(0, world.slotCounts[e.fam] - 1);
      world.footprints = world.footprints.filter(fp => fp.gx !== p.gx || fp.gz !== p.gz);
      world.patches.delete(key);
    }
  }
  for (const key of [...world.tileCache.keys()]) {
    const [gx2, gz2] = key.split(",").map(Number);
    if (Math.abs(gx2 - cx) > EVICT_RADIUS + 2 || Math.abs(gz2 - cz) > EVICT_RADIUS + 2)
      world.tileCache.delete(key);
  }
}

function updateWorld(world, P, px, pz) {
  evictBeyond(world, px, pz);
  const pending = [];
  for (let gz = pz - SPAWN_RADIUS; gz <= pz + SPAWN_RADIUS; gz++)
    for (let gx = px - SPAWN_RADIUS; gx <= px + SPAWN_RADIUS; gx++)
      if (!world.patches.has(`${gx},${gz}`))
        pending.push({ gx, gz, d2: (gx - px) ** 2 + (gz - pz) ** 2 });
  pending.sort((a, b) => a.d2 - b.d2);
  for (const p of pending) spawnPatch(world, P, p.gx, p.gz);
  if (world.initSeq < 0) world.initSeq = world.patchSeq;
}

function collectVisible(world, px, pz) {
  const tiles = [], ents = [];
  let minSeq = Infinity, maxSeq = 0;
  for (let gz = pz - SPAWN_RADIUS; gz <= pz + SPAWN_RADIUS; gz++)
    for (let gx = px - SPAWN_RADIUS; gx <= px + SPAWN_RADIUS; gx++) {
      const inR = Math.abs(gx - px) <= RENDER_RADIUS && Math.abs(gz - pz) <= RENDER_RADIUS;
      const p = world.patches.get(`${gx},${gz}`);
      if (p) {
        tiles.push({ ...p, pregen: !inR });
        if (p.seq < minSeq) minSeq = p.seq;
        if (p.seq > maxSeq) maxSeq = p.seq;
        for (const e of p.ents) ents.push({ ...e, pregen: !inR });
      } else {
        const t = world.tileCache.get(`${gx},${gz}`);
        tiles.push({ gx, gz, ar: t ? t.ar : 1, ti: -1, ents: [], seq: 0, pregen: !inR });
      }
    }
  return { tiles, ents, pawnGx: px, pawnGz: pz, minSeq, maxSeq, initSeq: world.initSeq };
}

/* ═══ UI COMPONENTS ═══ */

const ist = { padding: "2px 3px", fontSize: 11, fontFamily: "monospace", borderRadius: 4,
  border: "1px solid var(--color-border-tertiary)", background: "var(--color-background-primary)",
  color: "var(--color-text-primary)", textAlign: "right" };

function Num({ value, onChange, min = 0, max = 10, step = .01, w = 46 }) {
  const [txt, setTxt] = useState(String(Math.round(value * 1e4) / 1e4));
  const [fo, setFo] = useState(false);
  useEffect(() => { setTxt(String(Math.round(value * 1e4) / 1e4)); }, [value]);
  const commit = () => {
    const v = parseFloat(txt);
    if (!isNaN(v)) onChange(Math.max(min, Math.min(max, v)));
    else setTxt(String(Math.round(value * 1e4) / 1e4));
    setFo(false);
  };
  return (
    <span style={{ position: "relative", display: "inline-block" }}>
      <input type="text" value={txt} title={`${min} – ${max}`}
        onChange={e => setTxt(e.target.value)}
        onFocus={() => setFo(true)} onBlur={commit}
        onKeyDown={e => { if (e.key === "Enter") e.target.blur(); }}
        style={{ ...ist, width: w, outline: fo ? "1.5px solid var(--color-border-info)" : "none" }} />
      {fo && <span style={{ position: "absolute", left: 0, top: "100%", marginTop: 2,
        fontSize: 9, fontFamily: "monospace", color: "var(--color-text-tertiary)",
        whiteSpace: "nowrap", pointerEvents: "none", zIndex: 10,
        background: "var(--color-background-primary)", padding: "1px 3px",
        borderRadius: 3, border: "1px solid var(--color-border-tertiary)" }}>{min} – {max}</span>}
    </span>
  );
}

function SliderNum({ value, onChange, min = 0, max = 100, step = 1, sw = 70, nw = 44, label }) {
  return (<div style={{ display: "flex", alignItems: "center", gap: 4 }}>
    {label && <span style={{ fontSize: 10, color: "var(--color-text-secondary)", minWidth: 36 }}>{label}</span>}
    <input type="range" min={min} max={max} step={step} value={value} style={{ width: sw }}
      onChange={e => onChange(+e.target.value)} />
    <Num value={value} min={min} max={max} step={step} w={nw} onChange={onChange} />
  </div>);
}

function DragPanel({ title, children, ini = false, id, resetKey, onDock }) {
  const [open, setOpen] = useState(ini);
  const [pos, setPos] = useState(null);
  const [dragging, setDragging] = useState(false);
  const dragRef = useRef(null);
  const offsetRef = useRef({ x: 0, y: 0 });
  useEffect(() => { setPos(null); }, [resetKey]);
  const startDrag = useCallback(e => {
    if (e.target.closest("[data-notdrag]")) return;
    e.preventDefault();
    const rect = dragRef.current.getBoundingClientRect();
    offsetRef.current = { x: e.clientX - rect.left, y: e.clientY - rect.top };
    setDragging(true);
  }, []);
  useEffect(() => {
    if (!dragging) return;
    const onMove = e => {
      const pr = dragRef.current.parentElement.getBoundingClientRect();
      setPos({ x: e.clientX - pr.left - offsetRef.current.x, y: e.clientY - pr.top - offsetRef.current.y });
    };
    const onUp = () => setDragging(false);
    window.addEventListener("mousemove", onMove); window.addEventListener("mouseup", onUp);
    return () => { window.removeEventListener("mousemove", onMove); window.removeEventListener("mouseup", onUp); };
  }, [dragging]);
  const style = pos ? { position: "absolute", left: pos.x, top: pos.y, zIndex: 100, maxWidth: 520, minWidth: 280 } : {};
  return (
    <div ref={dragRef} style={{ marginBottom: pos ? 0 : 5,
      border: pos ? "1px solid rgba(0,0,0,.2)" : "1px solid var(--color-border-tertiary)",
      borderRadius: 8, overflow: "hidden", background: "var(--color-background-primary)",
      boxShadow: pos ? "0 4px 20px rgba(0,0,0,.3)" : "none", ...style }}>
      <div onMouseDown={startDrag} style={{ padding: "5px 10px", fontSize: 12, fontWeight: 500,
        userSelect: "none", display: "flex", alignItems: "center", gap: 5,
        color: "var(--color-text-primary)", background: "var(--color-background-secondary)", cursor: "grab" }}>
        <span data-notdrag="1" onClick={() => setOpen(!open)} style={{ cursor: "pointer", fontSize: 9,
          transition: "transform .15s", display: "inline-block",
          transform: open ? "rotate(90deg)" : "none", padding: "4px 2px" }}>&#9654;</span>
        <span style={{ flex: 1 }}>{title}</span>
        {pos && <span data-notdrag="1" onClick={() => { setPos(null); if (onDock) onDock(id); }}
          style={{ fontSize: 9, cursor: "pointer", opacity: .5, padding: "2px 4px" }}>dock</span>}
      </div>
      {open && <div style={{ padding: "5px 10px", maxHeight: pos ? 400 : "none",
        overflowY: pos ? "auto" : "visible" }}>{children}</div>}
    </div>
  );
}

const rw = { display: "flex", flexWrap: "wrap", gap: "3px 8px", alignItems: "center", marginBottom: 3 };
const lb = { fontSize: 10, color: "var(--color-text-secondary)", minWidth: 36 };
const thd = { padding: "1px 3px", fontWeight: 500, color: "var(--color-text-secondary)", textAlign: "right",
  borderBottom: "1px solid var(--color-border-tertiary)", fontSize: 9 };
const tdd = { padding: "1px 2px", textAlign: "right", borderBottom: "1px solid var(--color-border-tertiary)" };
const tdl = { padding: "1px 4px", color: "var(--color-text-secondary)", fontSize: 10,
  borderBottom: "1px solid var(--color-border-tertiary)" };

/* ═══ STORAGE ═══ */
const store = {
  async get(k) {
    try { if (window.storage) { const r = await window.storage.get(k); return r?.value ?? null; } } catch {}
    try { return localStorage.getItem(k); } catch {}
    return null;
  },
  async set(k, v) {
    try { if (window.storage) await window.storage.set(k, v); } catch {}
    try { localStorage.setItem(k, v); } catch {}
  },
};
const STORAGE_KEY = "7t:theme:v8";
const DEFAULTS_JSON = JSON.stringify(initState());
// Simple hash of defaults — when code changes defaults, saved state is discarded
const DEFAULTS_HASH = (() => { let h = 0; for (let i = 0; i < DEFAULTS_JSON.length; i++) h = (Math.imul(31, h) + DEFAULTS_JSON.charCodeAt(i)) | 0; return h; })();

/* ═══ MAIN APP ═══ */

export default function App() {
  const [P, setP] = useState(initState);
  const [loaded, setLoaded] = useState(false);
  const isModified = JSON.stringify(P) !== DEFAULTS_JSON;
  const [vis, setVis] = useState({ th: true, gr: true, age: true, clean: false, nums: false });
  const [view, setView] = useState(null);
  const [hov, setHov] = useState(null);
  const [selTh, setSelTh] = useState(0);
  const [dockKey, setDockKey] = useState(0);
  const [pawnPos, setPawnPos] = useState([0, 0]);
  const [showDump, setShowDump] = useState(false);
  const cvRef = useRef(null);
  const worldRef = useRef(null);
  const upd = fn => setP(prev => { const n = JSON.parse(JSON.stringify(prev)); fn(n); return n; });
  const defaultOrder = ["theme", "arch", "spawn", "tier", "sep", "prox", "density", "batch"];
  const [panelOrder, setPanelOrder] = useState(defaultOrder);

  // Load / save — discard saved state if code defaults have changed
  useEffect(() => { (async () => { try { const raw = await store.get(STORAGE_KEY);
    if (raw) { const d = JSON.parse(raw);
      if (d?.dh === DEFAULTS_HASH && d?.state?.th) setP(prev => ({ ...prev, ...d.state }));
    } } catch {} setLoaded(true); })(); }, []);
  useEffect(() => { if (!loaded) return; (async () => { try {
    await store.set(STORAGE_KEY, JSON.stringify({ dh: DEFAULTS_HASH, state: P }));
  } catch {} })(); }, [P, loaded]);
  // World rebuild on param change
  useEffect(() => { worldRef.current = createWorld(P); updateWorld(worldRef.current, P, pawnPos[0], pawnPos[1]);
    setView(collectVisible(worldRef.current, pawnPos[0], pawnPos[1])); }, [P]); // eslint-disable-line
  // Incremental on pawn move
  useEffect(() => { if (!worldRef.current) return; updateWorld(worldRef.current, P, pawnPos[0], pawnPos[1]);
    setView(collectVisible(worldRef.current, pawnPos[0], pawnPos[1])); }, [pawnPos]); // eslint-disable-line
  // Arrow keys
  useEffect(() => { const onKey = e => { if (["INPUT","SELECT","TEXTAREA"].includes(e.target.tagName)) return;
    const map = { ArrowUp: [0,-1], ArrowDown: [0,1], ArrowLeft: [-1,0], ArrowRight: [1,0] };
    const d = map[e.key]; if (d) { e.preventDefault(); setPawnPos(p => [p[0]+d[0], p[1]+d[1]]); } };
    window.addEventListener("keydown", onKey); return () => window.removeEventListener("keydown", onKey); }, []);

  /* ═══ CANVAS RENDERING ═══ */
  useEffect(() => {
    if (!view || !cvRef.current) return;
    const cv = cvRef.current, ctx = cv.getContext("2d");
    const dpr = window.devicePixelRatio || 1;
    const W = cv.clientWidth, H = cv.clientHeight;
    cv.width = W * dpr; cv.height = H * dpr; ctx.scale(dpr, dpr);
    const viewSide = (SPAWN_RADIUS * 2 + 1) * PE, mg = 8;
    const sc = Math.min((W - mg * 2) / viewSide, (H - mg * 2) / viewSide);
    const cenX = (view.pawnGx + .5) * PE, cenZ = (view.pawnGz + .5) * PE;
    const ox = W / 2 - cenX * sc, oz = H / 2 - cenZ * sc;
    const tp = (x, z) => [ox + x * sc, oz + z * sc];

    ctx.fillStyle = getComputedStyle(document.documentElement).getPropertyValue("--color-background-primary") || "#fff";
    ctx.fillRect(0, 0, W, H);
    const pw = PE * sc;
    const seqRange = view.maxSeq - view.minSeq;

    // Tiles
    for (const t of view.tiles) {
      const [sx, sz] = tp(t.gx * PE, t.gz * PE);
      if (vis.clean) {
        ctx.fillStyle = "#d5cfc5"; ctx.fillRect(sx, sz, pw, pw);
      } else {
        ctx.fillStyle = AC[t.ar]; ctx.fillRect(sx, sz, pw, pw);
        if (vis.th && t.ti >= 0) { ctx.fillStyle = THC[t.ti]; ctx.fillRect(sx, sz, pw, pw); }
      }
      if (vis.age) {
        const fr = seqRange > 0 ? (t.seq - view.minSeq) / seqRange : 0.5;
        const g = Math.round(fr * 255);
        ctx.fillStyle = `rgb(${g},${g},${g})`; ctx.fillRect(sx, sz, pw, pw);
        if (t.pregen) { ctx.fillStyle = "rgba(0,0,0,0.4)"; ctx.fillRect(sx, sz, pw, pw); }
        if (vis.nums) {
          const tg = fr > .5 ? 0 : 255;
          ctx.fillStyle = `rgba(${tg},${tg},${tg},0.6)`;
          ctx.font = `bold ${Math.max(7, pw * 0.2)}px monospace`;
          ctx.textAlign = "center"; ctx.textBaseline = "middle";
          ctx.fillText(view.maxSeq - t.seq + 1, sx + pw / 2, sz + pw / 2);
        }
      } else if (t.pregen) {
        ctx.fillStyle = "rgba(0,0,0,0.45)"; ctx.fillRect(sx, sz, pw, pw);
      }
      if (vis.gr) {
        ctx.strokeStyle = t.pregen ? "rgba(0,0,0,.12)" : "rgba(0,0,0,.07)";
        ctx.lineWidth = .5; ctx.strokeRect(sx, sz, pw, pw);
      }
    }

    // Render radius boundary
    {
      const [bx0, bz0] = tp((view.pawnGx - RENDER_RADIUS) * PE, (view.pawnGz - RENDER_RADIUS) * PE);
      const [bx1, bz1] = tp((view.pawnGx + RENDER_RADIUS + 1) * PE, (view.pawnGz + RENDER_RADIUS + 1) * PE);
      ctx.strokeStyle = "rgba(255,255,255,0.5)"; ctx.lineWidth = 2; ctx.setLineDash([]);
      ctx.strokeRect(bx0, bz0, bx1 - bx0, bz1 - bz0);
    }

    // Entities
    for (const e of view.ents) {
      const [ex, ez] = tp(e.x, e.z);
      const sz2 = Math.max((3 + e.sc * 7) * sc * 3, 1.5);
      ctx.save(); ctx.translate(ex, ez); ctx.rotate(-e.rot);
      if (e.pregen) ctx.globalAlpha = .25;
      else if (vis.age) {
        const fr = seqRange > 0 ? (e.seq - view.minSeq) / seqRange : 0.5;
        ctx.globalAlpha = .2 + fr * .8;
      }
      const cc = FCOL[e.fam];
      if (e.fam === 0) { // Pyramid — triangle
        ctx.fillStyle = cc; ctx.beginPath();
        ctx.moveTo(0, -sz2); ctx.lineTo(-sz2 * .7, sz2 * .6); ctx.lineTo(sz2 * .7, sz2 * .6);
        ctx.closePath(); ctx.fill();
      } else if (e.fam === 1) { // Arch — arc
        ctx.strokeStyle = cc; ctx.lineWidth = Math.max(1.5, sz2 * .25); ctx.lineCap = "round";
        ctx.beginPath(); ctx.arc(0, 0, sz2 * .8, Math.PI, 0); ctx.stroke();
      } else if (e.fam <= 3) { // Column/Antenna — circle + line
        ctx.fillStyle = cc; ctx.beginPath(); ctx.arc(0, 0, sz2 * .45, 0, Math.PI * 2); ctx.fill();
        ctx.strokeStyle = cc; ctx.lineWidth = 1;
        ctx.beginPath(); ctx.moveTo(0, 0); ctx.lineTo(0, -sz2 * .6); ctx.stroke();
      } else if (e.fam === 4) { // Palm — trunk + fronds
        ctx.strokeStyle = cc; ctx.lineWidth = Math.max(1, sz2 * .2);
        ctx.beginPath(); ctx.moveTo(0, sz2 * .5); ctx.lineTo(0, -sz2 * .3);
        ctx.moveTo(-sz2 * .4, -sz2 * .1); ctx.quadraticCurveTo(-sz2 * .2, -sz2 * .6, 0, -sz2 * .3);
        ctx.moveTo(sz2 * .4, -sz2 * .1); ctx.quadraticCurveTo(sz2 * .2, -sz2 * .6, 0, -sz2 * .3);
        ctx.stroke();
      } else if (e.fam === 5) { // Cactus — star
        ctx.fillStyle = cc; const r2 = sz2 * .3;
        ctx.beginPath();
        for (let i = 0; i < 5; i++) {
          const a = i * Math.PI * 2 / 5 - Math.PI / 2;
          ctx.lineTo(Math.cos(a) * r2, Math.sin(a) * r2);
          const b = a + Math.PI / 5;
          ctx.lineTo(Math.cos(b) * r2 * .4, Math.sin(b) * r2 * .4);
        }
        ctx.closePath(); ctx.fill();
      } else if (e.fam === 6) { // Blade — small dot
        ctx.fillStyle = cc; ctx.globalAlpha *= .7;
        ctx.beginPath(); ctx.arc(0, 0, sz2 * .3, 0, Math.PI * 2); ctx.fill();
      } else if (e.fam === 7) { // Floating — diamond
        ctx.fillStyle = cc; ctx.beginPath();
        ctx.moveTo(0, -sz2 * .5); ctx.lineTo(sz2 * .5, 0);
        ctx.lineTo(0, sz2 * .5); ctx.lineTo(-sz2 * .5, 0);
        ctx.closePath(); ctx.fill();
      } else { // Ribbon — wavy line
        ctx.strokeStyle = cc; ctx.lineWidth = Math.max(1.5, sz2 * .2); ctx.lineCap = "round";
        ctx.beginPath(); ctx.moveTo(-sz2 * .6, 0);
        ctx.quadraticCurveTo(-sz2 * .2, -sz2 * .4, 0, 0);
        ctx.quadraticCurveTo(sz2 * .2, sz2 * .4, sz2 * .6, 0);
        ctx.stroke();
      }
      ctx.restore();
    }

    // Pawn
    {
      const [px2, pz2] = tp((pawnPos[0] + .5) * PE, (pawnPos[1] + .5) * PE);
      ctx.fillStyle = "#e03030"; ctx.beginPath(); ctx.arc(px2, pz2, 5, 0, Math.PI * 2); ctx.fill();
      ctx.strokeStyle = "#fff"; ctx.lineWidth = 1.5; ctx.beginPath(); ctx.arc(px2, pz2, 5, 0, Math.PI * 2); ctx.stroke();
      ctx.strokeStyle = "#e03030"; ctx.lineWidth = 1;
      ctx.beginPath(); ctx.moveTo(px2 - 8, pz2); ctx.lineTo(px2 + 8, pz2);
      ctx.moveTo(px2, pz2 - 8); ctx.lineTo(px2, pz2 + 8); ctx.stroke();
    }
  }, [view, vis, pawnPos]);

  // Hover
  const onMove = useCallback(e => {
    if (!view || !cvRef.current) return;
    const rect = cvRef.current.getBoundingClientRect();
    const W = rect.width, H = rect.height;
    const vs = (SPAWN_RADIUS * 2 + 1) * PE;
    const sc2 = Math.min((W - 16) / vs, (H - 16) / vs);
    const ox2 = W / 2 - (view.pawnGx + .5) * PE * sc2;
    const oz2 = H / 2 - (view.pawnGz + .5) * PE * sc2;
    const wx = (e.clientX - rect.left - ox2) / sc2;
    const wz = (e.clientY - rect.top - oz2) / sc2;
    let best = null, bd = 300;
    for (const en of view.ents) {
      const d2 = (en.x - wx) ** 2 + (en.z - wz) ** 2;
      if (d2 < bd) { bd = d2; best = en; }
    }
    setHov(best);
  }, [view]);

  const ct = P.th[selTh];
  const stats = view ? {
    t: view.ents.length, patches: view.tiles.length,
    counts: FN.map((_, fi) => view.ents.filter(e => e.fam === fi).length),
  } : null;

  if (!loaded) return <div style={{ padding: 20, fontFamily: "monospace", fontSize: 11,
    color: "var(--color-text-tertiary)" }}>Loading…</div>;

  return (
    <div style={{ fontFamily: "var(--font-sans)", color: "var(--color-text-primary)", lineHeight: 1.4, position: "relative" }}>
      {/* ─── Toolbar ─── */}
      <div style={rw}>
        <SliderNum label="Seed" value={P.seed} min={0} max={99999} step={1} sw={60} nw={54}
          onChange={v => upd(n => { n.seed = v | 0; })} />
        <select value={P.mood} onChange={e => upd(n => { n.mood = +e.target.value; })}
          style={{ fontSize: 10, padding: "2px 4px", borderRadius: 4,
            border: "1px solid var(--color-border-tertiary)",
            background: "var(--color-background-primary)", color: "var(--color-text-primary)" }}>
          {MOOD_NAMES.map((mn, i) => <option key={i} value={i}>{mn}</option>)}
        </select>
        {[["th","Themes"],["gr","Grid"],["age","Age"],["clean","Clean"],["nums","#"]].map(([k, l]) =>
          <label key={k} style={{ fontSize: 10, color: "var(--color-text-secondary)", cursor: "pointer", marginLeft: 4 }}>
            <input type="checkbox" checked={vis[k]} onChange={e => setVis(prev => ({ ...prev, [k]: e.target.checked }))} /> {l}
          </label>)}
        <span style={{ fontSize: 9, color: "var(--color-text-tertiary)", marginLeft: 8 }}>
          Arrows ({pawnPos[0]},{pawnPos[1]})
        </span>
        <button onClick={() => { setDockKey(k => k + 1); setPanelOrder(defaultOrder); }}
          style={{ fontSize: 10, padding: "2px 8px", marginLeft: 8, borderRadius: 4, cursor: "pointer",
            border: "1px solid var(--color-border-tertiary)", background: "var(--color-background-secondary)",
            color: "var(--color-text-secondary)" }}>Dock</button>
        <button onClick={() => { setP(initState()); setSelTh(0); store.set(STORAGE_KEY, JSON.stringify(initState())); }}
          style={{ fontSize: 10, padding: "2px 8px", marginLeft: 4, borderRadius: 4, cursor: "pointer",
            border: "1px solid var(--color-border-tertiary)", background: "var(--color-background-secondary)",
            color: "var(--color-text-secondary)" }}>Reset</button>
        {isModified && <span style={{ fontSize: 9, color: "var(--color-text-tertiary)", marginLeft: 4 }}>● modified</span>}
        <button onClick={() => setShowDump(d => !d)}
          style={{ fontSize: 10, padding: "2px 8px", marginLeft: 4, borderRadius: 4, cursor: "pointer",
            border: "1px solid " + (showDump ? "var(--color-border-info)" : "var(--color-border-success)"),
            background: showDump ? "var(--color-background-info)" : "transparent",
            color: showDump ? "var(--color-text-info)" : "var(--color-text-success)" }}>
          {showDump ? "Hide" : "Save"}
        </button>
      </div>

      {/* ─── Dump ─── */}
      {showDump && (() => {
        const r = (v, d = 4) => +(+v).toFixed(d);
        const a = arr => "[" + arr.map(v => r(v)).join(", ") + "]";
        let s = "=== 7T THEME TOOL STATE (v3 — 7 families) ===\n";
        s += "seed: " + P.seed + "  mood: " + MOOD_NAMES[P.mood] + "  solo: " + (P.solo >= 0 ? THN[P.solo] : "off") + "\n\n";
        s += "archetype weights: " + a(P.aw) + "\n";
        s += "slots: " + FN.map((fn, fi) => fn.slice(0, 3) + ":" + MAX_SLOTS[fi]).join(" ") + "\n\n";
        s += "base spawn chances:\n";
        FK.forEach((fk, fi) => { s += "  " + FN[fi].padEnd(10) + r(P.sc[fk]) + "\n"; });
        s += "\nbase tier weights:\n";
        FK.forEach((fk, fi) => { s += "  " + FN[fi].padEnd(10) + a(P.tw[fk]) + "\n"; });
        s += "\nseparation matrix (" + NF + "×" + NF + "):\n";
        s += "          " + FN.map(n => n.slice(0, 4).padStart(5)).join("") + "\n";
        P.sep.forEach((row, ri) => {
          s += "  " + FN[ri].padEnd(8) + row.map(v => String(r(v, 0)).padStart(5)).join("") + "\n";
        });
        s += "\nmute: " + FN.map((fn, fi) => fn[0] + ":" + P.mute[fi]).join(" ") + "\n";
        s += "\nproximity affinity:\n";
        s += "          " + ["radius","max","thresh","gap×"].map(h => h.padStart(7)).join("") + "\n";
        FN.forEach((fn, fi) => {
          s += "  " + fn.padEnd(8)
            + String(r(P.prox.radius[fi], 0)).padStart(7)
            + String(r(P.prox.maxBoost[fi])).padStart(7)
            + String(P.prox.threshold[fi]).padStart(7)
            + String(r(P.prox.gapReduction[fi])).padStart(7) + "\n";
        });
        s += "  affinity matrix:\n";
        s += "          " + FN.map(n => n.slice(0, 4).padStart(5)).join("") + "\n";
        P.prox.aff.forEach((row, ri) => {
          s += "  " + FN[ri].padEnd(8) + row.map(v => r(v, 1).toString().padStart(5)).join("") + "\n";
        });
        s += "\ndensity: global:" + r(P.globalDensity) + " spacing:" + r(P.density.spacing, 0) + " min:" + r(P.density.min) + " max:" + r(P.density.max) + " exp:" + r(P.density.exponent) + "\n";
        s += "\nbatch: size:" + P.batch.size + " typeStr:" + r(P.batch.typeStr) + " scaleStr:" + r(P.batch.scaleStr) + " minObs:" + P.batch.minObs + " affCh:" + r(P.batch.affCh) + " repCh:" + r(P.batch.repCh) + "\n";
        s += "  cross-affinity:\n";
        s += "          " + FN.map(n => n.slice(0, 4).padStart(5)).join("") + "\n";
        (P.batch.crossAff || []).forEach((row, ri) => {
          s += "  " + FN[ri].padEnd(8) + (row || []).map(v => r(v, 1).toString().padStart(5)).join("") + "\n";
        });
        s += "\n";
        P.th.forEach((t, i) => {
          s += "--- Theme " + i + ": " + THN[i] + " ---\n";
          s += "  envelope: spike:" + r(t.spike, 0) + " sustain:" + t.sustain + " decay:" + t.decay
            + " cooldown:" + t.cooldown + " weight:" + r(t.weight ?? 0.2) + "\n";
          s += "  spawn: " + a(t.sp) + "\n";
          TK.forEach((tk, fi) => {
            s += "  tier_" + FN[fi].toLowerCase().padEnd(8) + a(t[tk] || [1, 1, 1]) + "\n";
          });
          s += "\n";
        });
        const taId = "save-dump-ta";
        return (<div style={{ position: "relative", marginBottom: 6 }}>
          <textarea id={taId} readOnly value={s} onFocus={e => e.target.select()}
            style={{ width: "100%", height: 200, fontSize: 10, fontFamily: "monospace", padding: 6,
              borderRadius: 6, border: "1px solid var(--color-border-tertiary)",
              background: "var(--color-background-secondary)", color: "var(--color-text-primary)", resize: "vertical" }} />
          <button onClick={ev => {
            const ta = document.getElementById(taId);
            if (ta) { ta.select(); ta.setSelectionRange(0, ta.value.length); }
            try { document.execCommand("copy"); ev.target.textContent = "Copied!";
              setTimeout(() => { ev.target.textContent = "Copy"; }, 1200); } catch {}
          }} style={{ position: "absolute", top: 4, right: 4, fontSize: 9, padding: "2px 8px",
            borderRadius: 4, cursor: "pointer", border: "1px solid var(--color-border-success)",
            background: "var(--color-background-primary)", color: "var(--color-text-success)" }}>Copy</button>
        </div>);
      })()}

      {/* ─── Canvas ─── */}
      <div style={{ position: "relative", border: "1px solid var(--color-border-tertiary)", borderRadius: 8,
        overflow: "hidden", background: "var(--color-background-primary)", marginBottom: 6 }}>
        <canvas ref={cvRef} onMouseMove={onMove}
          style={{ width: "100%", height: 460, display: "block", cursor: "crosshair" }} />

        {hov && <div style={{ position: "absolute", top: 4, right: 4,
          background: "var(--color-background-primary)", border: "1px solid var(--color-border-tertiary)",
          borderRadius: 6, padding: "4px 8px", fontSize: 10, lineHeight: 1.5, pointerEvents: "none" }}>
          <span style={{ fontWeight: 500, color: FCOL[hov.fam] }}>{FN[hov.fam]}</span> — {hov.tn} ({hov.sc.toFixed(2)})
          <br />{THN[hov.thm]} | {Math.round(hov.rot * 180 / Math.PI)}°
        </div>}

        {worldRef.current && (() => {
          const env = worldRef.current.env, active = env.active;
          const th2 = active >= 0 ? P.th[active] : null;
          let phase = "idle", progress = 0, total = 1;
          if (th2) {
            if (env.elapsed < th2.sustain) { phase = "sustain"; progress = env.elapsed; total = th2.sustain; }
            else if (env.elapsed < th2.sustain + th2.decay) { phase = "decay"; progress = env.elapsed - th2.sustain; total = th2.decay; }
            else phase = "expired";
          }
          const fill = total > 0 ? Math.min(1, progress / total) : 0;
          const coolStrs = env.cooldowns.map((c, i) => c > 0 ? THN[i][0] + c : null).filter(Boolean);
          const slotStr = FN.map((fn, fi) => fn.slice(0, 3) + ":" + worldRef.current.slotCounts[fi] + "/" + MAX_SLOTS[fi]).join(" ");
          return (
            <div style={{ position: "absolute", bottom: 4, left: 4, background: "rgba(0,0,0,0.55)",
              borderRadius: 5, padding: "3px 8px", fontSize: 9, lineHeight: 1.5, pointerEvents: "none",
              color: "#ccc", display: "flex", alignItems: "center", gap: 6, flexWrap: "wrap" }}>
              <span style={{ fontWeight: 600, color: active >= 0 ? "#e8e0d0" : "#888" }}>
                {active >= 0 ? THN[active] : "—"}</span>
              {th2 && <span style={{ color: phase === "sustain" ? "#8c8" : phase === "decay" ? "#ca8" : "#888" }}>{phase}</span>}
              {th2 && <span style={{ display: "inline-block", width: 80, height: 4,
                background: "rgba(255,255,255,0.12)", borderRadius: 2, overflow: "hidden", verticalAlign: "middle" }}>
                <span style={{ display: "block", width: fill * 80, height: 4,
                  background: phase === "sustain" ? "#6a6" : "#b85", borderRadius: 2 }} />
              </span>}
              {coolStrs.length > 0 && <span style={{ color: "#866", fontSize: 8 }}>cool: {coolStrs.join(" ")}</span>}
              <span style={{ color: "#9a9", fontSize: 8 }}>{slotStr}</span>
            </div>
          );
        })()}
      </div>

      {/* ─── Stats bar ─── */}
      {stats && <div style={{ fontSize: 10, color: "var(--color-text-secondary)", marginBottom: 6,
        display: "flex", gap: 6, flexWrap: "wrap", alignItems: "center" }}>
        <span>{stats.t} ents / {stats.patches} patches ({(stats.t / Math.max(stats.patches, 1)).toFixed(2)}/p)</span>
        {FN.map((name, fi) => {
          const muted = P.mute[fi];
          return (<button key={fi} onClick={() => upd(n => { n.mute[fi] = !n.mute[fi]; })}
            style={{ display: "flex", alignItems: "center", gap: 3, fontSize: 10, padding: "1px 6px",
              borderRadius: 4, cursor: "pointer",
              border: "1px solid " + (muted ? "var(--color-border-tertiary)" : FCOL[fi]),
              background: muted ? "var(--color-background-secondary)" : "transparent",
              color: muted ? "var(--color-text-tertiary)" : FCOL[fi],
              opacity: muted ? .5 : 1, textDecoration: muted ? "line-through" : "none" }}>
            {name} {stats.counts[fi]}
          </button>);
        })}
        <span style={{ marginLeft: "auto", display: "flex", gap: 5 }}>
          {AN.map((n, i) => P.aw[i] > 0 ? <span key={i} style={{ display: "flex", alignItems: "center", gap: 2 }}>
            <span style={{ width: 7, height: 7, borderRadius: 2, background: AC[i], display: "inline-block" }} />{n}
          </span> : null)}
        </span>
      </div>}

      {/* ─── Panels ─── */}
      {panelOrder.map(pid => {
        const dp = { key: pid, id: pid, resetKey: dockKey,
          onDock: id2 => setPanelOrder(prev => [...prev.filter(p => p !== id2), id2]) };
        switch (pid) {

        case "theme": return (
          <DragPanel {...dp} title={"Theme: " + THN[selTh] + (P.solo >= 0 ? " [SOLO: " + THN[P.solo] + "]" : "")} ini={true}>
            <div style={{ display: "flex", gap: 3, marginBottom: 6, flexWrap: "wrap" }}>
              {THN.map((name, i) => <button key={i} onClick={() => setSelTh(i)} style={{
                fontSize: 10, padding: "2px 7px", borderRadius: 4, cursor: "pointer",
                border: i === selTh ? "2px solid var(--color-border-info)" : "1px solid var(--color-border-tertiary)",
                background: i === selTh ? "var(--color-background-info)" : "var(--color-background-secondary)",
                color: i === selTh ? "var(--color-text-info)" : "var(--color-text-secondary)",
                opacity: P.solo >= 0 && P.solo !== i ? .35 : 1 }}>{name}</button>)}
            </div>
            <div style={{ display: "flex", gap: 4, marginBottom: 6, alignItems: "center" }}>
              <button onClick={() => upd(n => { n.solo = n.solo === selTh ? -1 : selTh; })} style={{
                fontSize: 10, padding: "3px 10px", borderRadius: 4, cursor: "pointer", fontWeight: 600,
                border: P.solo === selTh ? "2px solid #e03030" : "1px solid var(--color-border-tertiary)",
                background: P.solo === selTh ? "rgba(224,48,48,.12)" : "var(--color-background-secondary)",
                color: P.solo === selTh ? "#e03030" : "var(--color-text-secondary)" }}>
                {P.solo === selTh ? "● Solo" : "Solo"}
              </button>
              {P.solo >= 0 && <span style={{ fontSize: 9, color: "#e03030" }}>Only {THN[P.solo]}</span>}
            </div>

            <div style={{ fontSize: 10, fontWeight: 500, color: "var(--color-text-secondary)", marginBottom: 2 }}>Envelope</div>
            <div style={rw}>
              <span style={lb}>Spike</span><Num value={ct.spike} min={0} max={500} step={10} w={42} onChange={v => upd(n => { n.th[selTh].spike = v; })} />
              <span style={lb}>Sustain</span><Num value={ct.sustain} min={0} max={99} step={1} w={34} onChange={v => upd(n => { n.th[selTh].sustain = Math.round(v); })} />
              <span style={lb}>Decay</span><Num value={ct.decay} min={0} max={30} step={1} w={34} onChange={v => upd(n => { n.th[selTh].decay = Math.round(v); })} />
              <span style={lb}>Cool</span><Num value={ct.cooldown} min={0} max={99} step={1} w={34} onChange={v => upd(n => { n.th[selTh].cooldown = Math.round(v); })} />
              <span style={lb}>Wt</span><Num value={ct.weight ?? 0.2} min={0} max={1} w={34} onChange={v => upd(n => { n.th[selTh].weight = v; })} />
            </div>

            <div style={{ fontSize: 10, fontWeight: 500, color: "var(--color-text-secondary)", margin: "4px 0 2px" }}>Spawn weight (lattice)</div>
            <div style={rw}>
              {FN.map((name, fi) => <label key={fi} style={{ fontSize: 10, display: "flex", gap: 2, alignItems: "center", color: FCOL[fi] }}>
                {name.slice(0, 4)} <Num value={ct.sp[fi]} min={0} max={10} w={36} onChange={v => upd(n => { n.th[selTh].sp[fi] = v; })} />
              </label>)}
            </div>

            <div style={{ fontSize: 10, fontWeight: 500, color: "var(--color-text-secondary)", margin: "4px 0 2px" }}>Tier bias</div>
            {FN.map((fname, fi) => <div key={fi} style={{ marginBottom: 2 }}>
              <span style={{ fontSize: 9, color: "var(--color-text-tertiary)" }}>{fname}:</span>
              <div style={rw}>
                {(TN[fi] || []).map((tn, ti) => <label key={ti} style={{ fontSize: 10, display: "flex", gap: 2, alignItems: "center" }}>
                  {tn.slice(0, 3)} <Num value={(ct[TK[fi]] || [])[ti] || 1} min={0} max={10} w={34}
                    onChange={v => upd(n => {
                      if (!n.th[selTh][TK[fi]]) n.th[selTh][TK[fi]] = [1, 1, 1];
                      n.th[selTh][TK[fi]][ti] = v;
                    })} />
                </label>)}
              </div>
            </div>)}
          </DragPanel>);

        case "arch": return (
          <DragPanel {...dp} title="Archetype weights">
            <div style={rw}>
              {AN.map((name, i) => <label key={i} style={{ fontSize: 10, display: "flex", alignItems: "center", gap: 3 }}>
                <span style={{ width: 7, height: 7, borderRadius: 2, background: AC[i], display: "inline-block" }} />
                {name} <Num value={P.aw[i]} min={0} max={5} w={38} onChange={v => upd(n => { n.aw[i] = v; })} />
              </label>)}
            </div>
          </DragPanel>);

        case "spawn": return (
          <DragPanel {...dp} title="Base spawn chances">
            <table style={{ fontSize: 10, borderCollapse: "collapse", width: "100%" }}>
              <thead><tr><th style={{ ...thd, textAlign: "left" }}>Family</th><th style={thd}>Chance</th></tr></thead>
              <tbody>{FK.map((fk, fi) => <tr key={fk}>
                <td style={tdl}>{FN[fi]}</td>
                <td style={tdd}><Num value={P.sc[fk]} min={0} max={1} step={.001} w={52}
                  onChange={v => upd(n => { n.sc[fk] = v; })} /></td>
              </tr>)}</tbody>
            </table>
          </DragPanel>);

        case "tier": return (
          <DragPanel {...dp} title="Base tier weights">
            {FK.map((fk, fi) => <div key={fk} style={{ marginBottom: 4 }}>
              <span style={{ fontSize: 9, fontWeight: 500, color: "var(--color-text-secondary)" }}>{FN[fi]}</span>
              <div style={rw}>
                {(TN[fi] || []).map((tn, ti) => <label key={ti} style={{ fontSize: 10, display: "flex", gap: 2, alignItems: "center" }}>
                  {tn} <Num value={P.tw[fk][ti]} min={0} max={2} w={38} onChange={v => upd(n => { n.tw[fk][ti] = v; })} />
                </label>)}
              </div>
            </div>)}
          </DragPanel>);

        case "sep": return (
          <DragPanel {...dp} title={"Separation (" + NF + "×" + NF + " wu)"}>
            <div style={{ overflowX: "auto" }}>
              <table style={{ fontSize: 10, borderCollapse: "collapse" }}>
                <thead><tr>
                  <th style={{ ...thd, textAlign: "left" }}>↓ / →</th>
                  {FN.map((n, i) => <th key={i} style={thd}>{n.slice(0, 4)}</th>)}
                </tr></thead>
                <tbody>{P.sep.map((row, ri) => <tr key={ri}>
                  <td style={tdl}>{FN[ri]}</td>
                  {row.map((v, ci) => <td key={ci} style={tdd}>
                    <Num value={v} min={0} max={200} step={1} w={34}
                      onChange={nv => upd(n => { n.sep[ri][ci] = nv; })} />
                  </td>)}
                </tr>)}</tbody>
              </table>
            </div>
          </DragPanel>);

        case "prox": return (
          <DragPanel {...dp} title="Proximity affinity" ini={false}>
            <div style={{ fontSize: 10, fontWeight: 500, color: "var(--color-text-secondary)", marginBottom: 2 }}>Per-family parameters</div>
            <div style={{ overflowX: "auto" }}>
              <table style={{ fontSize: 10, borderCollapse: "collapse" }}>
                <thead><tr>
                  <th style={{ ...thd, textAlign: "left" }}>Family</th>
                  <th style={thd}>Radius</th><th style={thd}>Max</th>
                  <th style={thd}>Thresh</th><th style={thd}>Gap×</th>
                </tr></thead>
                <tbody>{FN.map((name, fi) => <tr key={fi}>
                  <td style={tdl}>{name}</td>
                  <td style={tdd}><Num value={P.prox.radius[fi]} min={0} max={300} step={10} w={38}
                    onChange={v => upd(n => { n.prox.radius[fi] = v; })} /></td>
                  <td style={tdd}><Num value={P.prox.maxBoost[fi]} min={1} max={10} step={.5} w={34}
                    onChange={v => upd(n => { n.prox.maxBoost[fi] = v; })} /></td>
                  <td style={tdd}><Num value={P.prox.threshold[fi]} min={0} max={10} step={1} w={30}
                    onChange={v => upd(n => { n.prox.threshold[fi] = Math.round(v); })} /></td>
                  <td style={tdd}><Num value={P.prox.gapReduction[fi]} min={0} max={1} step={.05} w={34}
                    onChange={v => upd(n => { n.prox.gapReduction[fi] = v; })} /></td>
                </tr>)}</tbody>
              </table>
            </div>
            <div style={{ fontSize: 10, fontWeight: 500, color: "var(--color-text-secondary)", margin: "6px 0 2px" }}>Affinity matrix</div>
            <div style={{ overflowX: "auto" }}>
              <table style={{ fontSize: 10, borderCollapse: "collapse" }}>
                <thead><tr>
                  <th style={{ ...thd, textAlign: "left" }}>↓ / →</th>
                  {FN.map((n, i) => <th key={i} style={thd}>{n.slice(0, 4)}</th>)}
                </tr></thead>
                <tbody>{(P.prox.aff || []).map((row, ri) => <tr key={ri}>
                  <td style={tdl}>{FN[ri]}</td>
                  {(row || []).map((v, ci) => <td key={ci} style={tdd}>
                    <Num value={v} min={0} max={3} step={.1} w={34}
                      onChange={nv => upd(n => { n.prox.aff[ri][ci] = nv; })} />
                  </td>)}
                </tr>)}</tbody>
              </table>
            </div>
            <div style={{ fontSize: 9, color: "var(--color-text-tertiary)", marginTop: 2 }}>
              Radius 0 = disabled. Gap× = how much affinity softens separation.
              Thresh = nearby count before boost activates.
            </div>
          </DragPanel>);

        case "density": return (
          <DragPanel {...dp} title="Density">
            <div style={rw}>
              <span style={lb}>Global</span>
              <Num value={P.globalDensity} min={0} max={5} step={.1} w={38}
                onChange={v => upd(n => { n.globalDensity = v; })} />
              <span style={lb}>Min</span>
              <Num value={P.density.min} min={0} max={5} step={.1} w={38}
                onChange={v => upd(n => { n.density.min = v; })} />
              <span style={lb}>Max</span>
              <Num value={P.density.max} min={0} max={5} step={.1} w={38}
                onChange={v => upd(n => { n.density.max = v; })} />
              <span style={lb}>Exp</span>
              <Num value={P.density.exponent} min={0.1} max={3} step={.1} w={38}
                onChange={v => upd(n => { n.density.exponent = v; })} />
              <span style={lb}>Spacing</span>
              <Num value={P.density.spacing} min={50} max={1000} step={50} w={46}
                onChange={v => upd(n => { n.density.spacing = v; })} />
            </div>
            <div style={{ fontSize: 9, color: "var(--color-text-tertiary)", marginTop: 2 }}>
              Global = master multiplier. Field = lattice-interpolated spatial variation.
              Min=Max=1 is flat. Try Min=0.3 Max=2.5 for sparse/dense regions.
            </div>
          </DragPanel>);

        case "batch": return (
          <DragPanel {...dp} title="Population batch">
            <div style={rw}>
              <span style={lb}>Size</span>
              <Num value={P.batch.size} min={1} max={64} step={1} w={34}
                onChange={v => upd(n => { n.batch.size = Math.round(v); })} />
              <span style={lb}>TypeStr</span>
              <Num value={P.batch.typeStr} min={0} max={5} step={.1} w={38}
                onChange={v => upd(n => { n.batch.typeStr = v; })} />
              <span style={lb}>ScaleStr</span>
              <Num value={P.batch.scaleStr} min={0} max={5} step={.1} w={38}
                onChange={v => upd(n => { n.batch.scaleStr = v; })} />
              <span style={lb}>MinObs</span>
              <Num value={P.batch.minObs} min={1} max={10} step={1} w={30}
                onChange={v => upd(n => { n.batch.minObs = Math.round(v); })} />
            </div>
            <div style={rw}>
              <span style={lb}>Aff %</span>
              <Num value={P.batch.affCh} min={0} max={1} step={.05} w={38}
                onChange={v => upd(n => { n.batch.affCh = v; })} />
              <span style={lb}>Rep %</span>
              <Num value={P.batch.repCh} min={0} max={1} step={.05} w={38}
                onChange={v => upd(n => { n.batch.repCh = v; })} />
              <span style={{ fontSize: 9, color: "var(--color-text-tertiary)" }}>
                Neutral: {Math.max(0, 1 - (P.batch.affCh + P.batch.repCh)).toFixed(2)}
              </span>
            </div>
            <div style={{ fontSize: 10, fontWeight: 500, color: "var(--color-text-secondary)", margin: "4px 0 2px" }}>Cross-family affinity</div>
            <div style={{ overflowX: "auto" }}>
              <table style={{ fontSize: 10, borderCollapse: "collapse" }}>
                <thead><tr>
                  <th style={{ ...thd, textAlign: "left" }}>obs↓ / tgt→</th>
                  {FN.map((n, i) => <th key={i} style={thd}>{n.slice(0, 4)}</th>)}
                </tr></thead>
                <tbody>{(P.batch.crossAff || []).map((row, ri) => <tr key={ri}>
                  <td style={tdl}>{FN[ri]}</td>
                  {(row || []).map((v, ci) => <td key={ci} style={tdd}>
                    <Num value={v} min={0} max={5} step={.1} w={34}
                      onChange={nv => upd(n => { n.batch.crossAff[ri][ci] = nv; })} />
                  </td>)}
                </tr>)}</tbody>
              </table>
            </div>
            <div style={{ fontSize: 9, color: "var(--color-text-tertiary)", marginTop: 2 }}>
              TypeStr/ScaleStr = 0 disables batch influence. Aff% + Rep% + Neutral = 1.0.
              Cross-affinity: 1.0 = neutral, &gt;1 = attracts, &lt;1 = suppresses.
            </div>
          </DragPanel>);

        default: return null;
        }
      })}
    </div>
  );
}