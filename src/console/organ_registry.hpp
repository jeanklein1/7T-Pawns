#pragma once
// ═══════════════════════════════════════════════════════════════════════
// ORGAN — THE COMPILED REGISTRY AND THE PANEL'S C ABI
//
// THE COMPILED-REGISTRY LAW (docs/ORGAN.md, L22): every entry's offset is
// `offsetof` against the declaration the program reads, so a rename fails
// the BUILD at the enrollment line. There is no second copy to drift from.
// THE SOVEREIGNTY BOUNDARY is state.hpp's — three accessors and no others,
// so GPU truth has no block id and a panel that cannot name a thing cannot
// write it. Blocks 3 and above are contracts-tier CPU banks, each its own
// base. THE MANIFEST IS THE WHITELIST: organ_set refuses any (block,
// offset, type) triple that is not an entry, counts it, and names it.
// ═══════════════════════════════════════════════════════════════════════

#include "core/instruments.hpp"                          // PURSE_0 R2 — t7::BUILD_STAMP, the tree the panel names
#include "cartridges/the_board/realization/state.hpp"
#include "cartridges/the_board/contracts/spine_state.hpp"   // MoodProfile + mood_def: the definition side
#include "cartridges/the_board/contracts/agent_tiers.hpp"    // TIER_LIVE, the world's definition bank
#include "cartridges/the_board/contracts/pawn_surface.hpp"    // PAWN_AURA_LIVE (block 4)
#include "cartridges/the_board/contracts/orb_surface.hpp"     // ORB_CONSOLE_LIVE (block 5)
#include "cartridges/the_board/contracts/orb_conductor.hpp"   // ORB_CONDUCTOR_LIVE (block 12)
#include "cartridges/the_board/contracts/control_panel.hpp"   // PANEL_LIVE (block 6)
#include "cartridges/the_board/contracts/ribbon_surface.hpp"  // RIBBON_LIVE (block 7)
#include "cartridges/the_board/contracts/indoor_module.hpp"   // INDOOR_LIVE (block 8, destructive)
#include "cartridges/the_board/contracts/mood_constants.hpp"  // WORLD_DRAW_LIVE (block 10, destructive)
#include "coupling/canvas_surface.hpp"                        // CANVAS_LIVE (block 9, t7::canvas)
#include "cartridges/the_board/contracts/driver_surface.hpp"  // the drivers' room (block 3)

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace t7 {
namespace organ {

// ─── Type tags ────────────────────────────────────────────────────────
// The lane count is the whole difference between them at the ABI: a VEC3
// is three contiguous floats, and a colour is a VEC3 over 0..1 that the
// panel happens to render with a colour input.
enum : uint8_t {
    ORGAN_F32 = 0,
    ORGAN_U32 = 1,
    ORGAN_BOOL = 2,
    ORGAN_VEC3 = 3,
    ORGAN_VEC4 = 4,
};

inline int lanes_of(uint8_t type) {
    switch (type) {
    case ORGAN_VEC3: return 3;
    case ORGAN_VEC4: return 4;
    default:         return 1;
    }
}

// ─── Block ids ────────────────────────────────────────────────────────
// One per CPU home GPUState hands the panel; the number is the bit
// position organ_mark_dirty uses. A BLOCK ID IS NOT A PROMISE ABOUT HOW
// THE HOME REACHES THE GPU: config_ is staged and the spine uploads it off
// configDirty_, lightingStage_ and agentRoomStage_ are flushed by
// organ_flush, so organ_mark_dirty routes and this enum does not.

// WINDOWS ARE NOT HOMES, so there is no id for GPUSceneConstants: its
// tier_gains are a window onto the agents' room, its figure profiles are
// packed from a constexpr table, its ribbon's home is bodies/ribbon.hpp.
// Nor one for frame_r — only its lighting region is CPU-authored, and that
// region has its own home and its own id.
enum : uint8_t {
    ORGAN_BLOCK_CONFIG     = 0,   // GPUDesignConfig      — config_
    ORGAN_BLOCK_LIGHTING   = 1,   // GPULighting          — lightingStage_
    ORGAN_BLOCK_AGENT_ROOM = 2,   // GPUAgentRoomConstants — agentRoomStage_
    ORGAN_BLOCK_DRIVERS    = 3,   // DriverSurface — DRIVER_LIVE; CPU-read,
                                  // and the seams are its flush
    // THE GRADUATED MODULE BANKS. Each is a contracts-tier CPU surface: a
    // module's authored console, given a live shadow so the panel can name
    // it. Block 3's shape — no accessor, no upload, and the readers that
    // consume the bank each tick ARE its flush. The bit space is
    // organTouched_, 32 bits.
    ORGAN_BLOCK_PAWN       = 4,   // PawnAuraProfile       — PAWN_AURA_LIVE
    ORGAN_BLOCK_ORBS       = 5,   // OrbConsole            — ORB_CONSOLE_LIVE
    ORGAN_BLOCK_PANEL      = 6,   // PanelSurface          — PANEL_LIVE
    ORGAN_BLOCK_RIBBON     = 7,   // RibbonSurface         — RIBBON_LIVE
    ORGAN_BLOCK_INDOOR     = 8,   // IndoorSurface — INDOOR_LIVE; DESTRUCTIVE
    ORGAN_BLOCK_CANVAS     = 9,   // canvas::CanvasSurface — CANVAS_LIVE
    // THE TWO DESTRUCTIVE BANKS. Both are read while a world or a ribbon is
    // being DRAWN and never re-read, so both keep INDOOR_LIVE's
    // temperament: a plain block id, no boundary wiring, GEN on every row.
    // The stricter temperament governs.
    ORGAN_BLOCK_WORLD        = 10,  // WorldDrawSurface   — WORLD_DRAW_LIVE
    ORGAN_BLOCK_RIBBON_SPAWN = 11,  // RibbonSpawnSurface — RIBBON_SPAWN_LIVE
                                    // (ATRIUM_2 — the arc and the sand, read as
                                    //  the entrance is drawn and not re-read)
    ORGAN_BLOCK_CONDUCTOR    = 12,  // OrbConductorConsole — ORB_CONDUCTOR_LIVE
                                    // (ORRERY_2 — the orb conductor's own
                                    //  console; the tick's row-watch lands a
                                    //  panel edit mid-reign)
    ORGAN_BLOCK_COUNT        = 13,
};

// A definition-only entry has no instance anywhere: block_base answers
// null, organ_set routes it straight to the definition path, and preview
// on it is refused. Its `offset` carries def_offset, so the triple stays
// unique and the manifest round-trips.

// ONE SENTINEL PER FAMILY, because a definition-only entry's `offset` is
// an offset into ITS OWN struct: two families on one sentinel would let
// two structs' offsets collide at the same number and resolve to each
// other. The convention DESCENDS from 255 — a third family takes 253 —
// which keeps the real ids growing upward and keeps `is_defonly` an
// explicit list rather than a range test.
enum : uint8_t {
    ORGAN_BLOCK_NONE     = 255,   // the_board::MoodProfile family
    ORGAN_BLOCK_NONE_ORB = 254,   // the_board::OrbMoodConfig family
};

// ─── Definition targets ───────────────────────────────────────────────
// Where a dial's DEFINITION lives, if it has one. An entry's home is its
// INSTANCE, on loan wherever a second author speaks; the definition is the
// fact that author reads when it next does, so writing it is how a panel
// edit outlives the author (docs/ORGAN.md, "Definition and preview").

// NEVER REINTERPRET; CONVERSION IS PERMITTED. A float target is a run of
// floats with the entry's lane count. An integer target — U32 or BOOL — is
// CONVERTED, by the rule organ_set's instance path uses, and never has a
// float's bit pattern written into it.

// A KIND NAMES THE FAMILY, AND THE FAMILY ANSWERS ONE QUESTION: MOOD, what
// this mood means (one row per mood, the target selects); TIER and
// BEHAVIOR, what this WORLD means (one bank each, so the target is ignored
// by design). definition_base is where that mapping lives. A KIND IS NOT A
// FLAG — two kinds share a flag when they share an AUTHOR, which is why
// BEHAVIOR raises TIER's.
enum : uint8_t {
    ORGAN_DEF_NONE = 0,   // no definition: the home IS the only truth there is
    ORGAN_DEF_MOOD = 1,   // the_board::MoodProfile — per-mood, target selects
    ORGAN_DEF_TIER = 2,   // the_board::AgentTierBank — the world's, one bank
    ORGAN_DEF_BEHAVIOR = 3,  // AgentBehaviorBank — the world's; raises TIER's flag
    ORGAN_DEF_ORB_MOOD = 4,  // ORB_MOOD_LIVE[mood] — per-mood; applier
                          // configure_orbs, own flag g_orb_def_dirty
};

// ─── Cadence ──────────────────────────────────────────────────────────
// WHEN a stop sounds. CADENCE IS A PROPERTY OF A FACT'S AUTHORSHIP, so it
// is DERIVED and not hand-painted; GEN is the one bit the registry cannot
// infer about an entry, so GEN is the one bit stored. derived_cadence()
// below is the ONE place the rule lives, so the manifest emitter and every
// reader of it agree about what a row means.
enum : uint8_t {
    ORGAN_CAD_LIVE     = 0,   // the home is read where it is needed
    ORGAN_CAD_GEN      = 1,   // STORED — the author's next natural event
    ORGAN_CAD_BOUNDARY = 2,   // derived from def_kind / sentinel / block
    ORGAN_CAD_DRIVEN   = 3,   // derived from `ro`: the row is a meter
};

// ─── The entry ────────────────────────────────────────────────────────
// POD, so the table is a constant the linker can place in rodata.
//
// NO RESERVED COLUMNS: a column arrives with the campaign that fills it,
// which is also the only campaign that can say what shape it needs.
struct OrganParam {
    const char* id;
    const char* label;
    const char* group;
    uint8_t     block;
    uint16_t    offset;
    uint8_t     type;
    float       minv, maxv, step;
    uint8_t     def_kind;     // ORGAN_DEF_NONE | _MOOD | _TIER | _BEHAVIOR | _ORB_MOOD
    uint16_t    def_offset;   // byte offset into the kind's own struct, if any
    uint8_t     ro;           // a WITNESS, not a dial: organ_set refuses it
    uint8_t     cad;          // ORGAN_CAD_LIVE | _GEN — the one stored cadence
};

// ─── The enrollment macro ─────────────────────────────────────────────
// The id is "block.field" so it is stable across relabelling: export/import
// keys on it, and a label is prose that may be improved without breaking a
// saved file.
#define ORGAN_PARAM_NS(NS, BLOCK, STRUCT, FIELD, TYPE, MIN, MAX, STEP, GROUP, LABEL) \
    OrganParam{ #BLOCK "." #FIELD, LABEL, GROUP,                              \
                ORGAN_BLOCK_##BLOCK,                                          \
                (uint16_t)offsetof(NS::STRUCT, FIELD),       \
                ORGAN_##TYPE, MIN, MAX, STEP,                                  \
                ORGAN_DEF_NONE, 0, 0, ORGAN_CAD_LIVE },

// THE SAME LINE, DECLARED GENERATIONAL. Identical in every column but the
// last: the edit lands at the author's next natural event, not now and not
// at the boundary. A dial that edits the future says so WHERE THE HAND IS
// — at the row, which is what the chip this feeds is for.
#define ORGAN_PARAM_GEN_NS(NS, BLOCK, STRUCT, FIELD, TYPE, MIN, MAX, STEP, GROUP, LABEL) \
    OrganParam{ #BLOCK "." #FIELD, LABEL, GROUP,                              \
                ORGAN_BLOCK_##BLOCK,                                          \
                (uint16_t)offsetof(NS::STRUCT, FIELD),       \
                ORGAN_##TYPE, MIN, MAX, STEP,                                  \
                ORGAN_DEF_NONE, 0, 0, ORGAN_CAD_GEN },

// The same line plus the field the dial DEFINES, and the family that field
// belongs to. The compiler takes this offset too, so a rename on the
// definition side fails at the enrollment exactly as a rename on the
// instance side does. DEFSTRUCT is the family's struct, named by the
// caller, so a further family costs one enum value and nothing here.
#define ORGAN_PARAM_DEF_NS(NS, BLOCK, STRUCT, FIELD, TYPE, MIN, MAX, STEP, GROUP,    \
                        LABEL, DEFKIND, DEFSTRUCT, DEFFIELD)                  \
    OrganParam{ #BLOCK "." #FIELD, LABEL, GROUP,                              \
                ORGAN_BLOCK_##BLOCK,                                          \
                (uint16_t)offsetof(NS::STRUCT, FIELD),       \
                ORGAN_##TYPE, MIN, MAX, STEP,                                  \
                ORGAN_DEF_##DEFKIND,                                          \
                (uint16_t)offsetof(NS::DEFSTRUCT, DEFFIELD), 0,        \
                ORGAN_CAD_LIVE },

// A DEFINITION WITH NO INSTANCE. Some facts have a definition the panel
// may write and no home it may address: MoodProfile's clear_color is read
// by apply_mood_lighting into clearColor_, a cartridge member and not one
// of the three exposed homes. The write is always a definition, preview is
// refused, and the value shown is the live mood's meaning.

// THE SENTINEL IS DERIVED FROM THE KIND, not written at the call site: one
// mapping line per family, here, and a further family adds one #define.
#define ORGAN_DEFONLY_BLOCK_MOOD     ORGAN_BLOCK_NONE
#define ORGAN_DEFONLY_BLOCK_ORB_MOOD ORGAN_BLOCK_NONE_ORB

#define ORGAN_PARAM_DEFONLY_NS(NS, TYPE, MIN, MAX, STEP, GROUP, LABEL,               \
                            DEFKIND, DEFSTRUCT, DEFFIELD)                     \
    OrganParam{ #DEFSTRUCT "." #DEFFIELD, LABEL, GROUP,                       \
                ORGAN_DEFONLY_BLOCK_##DEFKIND,                                \
                (uint16_t)offsetof(NS::DEFSTRUCT, DEFFIELD), \
                ORGAN_##TYPE, MIN, MAX, STEP,                                  \
                ORGAN_DEF_##DEFKIND,                                          \
                (uint16_t)offsetof(NS::DEFSTRUCT, DEFFIELD), 0,        \
                ORGAN_CAD_LIVE },

