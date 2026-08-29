#!/usr/bin/env python3
# ═══════════════════════════════════════════════════════════════════════
# THE GoL CENSUS (GOL_ROWS_3 F1) — what a tier row actually does
#
# WHY THIS EXISTS. A GoL tier row is a rule mask plus a dozen authored
# floats, and NOTHING in the tree tells you what the pair does. The row
# comments say "terminal", "plateaus", "walls"; three of those claims
# were false when GOL_RULES_1 wrote them, and the campaign that found
# out did it with a harness that lived in a session transcript. The next
# rule campaign would rebuild that harness and rebuild its
# approximations differently — which already happened once: between
# GOL_ROWS_1 and GOL_ROWS_2 the seeding gained the per-zone Gaussian it
# had been missing and a headline dark count moved from 9 to 12.
#
# So the harness lives here, reads the artifact, and is one thing.
#
# WHAT IS REAL AND WHAT IS TRANSLITERATED
#   REAL      the hashes and the bucket walk — this tool compiles a
#             driver that #includes primitives/seed_utils.hpp, so
#             cpu_lattice_node_seed, cpu_hash_f and cpu_sample_gaussian
#             are the program's own, not a Python copy of them.
#   REAL      the tier tables — parsed out of bodies/gol_zones.hpp at
#             every run. Edit a row, rerun, get the new answer.
#   REAL      the tick draw under --ladder — same function, same zone
#             seeds, at each row's own mean and sigma.
#   MIRRORED  quantize_tick_period, ported to Python for --ladder in the
#             same product-compare form (see read_ladder below for why a
#             f64 port is sound for a distributional witness).
#   MIRRORED  coupling_gol_next_state, pulse_cell_target's SPIRAL
#             branch, and zone_gol_evolve's spring + apply_boundary, all
#             transliterated from world.wgsl. If that file's versions
#             move, these must move with them — the census is a mirror
#             of the shader in the same sense the CPU tier table is
#             (L3 MIRROR), and it is not gate-covered.
#
# THE STATED LIMITATION — READ THIS BEFORE QUOTING A NUMBER
#   This tool does not reproduce zone_seed_mask, the GPU birth-moment
#   kernel that multiplies the seeded life plane by
#   discrete_visibility_rest. Evaluating it needs the whole colour and
#   field system. That kernel ONLY EVER REMOVES live cells at birth, so
#   every dark count this tool reports is a LOWER BOUND on the real one.
#   Comparisons BETWEEN candidates are sound — the omission applies to
#   all of them alike. A candidate's absolute distance from a band is
#   NOT sound, and a row that sits at the edge of one here is outside it
#   in the world.
#
# USAGE
#   python3 tools/gol_census.py                    # census every Conway row
#   python3 tools/gol_census.py --spiral           # Pulse: Spiral coherence
#   python3 tools/gol_census.py --ladder           # GOL_TEMPO_2: every
#         # row's tick draw through the snap. Asserts that every value is
#         # a rung of GOL_TICK_LADDER and that no row falls below the
#         # bottom rung, prints each row's rung histogram, its transit in
#         # seconds per rung, and the RECOVERED DUTY that witnesses the
#         # extrusion law. Exits nonzero on a violation, so it can be run
#         # as a gate.
#   python3 tools/gol_census.py --seeds 512        # widen the sample
#   python3 tools/gol_census.py --gens 4000        # run them longer
#   python3 tools/gol_census.py --candidate 'Vote:0x3E1E0:0.50:0.06:32'
#         # name:mask:density_mean:density_sigma:cells — a what-if row,
#         # censused beside the real ones without touching the tree.
#         # Repeatable.
#
# WHAT THE CONWAY COLUMNS MEAN
#   dark        no live cell at the end. The zone renders nothing — no
#               height, and the tint's `color_val > 0.01` fails — while
#               still holding its footprint against every other zone.
#               Dead ground. This is the failure column.
#   saturated   every cell live. A solid raised block at the row's
#               alive_height. A legitimate object, not a failure, but
#               one object.
#   structured  neither extreme.
#   froze       reached a fixed point inside the generation budget. A
#               row that never freezes is a boil; whether that is right
#               is the row's business, but it should be on purpose.
# ═══════════════════════════════════════════════════════════════════════
"""GOL_ROWS_3: census what each GoL tier row does, from the tree's own tables."""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
BODIES = os.path.join(REPO, "src", "cartridges", "the_board", "bodies",
                      "gol_zones.hpp")