// A WITNESS, NOT A DIAL. The same offsetof plumbing pointed at a DRIVEN
// value: the panel meters it and organ_set refuses to write it. No
// min/max/step, because a meter has no range to clamp against — the
// driver's own dials carry the ranges and enroll with ORGAN_PARAM above.
#define ORGAN_PARAM_RO_NS(NS, BLOCK, STRUCT, FIELD, TYPE, GROUP, LABEL)              \
    OrganParam{ #BLOCK "." #FIELD, LABEL, GROUP,                              \
                ORGAN_BLOCK_##BLOCK,                                          \
                (uint16_t)offsetof(NS::STRUCT, FIELD),       \
                ORGAN_##TYPE, 0.0f, 0.0f, 0.0f,                                \
                ORGAN_DEF_NONE, 0, 1, ORGAN_CAD_LIVE },

// ─── THE NAMESPACE PARAMETER, MADE INVISIBLE ──────────────────────────
// Every form above takes the enrolled struct's NAMESPACE first, because
// the canvas's banks live in `t7::canvas`, one tier BELOW the cartridge.
// The five forwards below are the price: a line that does not care about
// the namespace never mentions it; one that does writes _NS and says
// which.
#define ORGAN_PARAM(...)         ORGAN_PARAM_NS(the_board, __VA_ARGS__)
#define ORGAN_PARAM_GEN(...)     ORGAN_PARAM_GEN_NS(the_board, __VA_ARGS__)
#define ORGAN_PARAM_DEF(...)     ORGAN_PARAM_DEF_NS(the_board, __VA_ARGS__)
#define ORGAN_PARAM_DEFONLY(...) ORGAN_PARAM_DEFONLY_NS(the_board, __VA_ARGS__)
#define ORGAN_PARAM_RO(...)      ORGAN_PARAM_RO_NS(the_board, __VA_ARGS__)

inline const OrganParam kOrganParams[] = {
#include "console/organ_params.inc"
};
#undef ORGAN_PARAM
#undef ORGAN_PARAM_GEN
#undef ORGAN_PARAM_DEF
#undef ORGAN_PARAM_DEFONLY
#undef ORGAN_PARAM_RO
#undef ORGAN_PARAM_NS
#undef ORGAN_PARAM_GEN_NS
#undef ORGAN_PARAM_DEF_NS
#undef ORGAN_PARAM_DEFONLY_NS
#undef ORGAN_DEFONLY_BLOCK_MOOD
#undef ORGAN_DEFONLY_BLOCK_ORB_MOOD
#undef ORGAN_PARAM_RO_NS

inline constexpr size_t kOrganParamCount =
    sizeof(kOrganParams) / sizeof(kOrganParams[0]);

// ─── The live home ────────────────────────────────────────────────────
// Bound once at boot. Null until then, and every ABI entry point returns
// harmlessly on null — the panel's JS may be present on a page whose
// program has not finished booting.
inline the_board::GPUState* g_home = nullptr;
inline uint32_t g_rejected = 0;   // refused organ_set calls, shown in the panel

// A COUNT IS NOT A DIAGNOSIS. A bare count says a refusal happened and
// never which row or why, so every refusal also names its subject and its
// reason. One string, written at every refusal site, read by the status
// line.
inline std::string g_last_reject;
inline void note_reject(const char* id, const char* why) {
    ++g_rejected;
    g_last_reject.assign(id ? id : "(triple not in the manifest)");
    g_last_reject += " — ";
    g_last_reject += why;
}

// The live mood, BORROWED and never owned: the spine's own mood organ
// (contracts/spine_state.hpp), so the panel can never be looking at a mood
// the program has left. One pointer answers WHICH MOOD and WHICH REGIME
// the world was drawn into — two windows, one home, no copy.
inline const the_board::MoodState* g_mood = nullptr;

// The live HOST, borrowed the same way and for the same reason (RIBBON_1):
// the panel's host row must never name a host the program has left. The
// point's house is the spine's (contracts/point.hpp).
inline const the_board::PointState* g_point = nullptr;

inline void bind_home(the_board::GPUState* s) { g_home = s; }
inline void bind_mood(const the_board::MoodState* ms) { g_mood = ms; }
inline void bind_point(const the_board::PointState* p) { g_point = p; }
inline uint32_t current_mood()       { return g_mood ? g_mood->active     : 0u; }
inline uint32_t current_regime()     { return g_mood ? g_mood->regime     : 0u; }
inline uint32_t current_host()       { return g_point ? (uint32_t)g_point->host : 0u; }