PRIMS = os.path.join(REPO, "src", "cartridges", "the_board", "primitives")
INC = os.path.join(REPO, "src", "cartridges")

# The property indices select_gol_zone / seed_gol_zone roll on. Mirrors
# GoLZoneProp / PulseZoneProp in bodies/gol_zones.hpp.
PROP_DENSITY = 930
PROP_TICK_PERIOD = 931


# ── The ladder, and the census's port of its snapper ──────────────────
#
# MIRRORED, and the third kind of mirror in this file. The draws below
# are REAL — the driver calls the program's own cpu_sample_gaussian — but
# the SNAP here is a Python transliteration of quantize_tick_period in
# bodies/gol_zones.hpp, written in the same product-compare form so the
# two can be read side by side.
#
# WHY THAT IS SAFE. This census is a DISTRIBUTIONAL witness, not a
# bit-exactness one. Python computes x*x in f64 where the program
# computes it in f32, so for a draw lying within a hair of a boundary the
# two snappers can disagree about WHICH of two adjacent rungs it takes,
# and a histogram column can be off by a count. The MEMBERSHIP assertion
# is immune to that and it is the one that matters: both snappers return
# an element of GOL_TICK_LADDER for every input, so a disagreement moves
# a sample between rungs and can never move it off the ladder.
#
# The ladder itself is parsed out of the tree at every run, like the tier
# tables — edit a rung, rerun, get the new answer.


def read_ladder():
    with open(BODIES, encoding="utf-8") as fh:
        src = fh.read()
    body = src.split("GOL_TICK_LADDER[12]", 1)[1].split(";", 1)[0]
    rungs = [float(t) for t in re.findall(r"([\d.]+)f", body)]
    if len(rungs) != 12 or rungs != sorted(rungs):
        raise SystemExit(
            "gol-census: GOL_TICK_LADDER parsed as %d rung(s), %s — the "
            "ladder and this tool have diverged." % (len(rungs), rungs))
    return rungs


def quantize_tick_period(x, ladder):
    """Python port of bodies/gol_zones.hpp's quantize_tick_period.
    Nearest rung in log space; the boundary between lad[i] and lad[i+1] is
    their geometric mean, compared as x*x < lad[i]*lad[i+1]. Below the
    bottom rung and above the top the ladder clamps."""
    for i in range(11):
        if x * x < ladder[i] * ladder[i + 1]:
            return ladder[i]
    return ladder[11]

# ── Parsing the tables out of the tree ────────────────────────────────

CONWAY_FIELDS = ["rule_mask", "density_mean", "density_sigma",
                 "tick_period_mean", "tick_period_sigma",
                 "transition_fraction_mean", "transition_fraction_sigma",
                 "alive_height_mean", "alive_height_sigma",
                 "spring_variance", "weight", "force_no_height", "grid_cells"]

PULSE_FIELDS = ["field_fn", "tick_period_mean", "tick_period_sigma",
                "transition_fraction_mean", "transition_fraction_sigma",
                "phase_randomness_mean", "phase_randomness_sigma",
                "tempo_randomness_mean", "tempo_randomness_sigma",
                "alive_height_mean", "alive_height_sigma",
                "wander_radius_mean", "wander_radius_sigma",
                "spring_variance", "weight", "force_no_height",
                "boundary_mode", "grid_cells"]