inline void* block_base(uint8_t block) {
    if (!g_home) return nullptr;
    switch (block) {
    case ORGAN_BLOCK_CONFIG:     return g_home->organ_config_home();
    case ORGAN_BLOCK_LIGHTING:   return g_home->organ_lighting_home();
    case ORGAN_BLOCK_AGENT_ROOM: return g_home->organ_agent_room_home();
    case ORGAN_BLOCK_DRIVERS:    return &the_board::DRIVER_LIVE;
    case ORGAN_BLOCK_PAWN:       return &the_board::PAWN_AURA_LIVE;
    case ORGAN_BLOCK_ORBS:       return &the_board::ORB_CONSOLE_LIVE;
    case ORGAN_BLOCK_PANEL:      return &the_board::PANEL_LIVE;
    case ORGAN_BLOCK_RIBBON:     return &the_board::RIBBON_LIVE;
    case ORGAN_BLOCK_INDOOR:     return &the_board::INDOOR_LIVE;
    case ORGAN_BLOCK_CANVAS:     return &canvas::CANVAS_LIVE;
    case ORGAN_BLOCK_WORLD:      return &the_board::WORLD_DRAW_LIVE;
    case ORGAN_BLOCK_RIBBON_SPAWN: return &the_board::RIBBON_SPAWN_LIVE;
    case ORGAN_BLOCK_CONDUCTOR:  return &the_board::ORB_CONDUCTOR_LIVE;
    default:                     return nullptr;
    }
}

// A DEFINITION-ONLY ENTRY'S BLOCK. One helper rather than a literal at
// three call sites: the convention descends from 255 (see the block enum),
// so a further family adds one name here and none at a call site.
inline bool is_defonly(uint8_t block) {
    return block == ORGAN_BLOCK_NONE || block == ORGAN_BLOCK_NONE_ORB;
}

// DOES A WRITE TO THIS BLOCK RAISE A RE-SPEAK FLAG? A bank whose author is
// re-spoken at the frame boundary gives its dials BOUNDARY cadence, and
// that is a property of the BLOCK rather than of the entry — so it is
// stated once, here, rather than per line.
inline bool block_has_boundary(uint8_t block) {
    // The orb console's only reader is configure_orbs — the orb MOOD
    // bank's author too — so a write to block 5 is consumed at the frame
    // boundary rather than where it lands. THIS ANSWERS WHEN, NOT WHAT:
    // what the boundary DOES per field rides g_orb_console_dirty, not this
    // predicate, because a cadence question and a plumbing question are
    // two questions.
    return block == ORGAN_BLOCK_ORBS;
}

// THE ONE PLACE THE CADENCE RULE LIVES. Every reader of a row's cadence
// calls this, so none can disagree about what a row means. Order matters:
// a witness is DRIVEN even if its block has a boundary, because the row is
// a meter and a meter's cadence is its author's.
inline uint8_t derived_cadence(const OrganParam& e) {
    if (e.ro) return ORGAN_CAD_DRIVEN;
    if (e.def_kind != ORGAN_DEF_NONE || is_defonly(e.block)
        || block_has_boundary(e.block)) return ORGAN_CAD_BOUNDARY;
    return e.cad;                       // LIVE or the stored GEN
}

// ─── THE SHELL'S TWO QUESTIONS, DERIVED HERE ──────────────────────────
// A RULE RESTATED IN A SECOND LANGUAGE IS A RULE WITH TWO HOMES, and the
// copy is the one that drifts (L46).
// THE SHELL MUST NOT KNOW A BLOCK NUMBER OR A KIND NUMBER: it asks two
// questions, the manifest answers them, and a further definition family
// answers here while the shell learns nothing.

// May the panel address this row's INSTANCE? A preview write needs one;
// a definition-only row has none, so it targets the live mood whatever
// the mode toggle says.
inline uint8_t derived_has_instance(const OrganParam& e) {
    return is_defonly(e.block) ? 0u : 1u;
}

// How a DEFINITION is addressed — the export's keying and the panel's
// follow-the-mood refresh both turn on this and on nothing else.
enum : uint8_t {
    ORGAN_SCOPE_NONE  = 0,   // no definition behind this row
    ORGAN_SCOPE_MOOD  = 1,   // one row per mood: the write's target picks it
    ORGAN_SCOPE_WORLD = 2,   // one bank for the world: the target is ignored
};
inline uint8_t derived_scope(const OrganParam& e) {
    switch (e.def_kind) {
    case ORGAN_DEF_MOOD:
    case ORGAN_DEF_ORB_MOOD: return ORGAN_SCOPE_MOOD;
    case ORGAN_DEF_TIER:
    case ORGAN_DEF_BEHAVIOR: return ORGAN_SCOPE_WORLD;
    default:                 return ORGAN_SCOPE_NONE;
    }
}

// THE WHITELIST LOOKUP. A triple that is not an entry is not addressable,
// full stop — this is what keeps organ_set from being a memory editor.
inline const OrganParam* find_entry(int block, int offset, int type) {
    for (size_t i = 0; i < kOrganParamCount; ++i) {
        const OrganParam& e = kOrganParams[i];
        if (e.block == block && e.offset == offset && e.type == type)
            return &e;
    }
    return nullptr;
}

// Declared here because read_lane reaches for it: a definition-only entry
// has no instance, so reading its value IS reading its definition. The
// body stays beside the rest of the definition path, below.
inline float read_definition(const OrganParam& e, uint32_t mood, int lane);

inline float read_lane(const OrganParam& e, int lane) {
    // A definition-only entry has no instance to read, so its "value" is
    // the LIVE mood's definition: the manifest and any meter show what the
    // current mood means.
    if (is_defonly(e.block))
        return read_definition(e, current_mood(), lane);
    void* base = block_base(e.block);
    if (!base || lane < 0 || lane >= lanes_of(e.type)) return 0.0f;
    const char* p = static_cast<const char*>(base) + e.offset;
    if (e.type == ORGAN_U32 || e.type == ORGAN_BOOL) {
        uint32_t v = 0;
        std::memcpy(&v, p, sizeof(v));
        return static_cast<float>(v);
    }
    float v = 0.0f;
    std::memcpy(&v, p + lane * sizeof(float), sizeof(float));
    return v;
}

// ─── THE DEFINITION WRITE PATH ────────────────────────────────────────
// Writing a definition does NOT write the instance: the panel says what
// the mood MEANS and the mood's own apply turns that into an instance, so
// the panel stays a VIEW. The re-apply is deferred to the frame boundary
// for the flush's own reason — a drag is many events, the mood is applied
// once — and the cartridge takes the flag, owning the deps and the queue.
inline bool     g_def_dirty = false;
inline uint32_t g_def_dirty_mood = 0;
inline bool     g_tier_def_dirty = false;   // the world bank changed
inline bool     g_orb_def_dirty  = false;   // the orb mood bank changed

// AND WHICH MOOD IT MEANT. Both mood-selected kinds record the target, so
// the boundary drops a write aimed at a mood the program has left rather
// than re-speaking the live one with it. ONE SLOT, AND ITS LIMIT SAID
// PLAINLY: two moods written between one boundary and the next leave the
// last one's id here — reachable through a multi-mood preset import, where
// the guard makes the case SAFE (dropped) rather than wrong.
inline uint32_t g_orb_def_dirty_mood = 0;
inline void raise_orb_definition(uint32_t mood) {
    g_orb_def_dirty = true;
    g_orb_def_dirty_mood = mood;
}

// ─── THE TOUCHED MASK ─────────────────────────────────────────────
// WHICH FIELDS the orb bank's writes touched since the boundary last
// looked (A BIT IS AN OFFSET / 4, into OrbMoodConfig). The FLAG says THAT
// the bank changed, the MASK says WHAT, and the boundary decides how much
// re-speak the edit requires. A RAISE WITH NO BITS MEANS EVERYTHING — what
// door RESPEAK promises, and the answer for a caller that does not say.
inline uint32_t g_orb_def_touched = 0;
inline uint32_t take_orb_def_touched() {
    const uint32_t m = g_orb_def_touched;
    g_orb_def_touched = 0;
    return m;
}

// THE CLASSIFICATION LIVES HERE, NOT AT THE BOUNDARY. Four of the nineteen
// orb-mood facts are baked into orb_state by the init kernel — `enabled`,
// `count`, `drag` and `palette_id` — so touching any of them re-seeds the
// sky; the other fifteen are per-frame GPU reads the uniform upload
// carries. It sits beside the mask because the bit convention is defined
// here and nowhere else.
inline constexpr uint32_t ORB_RESEED_BITS =
      (1u << (offsetof(the_board::OrbMoodConfig, enabled)    / 4u))
    | (1u << (offsetof(the_board::OrbMoodConfig, count)      / 4u))
    | (1u << (offsetof(the_board::OrbMoodConfig, palette_id) / 4u))
    | (1u << (offsetof(the_board::OrbMoodConfig, drag)       / 4u));
static_assert(ORB_RESEED_BITS == 0x00001023u,
    "the reseed set is enabled 0 · count 1 · drag 5 · palette_id 12. "
    "A field reordered in OrbMoodConfig fails the BUILD "
    "here rather than teaching the boundary to re-seed on the wrong dial");
static_assert(sizeof(the_board::OrbMoodConfig) / 4u <= 32u,
    "the touched mask is a uint32: every field's offset/4 must be a bit "
    "it can hold. 27 words today, five to spare");

// ─── THE CONSOLE MASK ─────────────────────────────────────────────
// A CPU bank whose reader is an event gets a per-field mask: the boundary
// routes each field to its reader's cadence — dome, noise and speed are
// per-frame GPU reads and take targeted partials; base size is baked at
// init and raises the orb re-speak. A BIT IS AN OFFSET / 4.
inline uint32_t g_orb_console_dirty = 0;   // bit = offsetof/4
inline uint32_t take_orb_console_dirty() {
    const uint32_t m = g_orb_console_dirty;
    g_orb_console_dirty = 0;
    return m;
}
// The bits the cartridge boundary reads, proved here rather than trusted
// there: a field reordered in OrbConsole fails the BUILD at this line
// instead of routing a dome radius into the noise floor.
static_assert(offsetof(the_board::OrbConsole, dome_radius) == 0
           && offsetof(the_board::OrbConsole, base_size)   == 4
           && offsetof(the_board::OrbConsole, noise_floor) == 8
           && offsetof(the_board::OrbConsole, speed_mult)  == 12,
    "the console mask's bits are offset/4 — dome 0, base size 1, noise 2, "
    "speed mult 3; the cartridge boundary routes on exactly those four");

// One base per definition family. MOOD selects by target; TIER is the
// world's single bank and ignores it.
inline char* definition_base(const OrganParam& e, uint32_t mood) {
    switch (e.def_kind) {
    case ORGAN_DEF_MOOD: return reinterpret_cast<char*>(&the_board::mood_def(mood));
    case ORGAN_DEF_TIER: return reinterpret_cast<char*>(&the_board::TIER_LIVE);
    case ORGAN_DEF_BEHAVIOR: return reinterpret_cast<char*>(&the_board::BEHAVIOR_LIVE);
    case ORGAN_DEF_ORB_MOOD:
        return reinterpret_cast<char*>(
            &the_board::ORB_MOOD_LIVE[mood % the_board::MOOD_COUNT]);
    default:             return nullptr;
    }
}

inline bool write_definition(const OrganParam& e, uint32_t mood, const float* in) {
    if (e.def_kind == ORGAN_DEF_NONE) return false;

    char* p = definition_base(e, mood);
    if (!p) return false;
    p += e.def_offset;

    // A U32 OR BOOL DEFINITION CONVERTS; IT DOES NOT REINTERPRET — the
    // same conversion organ_set's instance path does, applied on the
    // definition side. Every MoodProfile target is a float run; the orb
    // bank is what needs this, because `count`, `enabled` and the three id
    // choices are the sky's most useful dials.
    if (e.type == ORGAN_U32 || e.type == ORGAN_BOOL) {
        uint32_t v = (uint32_t)(in[0] < 0.0f ? 0.0f : in[0]);
        std::memcpy(p, &v, sizeof(v));
    } else {
        const int n = lanes_of(e.type);
        for (int l = 0; l < n; ++l) {
            float f = in[l];
            if (f < e.minv) f = e.minv;
            if (f > e.maxv) f = e.maxv;
            std::memcpy(p + l * sizeof(float), &f, sizeof(float));
        }
    }
    // TIER and BEHAVIOR share one author — upload_agent_registries_to_gpu
    // reads both banks — so they share one flag and one boundary re-speak:
    // a second flag would be a second name for one occasion.
    if (e.def_kind == ORGAN_DEF_TIER || e.def_kind == ORGAN_DEF_BEHAVIOR) {
        g_tier_def_dirty = true;
    } else if (e.def_kind == ORGAN_DEF_ORB_MOOD) {
        // Its own author, so its own flag — the converse of BEHAVIOR's
        // case, and the same rule: the flag names the occasion.
        raise_orb_definition(mood);
        // And WHICH field, so the boundary re-speaks no more than the
        // edit requires. `def_offset` and not `offset`: the write above
        // lands at `p + e.def_offset`, so the bit names the same word. The
        // two are equal for a definition-only row and differ for a
        // ORGAN_PARAM_DEF row whose instance lives elsewhere.
        g_orb_def_touched |= (1u << (e.def_offset / 4u));
    } else {
        g_def_dirty = true; g_def_dirty_mood = mood;
    }
    return true;
}