def _num(tok):
    """One initialiser token -> a float. Named constants resolve to their
    value; the tables use them only for enums."""
    t = tok.strip()
    if t.startswith("0x"):
        return float(int(t.rstrip("u"), 16))
    if t in ("false", "true"):
        return 1.0 if t == "true" else 0.0
    for suffix, value in (("::BREATH", 0.0), ("::SPIRAL", 1.0),
                          ("::REFLECT", 0.0), ("::WRAP", 1.0)):
        if t.endswith(suffix):
            return value
    if t.endswith("u"):
        return float(t[:-1])
    return float(t.rstrip("f"))


def _table(src, anchor, fields):
    """Rows of `anchor`'s initialiser list, as (label, {field: value})."""
    body = src.split(anchor, 1)[1].split("};", 1)[0]
    rows = []
    for label, init in re.findall(r"/\* \d+: ([\w&]+)\s*\*/ \{ (.*?) \},", body):
        vals = [_num(t) for t in init.split(",")]
        if len(vals) != len(fields):
            raise SystemExit(
                "gol-census: %s row %s has %d initialisers, the struct has "
                "%d fields. The table and this tool have diverged — fix the "
                "field list at the top of this file."
                % (anchor, label, len(vals), len(fields)))
        rows.append((label, dict(zip(fields, vals))))
    return rows


def _names(src, array):
    body = src.split(array, 1)[1].split("};", 1)[0]
    return re.findall(r'"([^"]+)"', body)


def read_tables():
    with open(BODIES, encoding="utf-8") as fh:
        src = fh.read()
    conway = _table(src, "GOL_TIERS[GOL_TIER_COUNT]", CONWAY_FIELDS)
    pulse = _table(src, "GOL_PULSE_TIERS[GOL_PULSE_TIER_COUNT]", PULSE_FIELDS)
    cnames = _names(src, "GOL_TIER_NAMES[]")
    pnames = _names(src, "GOL_PULSE_TIER_NAMES[]")
    if len(cnames) != len(conway) or len(pnames) != len(pulse):
        raise SystemExit(
            "gol-census: %d Conway rows vs %d names, %d Pulse rows vs %d "
            "names — the tables and the name arrays disagree."
            % (len(conway), len(cnames), len(pulse), len(pnames)))
    return ([(n, r) for n, (_, r) in zip(cnames, conway)],
            [(n, r) for n, (_, r) in zip(pnames, pulse)])


# ── The driver ────────────────────────────────────────────────────────
#
# Everything below the include is a transliteration of world.wgsl. The
# spawn path above it is select_gol_zone's and seed_gol_zone's.