inline float read_definition(const OrganParam& e, uint32_t mood, int lane) {
    if (e.def_kind == ORGAN_DEF_NONE || lane < 0 || lane >= lanes_of(e.type))
        return 0.0f;
    const char* p = definition_base(e, mood);
    if (!p) return 0.0f;
    p += e.def_offset;
    // The read mirrors the write, and read_lane's instance branch, exactly:
    // an integer definition is CONVERTED back, never reinterpreted.
    if (e.type == ORGAN_U32 || e.type == ORGAN_BOOL) {
        uint32_t u = 0;
        std::memcpy(&u, p, sizeof(u));
        return static_cast<float>(u);
    }
    float v = 0.0f;
    std::memcpy(&v, p + lane * sizeof(float), sizeof(float));
    return v;
}

// Taken once, by the frame boundary. Returns false when there is nothing
// to re-apply, so the caller pays a branch on a quiet frame.
inline bool take_definition_dirty(uint32_t& mood) {
    if (!g_def_dirty) return false;
    g_def_dirty = false;
    mood = g_def_dirty_mood;
    return true;
}

// ─── DOORS ────────────────────────────────────────────────────────────
// A DOOR RAISES A FLAG THE BOUNDARY ALREADY CONSUMES AND ADDS NO AUTHOR,
// which is why it does not violate the sovereignty boundary. The table
// below carries ids and labels ONLY: behavior lives in the cartridge,
// where the deps are, and this file knows neither the mood state nor the
// queue.

// A DOOR IS ALSO WHERE A PLAYER-OWNED FACT BELONGS. The orb rule and the
// flock gesture are the sky's two — the mood seeds each once and the
// player wins after, so neither can honestly wear a dial. Each door calls
// the function its key calls: cycle_orb_motion_rule / cycle_orb_gesture,
// the commands KP_8 and KP_7 press.
enum : uint32_t {
    ORGAN_DOOR_RESPEAK     = 0,   // raise every definition flag at once
    ORGAN_DOOR_ORB_RULE    = 1,   // cycle the sky's motion rule (player-owned)
    ORGAN_DOOR_ORB_GESTURE = 2,   // cycle the active rule's gesture
    ORGAN_DOOR_COUNT       = 3,
};

struct OrganDoor { uint32_t id; const char* label; };

inline constexpr OrganDoor kOrganDoors[] = {
    { ORGAN_DOOR_RESPEAK,     "Re-speak definitions" },
    { ORGAN_DOOR_ORB_RULE,    "Cycle orb rule" },
    { ORGAN_DOOR_ORB_GESTURE, "Cycle orb gesture" },
};
static_assert(sizeof(kOrganDoors) / sizeof(kOrganDoors[0]) == ORGAN_DOOR_COUNT,
    "one row per door id — the manifest emits this table and the shell "
    "renders one button per row, so a missing row is a missing button");

// A BITMASK, so presses coalesce by construction: three clicks between two
// frame boundaries are one raise, as a slider drag is one WriteBuffer.
inline uint32_t g_doors_pending = 0;

inline uint32_t take_doors_pending() {
    const uint32_t m = g_doors_pending;
    g_doors_pending = 0;
    return m;
}

// ─── THE MOOD DOOR ───────────────────────────────────────────────
// A door with a parameter: WHICH mood. The shell's select asks the program
// to go somewhere; the frame boundary presses request_mood_transition —
// the same door keys 5-9 and every portal press walk, with its own guards
// (a transition in flight ignores the press). One pending id, last press
// wins, taken once; MOOD_COUNT is "no request" and no mood has that id.
inline uint32_t g_go_mood_pending = the_board::MOOD_COUNT;
inline bool take_go_mood(uint32_t& mood) {
    if (g_go_mood_pending >= the_board::MOOD_COUNT) return false;
    mood = g_go_mood_pending;
    g_go_mood_pending = the_board::MOOD_COUNT;
    return true;
}

// ─── THE HOST DOOR (RIBBON_1) ────────────────────────────────────
// The same shape as the mood door: a door with a parameter. The shell asks
// the program to hand the point to a host; the frame boundary presses
// possess() — the ONE transaction key R also presses, with its own guards
// (no ribbon to ride is a refusal, not a crash). One pending id, last press
// wins, taken once; HOST_NONE is "no request" and no host has that id.
inline constexpr uint32_t HOST_NONE = 0xFFFFFFFFu;
inline uint32_t g_go_host_pending = HOST_NONE;
inline bool take_go_host(uint32_t& host) {
    if (g_go_host_pending == HOST_NONE) return false;
    host = g_go_host_pending;
    g_go_host_pending = HOST_NONE;
    return true;
}

// The tier bank's re-apply, taken once by the frame boundary (the
// cartridge, which owns the agents' deps and the queue — this file
// knows neither).
inline bool take_tier_definition_dirty() {
    if (!g_tier_def_dirty) return false;
    g_tier_def_dirty = false;
    return true;
}

// The orb mood bank's re-apply, taken once by the frame boundary. Its
// applier is configure_orbs, which the cartridge can reach and this file
// cannot.
inline bool take_orb_definition_dirty(uint32_t& mood) {
    if (!g_orb_def_dirty) return false;
    g_orb_def_dirty = false;
    mood = g_orb_def_dirty_mood;
    return true;
}

// ─── THE RULE WINDOW ──────────────────────────────────────────────
// A DIAL WHOSE EFFECT DEPENDS ON A MODE STANDS NEXT TO A TRUTHFUL READOUT
// OF THAT MODE. Fifteen orb rows are rule-scoped, so the panel says which
// rule is in force and a dormant row reads as dormant, not dead.

// A WINDOW, NOT A HOME: `OrbsState.current_motion_rule` and
// `.gesture_idx[]` are the only truth. The rule lives in `OrbsState`, a
// body this header may not include, so the window is a copy the cartridge
// writes. PACKED into one uint32 and one ABI call, so no second call can
// fall out of step with the first.

//   bits  0..7   motion rule
//   bits  8..15  the active rule's gesture
//   bit  16      os.active — the dome is lit
//   bits 17..25  os.count  — motes (0…511; Dim::MAX_ORBS is 256)
inline uint32_t g_orb_rule_view = 0;
inline void set_orb_rule_view(uint32_t rule, uint32_t gesture,
                              bool active, uint32_t count) {
    g_orb_rule_view = (rule & 0xFFu)
                    | ((gesture & 0xFFu) << 8)
                    | ((active ? 1u : 0u) << 16)
                    | ((count & 0x1FFu) << 17);
}

} // namespace organ
} // namespace t7

// ═══ THE C ABI ═══════════════════════════════════════════════════════
// extern "C" so ccall/cwrap can reach it by name; KEEPALIVE so the linker
// does not garbage-collect a function no C++ caller has.
#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE
#endif

extern "C" {

// Built lazily, once, into a static string whose c_str outlives the call.
// Carries the CURRENT value of every entry, so the panel opens showing the
// program rather than showing its own defaults — a VIEW, per the charter.
EMSCRIPTEN_KEEPALIVE inline const char* organ_manifest(void) {
    using namespace t7::organ;
    static std::string json;
    json.clear();
    json.reserve(kOrganParamCount * 220 + 32);
    json.push_back('[');
    char buf[512];
    for (size_t i = 0; i < kOrganParamCount; ++i) {
        const OrganParam& e = kOrganParams[i];
        if (i) json.push_back(',');
        std::snprintf(buf, sizeof buf,
            "{\"id\":\"%s\",\"label\":\"%s\",\"group\":\"%s\",\"block\":%u,"
            "\"offset\":%u,\"type\":%u,\"min\":%g,\"max\":%g,\"step\":%g,"
            "\"def\":%u,\"scope\":%u,\"inst\":%u,"
            "\"ro\":%u,\"cad\":%u,\"v\":[",
            e.id, e.label, e.group, (unsigned)e.block, (unsigned)e.offset,
            (unsigned)e.type, e.minv, e.maxv, e.step,
            (unsigned)e.def_kind,
            (unsigned)derived_scope(e),        // derived, not stored
            (unsigned)derived_has_instance(e), // derived, not stored
            (unsigned)(e.ro ? 1u : 0u),
            (unsigned)derived_cadence(e));   // derived, not stored
        json += buf;
        const int n = lanes_of(e.type);
        for (int l = 0; l < n; ++l) {
            std::snprintf(buf, sizeof buf, "%s%g", l ? "," : "", read_lane(e, l));
            json += buf;
        }
        json += "]}";
    }
    json.push_back(']');
    return json.c_str();
}

// The door roster, emitted separately from the dial manifest so a shell
// that carries no door bar keeps working.
EMSCRIPTEN_KEEPALIVE inline const char* organ_doors(void) {
    using namespace t7::organ;
    static std::string json;
    json.clear();
    json.push_back('[');
    char buf[256];
    for (size_t i = 0; i < ORGAN_DOOR_COUNT; ++i) {
        if (i) json.push_back(',');
        std::snprintf(buf, sizeof buf, "{\"i\":%u,\"l\":\"%s\"}",
                      kOrganDoors[i].id, kOrganDoors[i].label);
        json += buf;
    }
    json.push_back(']');
    return json.c_str();
}

// Writes the member and sets the block's dirty bit. It does NOT upload:
// the flush is once a frame at the boundary, so a slider drag is many of
// these calls and one WriteBuffer (docs/ORGAN.md, "The write path").

// TARGET. -1 is PREVIEW: write the instance, which the program's other
// authors may take back. A mood id (0..MOOD_COUNT-1) is DEFINITION: write
// what that mood MEANS and let its own apply produce the instance.
// Definition is the default; a dial with no definition target falls back
// to the instance under either mode.
EMSCRIPTEN_KEEPALIVE inline void organ_set(int block, int offset, int type,
                                           float x, float y, float z, float w,
                                           int target) {
    using namespace t7::organ;
    const OrganParam* e = find_entry(block, offset, type);
    if (!e)    { note_reject(nullptr, "not in the manifest"); return; }
    if (e->ro) { note_reject(e->id, "a witness, not a dial"); return; }
    if (is_defonly(e->block)) {            // definition-only:
        const float lanes_only[4] = { x, y, z, w };
        if (target < 0 || !write_definition(*e, (uint32_t)target, lanes_only))
            note_reject(e->id, target < 0                  // no instance to fall back to
                ? "preview on a definition-only row — there is no instance to show"
                : "the definition write did not land");
        return;
    }
    void* base = block_base((uint8_t)block);
    if (!base) { note_reject(e->id, "the block has no home"); return; }

    const float lanes_in[4] = { x, y, z, w };
    if (target >= 0 && write_definition(*e, (uint32_t)target, lanes_in)) {
        // Deliberately no instance write and no dirty bit: the instance is
        // the applier's to produce.
        return;
    }

    char* p = static_cast<char*>(base) + e->offset;
    if (type == ORGAN_U32 || type == ORGAN_BOOL) {
        uint32_t v = (uint32_t)(x < 0.0f ? 0.0f : x);
        std::memcpy(p, &v, sizeof(v));
    } else {
        const int n = lanes_of((uint8_t)type);
        for (int l = 0; l < n; ++l) {
            float v = lanes_in[l];
            if (v < e->minv) v = e->minv;
            if (v > e->maxv) v = e->maxv;
            std::memcpy(p + l * sizeof(float), &v, sizeof(float));
        }
    }
    g_home->organ_mark_dirty((uint32_t)block);
    // A BLOCK WITH A BOUNDARY RAISES ITS AUTHOR'S FLAG, PER FIELD. The orb
    // console's only reader is configure_orbs, which is also the orb mood
    // bank's applier — one author, one flag, the rule BEHAVIOR follows
    // against TIER. The raise is per FIELD so the boundary can decide what
    // each costs. The hook lives at this one site, after the clamp and the
    // write succeed, keyed on the block, and never in the shell.
    if (block == ORGAN_BLOCK_ORBS)
        g_orb_console_dirty |= (1u << (e->offset / 4u));
}

// BY INDEX, LIKE ITS SIBLINGS. The manifest index IS the index in
// kOrganParams, because the manifest is emitted in table order — so the
// shell reads a value with the index it is already holding rather than
// spelling a home's coordinates. A row is identified by its (block,
// offset, TYPE) triple, and two rows may share a pair.
EMSCRIPTEN_KEEPALIVE inline float organ_get(int index, int lane) {
    using namespace t7::organ;
    if (index < 0 || (size_t)index >= kOrganParamCount) return 0.0f;
    return read_lane(kOrganParams[index], lane);
}

// The panel's own witnesses, read once per frame by its status line.
EMSCRIPTEN_KEEPALIVE inline int organ_rejected_count(void) {
    return (int)t7::organ::g_rejected;
}
// The last refusal, in words. Empty until one happens.
EMSCRIPTEN_KEEPALIVE inline const char* organ_last_reject(void) {
    return t7::organ::g_last_reject.c_str();
}
// Blocks the panel's edits RECONCILED on the last frame boundary — not
// blocks written at that boundary. config_ counts here and uploads in the
// spine, which is why the panel labels this "reconciled".
EMSCRIPTEN_KEEPALIVE inline int organ_flush_count(void) {
    return t7::organ::g_home
         ? (int)t7::organ::g_home->organ_last_flush_count() : 0;
}
// PURSE_0 R-D — THE COUNT IS THE READINESS GATE, AND IT WAS NOT ONE.
//
// organ_panel.js polls `if (C.count() <= 0) return;  // registry not bound
// yet` and clears its interval the moment this answers. It returned
// kOrganParamCount — a COMPILE-TIME CONSTANT — so it was never zero and
// the comment asserted an invariant this side did not honour.
//
// WHAT THAT COSTS, and it is not theoretical on a slow machine.
// organ_manifest() does not gate either, and read_lane returns 0.0f on a
// null base rather than failing, so the manifest parses, the poll stops,
// and the panel opens — against its own charter, showing zeros instead of
// the program. Worse, `?preset=<name>` walks the same road at boot
// (deliberately, so an exhibition needs no panel): every CONFIG write then
// lands in organ_set's `if (!base) note_reject(e->id, "the block has no
// home")` while the console still prints `N applied`. A scene that
// silently does not apply, reported as applied.
//
// The window is real: bind_home runs after device creation, and this
// tree's own console has observed device creation at 70,459 and 205,527
// ms. The panel's first poll is at 500.
//
// So the count now MEANS what the panel already believed it meant. The
// static table is still the count; what changed is that it is withheld
// until the ABI it describes can actually be written through.
EMSCRIPTEN_KEEPALIVE inline int organ_param_count(void) {
    if (!t7::organ::g_home) return 0;
    return (int)t7::organ::kOrganParamCount;
}

// PURSE_0 R2 — WHICH TREE IS THIS? The panel's status line already names
// the ARTIFACT (window.T7_BUILD_ID, the digest web_dist bakes into the
// shell). This hands it the TREE, from the C++ that was compiled, so the
// two provenance facts sit in one line and neither can be read without
// the other.
//
// IT IS NOT A DIAL AND CANNOT BE ONE. Adding a dial is one line in
// organ_params.inc and no JS edit — but that road carries F32/U32/BOOL
// LANES, and a stamp is a string. So it rides the ABI as its own export,
// which is why this one costs a cwrap and a line of JS and the dials do
// not. The shell gate is the witness that both halves moved together.
EMSCRIPTEN_KEEPALIVE inline const char* organ_build_stamp(void) {
    return t7::BUILD_STAMP;
}


// The mood the program is in. The panel needs it to address a definition
// and to key an export, and it keeps no copy of its own.
EMSCRIPTEN_KEEPALIVE inline int organ_mood(void) {
    return (int)t7::organ::current_mood();
}

// The regime the live world was drawn into: the Atmosphere.regime[] INDEX
// (0-based; the shell shows it as the label's number). Read through the
// same borrowed pointer as organ_mood(), so the panel's regime lines can
// never name a regime the draw has left. The seed drew it and RESPEAK
// keeps the seed; only a weight dial moves it without a transition.
EMSCRIPTEN_KEEPALIVE inline int organ_regime(void) {
    return (int)t7::organ::current_regime();
}

// The sky's live motion rule, packed with the ACTIVE rule's gesture index,
// whether the dome is lit and how many motes it carries. set_orb_rule_view
// states the bit layout; this is its one reader and does not restate it.
// The panel reads it to say WHICH MODE the fifteen rule-scoped orb rows
// are acting in, and whether there is a sky to act on. Zero before the
// first configure reads as brownian/0 — what the program seeds to.
EMSCRIPTEN_KEEPALIVE inline int organ_orb_rule(void) {
    return (int)t7::organ::g_orb_rule_view;
}

// Press a door. Out-of-range ids are ignored rather than counted as
// rejections: a rejection means the panel asked for something the manifest
// forbids, and a door id the build does not carry is a stale shell.
EMSCRIPTEN_KEEPALIVE inline void organ_door(uint32_t id) {
    using namespace t7::organ;
    if (id < ORGAN_DOOR_COUNT) g_doors_pending |= (1u << id);
}

// Ask the program to enter a mood by id; organ_mood_names gives the
// labels. Out of range is ignored, for organ_door's own reason.
EMSCRIPTEN_KEEPALIVE inline void organ_go_mood(int mood) {
    using namespace t7::organ;
    if (mood >= 0 && (uint32_t)mood < t7::the_board::MOOD_COUNT)
        g_go_mood_pending = (uint32_t)mood;
}

// Which host the point is on: 0 pawn, 1 camera, 2 ribbon. The panel's host
// row reads it so the row can never show a host the program has left —
// through the same borrowed pointer organ_mood() reads the mood through.
EMSCRIPTEN_KEEPALIVE inline int organ_host(void) {
    return (int)t7::organ::current_host();
}

// Ask the program to hand the point to a host by id. Out of range is
// ignored, for organ_door's own reason.
EMSCRIPTEN_KEEPALIVE inline void organ_go_host(int host) {
    using namespace t7::organ;
    if (host >= 0 && host <= 2) g_go_host_pending = (uint32_t)host;
}

// The names, positional by id: a JSON array the shell builds its mood
// select from, so a new mood appears there with no JS edit.
EMSCRIPTEN_KEEPALIVE inline const char* organ_mood_names(void) {
    static std::string json;
    json.clear();
    json.push_back('[');
    for (uint32_t m = 0; m < t7::the_board::MOOD_COUNT; ++m) {
        if (m) json.push_back(',');
        json.push_back('"');
        json += t7::the_board::MOOD_NAMES[m];
        json.push_back('"');
    }
    json.push_back(']');
    return json.c_str();
}

// One lane of one dial's DEFINITION for one mood. Zero for a dial with no
// definition target; the panel asks the manifest's "def" before this.
EMSCRIPTEN_KEEPALIVE inline float organ_def_get(int index, int mood, int lane) {
    using namespace t7::organ;
    if (index < 0 || (size_t)index >= kOrganParamCount || mood < 0) return 0.0f;
    return read_definition(kOrganParams[index], (uint32_t)mood, lane);
}

} // extern "C"