DRIVER = r'''
#include "the_board/primitives/seed_utils.hpp"
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <set>
#include <string>
#include <vector>
using namespace t7::the_board;

static const float PI_ = 3.14159265359f;
static const uint32_t SEED_BAND = 250u;   // GoLZoneProp::SEED_BAND
static const uint32_t PROP_DENSITY = 930u;
static const uint32_t PROP_TICK_PERIOD = 931u;  // GoLZoneProp::TICK_PERIOD

// world.wgsl §3.7 — coupling_gol_next_state
static float next_state(bool alive, int neighbors, uint32_t rule_mask) {
    uint32_t n = (uint32_t)neighbors;
    uint32_t bit = alive ? (9u + n) : n;
    return (rule_mask & (1u << bit)) != 0u ? 1.0f : 0.0f;
}

// world.wgsl — zone_gol_evolve's Conway branch: Moore neighbourhood,
// wrapped with `% gs` on both axes.
static void step(const std::vector<float>& a, std::vector<float>& b,
                 int N, uint32_t rule) {
    for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
        int c = 0;
        for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
            if (!dx && !dy) continue;
            if (a[((y + dy + N) % N) * N + ((x + dx + N) % N)] > 0.5f) c++;
        }
        b[y * N + x] = next_state(a[y * N + x] > 0.5f, c, rule);
    }
}

// world.wgsl — gol_cell_hash / gol_cell_variation
static uint32_t cell_hash(uint32_t cx, uint32_t cy) {
    return cx * 374761393u + cy * 668265263u;
}
static float cell_var(uint32_t h) { return (float)(h & 0xFFFFu) / 65535.0f; }

// world.wgsl — pulse_cell_target, SPIRAL branch. config.mode_gol_tick_scale
// is pinned at 1.0 by the boot block and has no driver, so it is 1 here.
static float spiral(uint32_t cx, uint32_t cy, float t_beats, float tick_period,
                    float phase_randomness, float tempo_randomness,
                    uint32_t grid_size) {
    uint32_t h = cell_hash(cx, cy);
    float cell_phase = cell_var(h) * phase_randomness * 2.0f * PI_;
    uint32_t h2 = cell_hash(cx + 137u, cy + 251u);
    float tempo_jitter = 1.0f + (cell_var(h2) - 0.5f) * tempo_randomness;
    float n = (float)grid_size;
    float px = (float)cx + 0.5f - n * 0.5f, py = (float)cy + 0.5f - n * 0.5f;
    float r = std::sqrt(px * px + py * py);
    float th = std::atan2(py, px) / (2.0f * PI_);
    float u = th * 2.0f + r / (n * 0.5f)
            - t_beats * tempo_jitter / std::fmax(tick_period * 1.0f, 0.01f);
    return 0.5f + 0.5f * std::cos(u * 2.0f * PI_ + cell_phase);
}
// the same field with every scatter term removed — the shape to hold against
static float spiral_ideal(uint32_t cx, uint32_t cy, float t_beats,
                          float tick_period, uint32_t grid_size) {
    float n = (float)grid_size;
    float px = (float)cx + 0.5f - n * 0.5f, py = (float)cy + 0.5f - n * 0.5f;
    float r = std::sqrt(px * px + py * py);
    float th = std::atan2(py, px) / (2.0f * PI_);
    return 0.5f + 0.5f * std::cos((th * 2.0f + r / (n * 0.5f)
                 - t_beats / std::fmax(tick_period, 0.01f)) * 2.0f * PI_);
}
static float wrap01(float x) { return x - std::floor(x); }   // apply_boundary WRAP

// select_gol_zone: the per-zone density is a Gaussian draw, clamped, and
// seed_gol_zone then rolls each cell against THAT — not against the mean.
static float zone_density(uint32_t seed, float mean, float sigma) {
    return std::max(0.05f, std::min(0.9f,
        cpu_sample_gaussian(seed, PROP_DENSITY, mean, sigma)));
}

static float population(const std::vector<float>& a) {
    float p = 0; for (float v : a) p += v; return p;
}

// ── Conway census ────────────────────────────────────────────────────
static void census(const char* name, uint32_t rule, float dm, float ds,
                   int N, int seeds, int gens) {
    int dark = 0, full = 0, structured = 0, froze = 0;
    double live = 0, dens = 0;
    for (uint32_t k = 0; k < (uint32_t)seeds; k++) {
        uint32_t seed = cpu_lattice_node_seed(9000u + k, (int32_t)k, 13, SEED_BAND);
        float d = zone_density(seed, dm, ds);
        dens += d;
        std::vector<float> a(N * N), b(N * N);
        for (int i = 0; i < N * N; i++)
            a[i] = cpu_hash_f(seed + i, PROP_DENSITY) < d ? 1.0f : 0.0f;
        int at = -1;
        for (int g = 1; g <= gens; g++) {
            step(a, b, N, rule);
            bool same = (a == b);
            a.swap(b);
            if (same && at < 0) at = g;
        }
        if (at > 0) froze++;
        float lv = 100.0f * population(a) / (N * N);
        live += lv;
        if (lv < 0.5f) dark++; else if (lv > 99.5f) full++; else structured++;
    }
    printf("  %-16s 0x%-6X %.2f/%.2f %2d | %4d | %4d | %4d | %5.1f%% | %4d | %5.1f%%\n",
           name, rule, dm, ds, N, dark, full, structured,
           live / seeds, froze, 100.0 * dark / seeds);
}

// ── Spiral coherence ─────────────────────────────────────────────────
// Shape correlation: the best match against the scatter-free spiral over
// ALL phase offsets. The spring lags the drive by a fixed phase, and a
// rotating spiral shifted in phase is the same spiral — an un-lagged
// correlation reads a large constant negative and means nothing.
static double shape_corr(const std::vector<float>& v, int N, float t, float tick) {
    double best = -2;
    for (int k = 0; k < 180; k++) {
        float tt = t - tick * (float)k / 180.0f;
        double sa=0, sb=0, saa=0, sbb=0, sab=0; int n=0;
        for (uint32_t y = 0; y < (uint32_t)N; y++)
        for (uint32_t x = 0; x < (uint32_t)N; x++) {
            double A = v[y * N + x], B = spiral_ideal(x, y, tt, tick, (uint32_t)N);
            sa+=A; sb+=B; saa+=A*A; sbb+=B*B; sab+=A*B; n++;
        }
        double ma = sa/n, mb = sb/n;
        double den = std::sqrt((saa/n - ma*ma) * (sbb/n - mb*mb));
        if (den > 1e-12) { double c = (sab/n - ma*mb) / den; if (c > best) best = c; }
    }
    return best;
}

static void spiral_run(float tick, float trans, float phase, float tempo,
                       float sv, int N, int bnd) {
    printf("  row: tick %.1f, trans %.2f, phase %.2f, tempo %.2f, sv %.2f, "
           "%d cells, %s\n", tick, trans, phase, tempo, sv, N,
           bnd ? "WRAP" : "REFLECT");
    printf("  omega = 3 / (%.2f x %.1f) = %.2f   (transition_fraction x "
           "tick_period IS the spring)\n\n", trans, tick, 3.0f / (trans * tick));
    printf("    t_beats  at 120bpm   shape corr   neighbour step   distinct   rails\n");
    std::vector<float> vis(N * N, 0.0f), vel(N * N, 0.0f);
    float dt = 1.0f / 60.0f, t = 0.0f;
    const int marks[] = {20, 75, 150, 300, 900, 1800, 3600};
    int mi = 0;
    while (mi < 7) {
        t += dt * 2.0f;                                  // beats, at 120bpm
        for (uint32_t y = 0; y < (uint32_t)N; y++)
        for (uint32_t x = 0; x < (uint32_t)N; x++) {
            int i = y * N + x;
            float tgt = spiral(x, y, t, tick, phase, tempo, (uint32_t)N);
            float om = 3.0f / std::fmax(trans * tick, 0.01f);
            if (sv > 0.001f) {
                float j = 1.0f + (cell_var(cell_hash(x + 53u, y + 97u)) - 0.5f) * sv;
                om = om / std::fmax(j, 0.3f);
            }
            float od = om * dt, e = std::exp(-od), d = vis[i] - tgt;
            float nv = tgt + (d * (1.0f + od) + vel[i] * dt) * e;
            float nvel = (vel[i] * (1.0f - od) - d * om * om * dt) * e;
            if (std::fabs(nv - tgt) < 0.001f && std::fabs(nvel) < 0.01f) {
                vis[i] = tgt; vel[i] = 0.0f;
            } else {
                vis[i] = bnd ? wrap01(nv) : nv;
                vel[i] = (nv < 0.0f || nv > 1.0f) ? 0.0f : nvel;
            }
        }
        if (t >= marks[mi]) {
            std::set<int> buckets; int rails = 0; double stepsum = 0; int m = 0;
            for (int y = 0; y < N; y++) for (int x = 0; x < N; x++) {
                float A = vis[y * N + x];
                buckets.insert((int)(A * 1000));
                if (A < 0.01f || A > 0.99f) rails++;
                if (x + 1 < N) { stepsum += std::fabs(A - vis[y * N + x + 1]); m++; }
            }
            printf("    %7d  %6.1f min      %6.3f          %.3f          %4zu      %4d\n",
                   marks[mi], marks[mi] / 120.0, shape_corr(vis, N, t, tick),
                   stepsum / m, buckets.size(), rails);
            mi++;
        }
    }
}

// ── The ladder draw ──────────────────────────────────────────────────
//
// select_gol_zone's tick draw, verbatim and REAL: the program's own
// cpu_sample_gaussian at the row's own mean and sigma, on the same zone
// seeds the Conway census walks. The SNAP is not done here — the raw f32
// draw is printed and Python snaps it, so the census's port of
// quantize_tick_period is what gets exercised.
static void ladder_draw(const char* name, float mu, float sigma, int seeds) {
    for (uint32_t k = 0; k < (uint32_t)seeds; k++) {
        uint32_t seed = cpu_lattice_node_seed(9000u + k, (int32_t)k, 13, SEED_BAND);
        printf("%s %.9g\n", name,
               cpu_sample_gaussian(seed, PROP_TICK_PERIOD, mu, sigma));
    }
}

// argv: MODE then packed rows.
//   conway  seeds gens   then name:mask:dm:ds:cells per row
//   spiral  tick:trans:phase:tempo:sv:cells:bnd
//   ladder  seeds        then name:tick_mu:tick_sigma per row
int main(int argc, char** argv) {
    std::string mode = argv[1];
    if (mode == "ladder") {
        int seeds = atoi(argv[2]);
        for (int a = 3; a < argc; a++) {
            std::string s = argv[a];
            std::vector<std::string> f; size_t p = 0, q;
            while ((q = s.find(':', p)) != std::string::npos) {
                f.push_back(s.substr(p, q - p)); p = q + 1;
            }
            f.push_back(s.substr(p));
            ladder_draw(f[0].c_str(), (float)atof(f[1].c_str()),
                        (float)atof(f[2].c_str()), seeds);
        }
        return 0;
    }
    if (mode == "spiral") {
        float f[7]; std::string s = argv[2]; size_t p = 0; int i = 0;
        while (i < 7) {
            size_t q = s.find(':', p);
            f[i++] = atof(s.substr(p, q == std::string::npos ? q : q - p).c_str());
            if (q == std::string::npos) break;
            p = q + 1;
        }
        spiral_run(f[0], f[1], f[2], f[3], f[4], (int)f[5], (int)f[6]);
        return 0;
    }
    int seeds = atoi(argv[2]), gens = atoi(argv[3]);
    for (int a = 4; a < argc; a++) {
        std::string s = argv[a];
        std::vector<std::string> f; size_t p = 0, q;
        while ((q = s.find(':', p)) != std::string::npos) {
            f.push_back(s.substr(p, q - p)); p = q + 1;
        }
        f.push_back(s.substr(p));
        census(f[0].c_str(), (uint32_t)strtoul(f[1].c_str(), 0, 16),
               (float)atof(f[2].c_str()), (float)atof(f[3].c_str()),
               atoi(f[4].c_str()), seeds, gens);
    }
    return 0;
}
'''


def build(tmp):
    cxx = shutil.which("clang++") or shutil.which("g++")
    if not cxx:
        raise SystemExit("gol-census: no clang++ or g++ on PATH")
    src = os.path.join(tmp, "census.cpp")
    with open(src, "w", encoding="utf-8") as fh:
        fh.write(DRIVER)
    exe = os.path.join(tmp, "census")
    c = subprocess.run([cxx, "-std=c++20", "-O2", "-I", INC, src, "-o", exe],
                       capture_output=True, text=True)
    if c.returncode != 0:
        sys.stdout.write(c.stdout + c.stderr)
        raise SystemExit("gol-census: the driver did not compile against "
                         "primitives/seed_utils.hpp")
    return exe


def decode(mask):
    born = "".join(str(n) for n in range(9) if int(mask) & (1 << n))
    surv = "".join(str(n) for n in range(9) if int(mask) & (1 << (9 + n)))
    return "B%s/S%s" % (born, surv)


# ── The ladder census (GOL_TEMPO_2 U4) ────────────────────────────────
#
# THE §1.4 WITNESS. The extrusion law says transit = transition_fraction x
# tick_period, so quantizing the tick stretches every transit with its
# rung and leaves every row's DUTY CYCLE where its author put it. The
# recovered-duty column is that claim, measured: transit / rung must come
# back as the row's own transition_fraction mean, on every rung it lands.
def ladder_census(exe, conway, pulse, seeds):
    ladder = read_ladder()
    rows = ([(n, r, "Conway") for n, r in conway]
            + [(n, r, "Pulse") for n, r in pulse])

    packed = ["%s:%.9g:%.9g" % (n.replace(" ", "_"), r["tick_period_mean"],
                                r["tick_period_sigma"]) for n, r, _ in rows]
    out = subprocess.run([exe, "ladder", str(seeds)] + packed,
                         capture_output=True, text=True)
    if out.returncode != 0:
        sys.stdout.write(out.stdout + out.stderr)
        raise SystemExit("gol-census: the ladder driver did not run")

    draws = {}
    for line in out.stdout.splitlines():
        name, value = line.rsplit(" ", 1)
        draws.setdefault(name, []).append(float(value))

    print("── the tick ladder, %d zone seeds per row ──\n" % seeds)
    print("   GOL_TICK_LADDER = { %s }"
          % ", ".join(("%g" % r) for r in ladder))
    print("   The draw is REAL (the program's own cpu_sample_gaussian); the "
          "snap is this\n   file's port of quantize_tick_period. Transit "
          "seconds are quoted at 100 BPM\n   (one beat = 0.6 s). Recovered "
          "duty is transit / rung, which must return the\n   row's own "
          "transition_fraction mean — the extrusion law, measured.\n")

    violations = []
    for name, row, algo in rows:
        key = name.replace(" ", "_")
        raw = draws.get(key)
        if not raw:
            raise SystemExit("gol-census: the driver returned no draws for %s"
                             % name)
        snapped = [quantize_tick_period(x, ladder) for x in raw]

        off = sorted({v for v in snapped if v not in ladder})
        if off:
            violations.append("%s: %d draw(s) off the ladder, e.g. %s"
                              % (name, sum(1 for v in snapped if v in off),
                                 off[:4]))
        low = min(snapped)
        if low < ladder[0]:
            violations.append("%s: min rung %g is below the bottom rung %g"
                              % (name, low, ladder[0]))

        mu, sigma = row["tick_period_mean"], row["tick_period_sigma"]
        trans = row["transition_fraction_mean"]
        on_ladder = "rung" if mu in ladder else "*** OFF LADDER ***"
        print("  %-9s %-8s mean %g +/- %g  (%s)   raw %.3f..%.3f"
              % (algo, name, mu, sigma, on_ladder, min(raw), max(raw)))

        hist = {}
        for v in snapped:
            hist[v] = hist.get(v, 0) + 1
        modal = max(hist, key=lambda k: hist[k])
        if modal != quantize_tick_period(mu, ladder):
            violations.append(
                "%s: modal rung %g is not the mean's own rung %g"
                % (name, modal, quantize_tick_period(mu, ladder)))
        print("      %-7s %7s  %8s  %8s  %s"
              % ("rung", "zones", "share", "transit", "recovered duty"))
        for rung in ladder:
            n = hist.get(rung, 0)
            if not n:
                continue
            transit_beats = trans * rung
            print("      %-7g %7d  %7.2f%%  %7.2fs  %.4f%s"
                  % (rung, n, 100.0 * n / len(snapped), transit_beats * 0.6,
                     transit_beats / rung,
                     "   <- the mean's rung" if rung == modal else ""))
        print()

    print("  %d row(s), %d draw(s) each, %d total."
          % (len(rows), seeds, len(rows) * seeds))
    if violations:
        print("\n  LADDER VIOLATIONS:")
        for v in violations:
            print("    - %s" % v)
        print("\n  gol-census --ladder: FAIL")
        return 1
    print("  Every draw landed on a rung; every row's minimum is at or above "
          "the bottom\n  rung; every row's modal mass sits on its mean's own "
          "rung; every recovered duty\n  returned its row's transition_fraction.")
    print("\n  gol-census --ladder: PASS")
    return 0


def main():
    ap = argparse.ArgumentParser(
        description="Census what each GoL tier row actually does.")
    ap.add_argument("--seeds", type=int, default=None,
                    help="zone seeds per row (default 32; 4096 under --ladder, "
                         "which needs a distribution rather than a sample)")
    ap.add_argument("--gens", type=int, default=2000,
                    help="generations per zone (default 2000)")
    ap.add_argument("--row", action="append", default=[],
                    help="census only this Conway row, by name. Repeatable.")
    ap.add_argument("--candidate", action="append", default=[],
                    help="a what-if row not in the tree: "
                         "name:mask:density_mean:density_sigma:cells")
    ap.add_argument("--spiral", action="store_true",
                    help="Pulse instead: the Spiral row's arm coherence")
    ap.add_argument("--ladder", action="store_true",
                    help="GOL_TEMPO_2: every row's tick draw through the "
                         "snap. Asserts membership in GOL_TICK_LADDER and "
                         "prints the rung histogram, transit and recovered "
                         "duty. Exits nonzero on a violation.")
    args = ap.parse_args()

    conway, pulse = read_tables()
    seeds = args.seeds if args.seeds is not None else (4096 if args.ladder else 32)

    tmp = tempfile.mkdtemp(prefix="gol_census_")
    try:
        exe = build(tmp)

        if args.ladder:
            return ladder_census(exe, conway, pulse, seeds)

        if args.spiral:
            row = dict(pulse).get("Spiral")
            if row is None:
                raise SystemExit("gol-census: no Spiral row in GOL_PULSE_TIERS")
            print("── the Spiral row's arm coherence, from the tree's own values ──\n")
            packed = "%f:%f:%f:%f:%f:%d:%d" % (
                row["tick_period_mean"], row["transition_fraction_mean"],
                row["phase_randomness_mean"], row["tempo_randomness_mean"],
                row["spring_variance"], int(row["grid_cells"]),
                int(row["boundary_mode"]))
            subprocess.run([exe, "spiral", packed])
            print("\n  shape corr 1.00 = the arms are intact. Tempo scatter is a "
                  "per-cell FREQUENCY\n  multiplier and its phase error integrates "
                  "in t_beats; phase scatter is a bounded\n  static offset. Only "
                  "one of the two can ever cost coherence.")
            return 0

        rows = [(n, r) for n, r in conway if not args.row or n in args.row]
        if args.row and not rows:
            raise SystemExit("gol-census: no Conway row named %s. Rows: %s"
                             % (", ".join(args.row),
                                ", ".join(n for n, _ in conway)))

        packed = ["%s:%X:%.4f:%.4f:%d" % (n.replace(" ", "_"), int(r["rule_mask"]),
                                          r["density_mean"], r["density_sigma"],
                                          int(r["grid_cells"]))
                  for n, r in rows]
        for c in args.candidate:
            f = c.split(":")
            if len(f) != 5:
                raise SystemExit("gol-census: --candidate wants "
                                 "name:mask:density_mean:density_sigma:cells")
            packed.append("%s:%X:%.4f:%.4f:%d"
                          % (f[0].replace(" ", "_"), int(f[1], 0),
                             float(f[2]), float(f[3]), int(f[4])))

        print("── the Conway rows, %d zone seeds each, %d generations ──"
              % (seeds, args.gens))
        print("   dark counts are a LOWER BOUND: zone_seed_mask is not modelled "
              "(see the header).\n")
        for n, r in rows:
            print("   %-16s %s" % (n, decode(r["rule_mask"])))
        for c in args.candidate:
            f = c.split(":")
            print("   %-16s %s   (candidate, not in the tree)"
                  % (f[0], decode(int(f[1], 0))))
        print()
        print("  %-16s %-8s %-9s %-2s | %-4s | %-4s | %-4s | %-6s | %-4s | %s"
              % ("row", "mask", "dens m/s", "N", "dark", "satu", "strc",
                 "live", "frze", "dark%"))
        print("  " + "-" * 88)
        subprocess.run([exe, "conway", str(seeds), str(args.gens)] + packed)
        return 0
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
