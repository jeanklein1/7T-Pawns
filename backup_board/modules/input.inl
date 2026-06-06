// ─── input.inl ──────────────────────────────────────────────────
//
// The home of input dispatch. Keyboard, mouse, scroll. Routes raw
// GLFW events to module-owned commands.
//
// ┌─── Public surface (called from outside this file) ──────────────┐
// │                                                                  │
// │  GLFW callbacks (the spine routes raw events here):              │
// │    on_key_down(key)                                              │
// │    on_key_up(key)                                                │
// │    on_mouse_move(dx, dy)                                         │
// │    on_mouse_button(button, pressed)                              │
// │    on_scroll(delta)                                              │
// │                                                                  │
// │  Per-frame:                                                      │
// │    update_movement_intent()    — recomputes (move_x, move_z)     │
// │    clear_input_deltas()        — zeroes mouse deltas at frame end│
// │                                                                  │
// │  Camera/view commands (driven by key dispatch):                  │
// │    toggle_fpv_mode()                                             │
// │    set_render_radius(r)                                          │
// │                                                                  │
// └──────────────────────────────────────────────────────────────────┘
//
// ── A note on bindings (READ ME if changing keys) ─────────────────
//
// All key bindings in this file are temporary scaffolding. The
// program is being developed as a synth for visuals: the goal is for
// the functions invoked here to be drivable by music, automation, or
// remappable input vocabularies — not by these specific keys.
//
// What's durable: the *functions* in the registry below
//   (try_possess_nearest, toggle_mmode, cycle_orb_palette, ...).
//   Other modules' Public surfaces document those functions as their
//   permanent entry points.
//
// What's temporary: every key code in the registry. Bindings will
//   be remapped, replaced with MIDI control changes, or driven by
//   the analysis layer in time.
//
// One inherited constraint worth knowing: A–Z keys are claimed by
// the analysis layer above the cartridge as MIDI piano notes (per
// the boot banner: "A-Z=piano keys"). Letter keys cannot double as
// cartridge diagnostics without playing notes every time you cycle
// a behavior. That's why the diagnostic surface uses function keys.
//
// ──────────────────────────────────────────────────────────────────
//
// Included inside the Cartridge class body (private section).
// Depends on: musical.inl (toggle_mmode, MMODE_*), pawn.inl
//             (pawn_state_.aura_enabled, pawn_state_.aura_height_enabled, pawn_state_.aura_cfg_dirty),
//             mood.inl (request_mood_transition, MOOD_*),
//             agents.inl (try_possess_nearest, cycle_agent_*,
//             force_respawn_population), cube_behaviors.inl
//             (cycle_cube_behavior_override, corral_cubes,
//             toggle_cube_kite_mode), floater_vocabulary.inl
//             (cycle_floater_coordination), orbs.inl (cycle_orb_*,
//             toggle_orb_anchor).
// ─────────────────────────────────────────────────────────────────


// ═══ KEY BINDING REGISTRY ════════════════════════════════════════
//
// What every key does, organized by purpose. Bindings are temporary
// (see the note in the header); functions are durable. When changing
// what a key does, change both this table and the dispatch below.
//
// ── Movement (held; arrow keys) ──────────────────────────────────
//   ↑              keys_.forward                — move pawn forward
//   ↓              keys_.backward               — move pawn backward
//   ←              keys_.left                   — strafe pawn left
//   →              keys_.right                  — strafe pawn right
//
// ── World / aura toggles ─────────────────────────────────────────
//   1              gpuState_.toggle_freeze_sphere() — freeze chase sphere
//   2              pawn_state_.aura_height_enabled flip      — terrain extrusion on/off
//   3              pawn_state_.aura_enabled flip            — aura field on/off
//   5              request_mood_transition(MOOD_OPEN_SUNSET)
//   6              request_mood_transition(MOOD_INDOOR_FLAT)
//   7              request_mood_transition(MOOD_INDOOR_VAULT)
//   8              request_mood_transition(MOOD_FINITE_OUTDOOR)
//   9              request_mood_transition(MOOD_FINITE_OUTDOOR_REF)
//   0              cycle_orb_palette()          — next orb palette
//   [              set_render_radius(-1)        — smaller patch ring
//   ]              set_render_radius(+1)        — larger patch ring
//
// ── Musical modes (numpad) ───────────────────────────────────────
//   KP_1           toggle_mmode(MMODE_TERRAIN_WAVES)
//   KP_2           toggle_mmode(MMODE_COLOR_SHIFT)
//   KP_3           toggle_mmode(MMODE_CHECKER_SCATTER)
//   KP_4           toggle_mmode(MMODE_PALETTE_DRIFT)
//   KP_5           toggle_mmode(MMODE_GOL_TEMPO)
//   KP_6           toggle_mmode(MMODE_AURA_EXPAND)
//   KP_7           toggle_mmode(MMODE_RADIAL_PULSE)
//   KP_8           cycle_orb_motion_rule()      — next orb motion rule
//   KP_9           toggle_orb_anchor()          — orb anchor on/off
//   KP_DECIMAL     cycle_orb_gesture()          — next gesture per rule
//
// ── Camera / possession ──────────────────────────────────────────
//   L_CTRL/R_CTRL  toggle_fpv_mode()            — orbit ↔ first-person
//   CAPS_LOCK      try_possess_nearest()        — possess nearest agent
//
// ── Diagnostics (function keys) ──────────────────────────────────
//   F1             cycle_agent_behavior_override   none → random_walk → ...
//   F2             cycle_agent_tier_override       none → worker → ...
//   F3             force_respawn_population        repopulate evicted agents
//   F4             cycle_cube_behavior_override    stationary → curlfield → phasewave
//   F5             cycle_floater_coordination      0.0 → 0.5 → 1.0
//   F6             corral_cubes                    teleport cubes around pawn
//   F7             toggle_cube_kite_mode           cubes follow pawn on/off
//
// ── Mouse / scroll ───────────────────────────────────────────────
//   LMB drag       look_az_delta, look_el_delta
//   RMB drag       pan_x_delta, pan_y_delta
//   scroll         zoom_delta
// ─────────────────────────────────────────────────────────────────


// ═══ GLFW KEY CODE FALLBACKS ═════════════════════════════════════
//
// Some GLFW header configurations don't expose all of these key
// constants by default. Defining them here as fallbacks lets the
// switch below compile regardless of which GLFW variant the host
// project ships. Values match the GLFW 3.x canonical numbering.

#ifndef GLFW_KEY_KP_0
#define GLFW_KEY_KP_0  320
#endif
#ifndef GLFW_KEY_KP_1
#define GLFW_KEY_KP_1  321
#endif
#ifndef GLFW_KEY_KP_2
#define GLFW_KEY_KP_2  322
#endif
#ifndef GLFW_KEY_KP_3
#define GLFW_KEY_KP_3  323
#endif
#ifndef GLFW_KEY_KP_4
#define GLFW_KEY_KP_4  324
#endif
#ifndef GLFW_KEY_KP_5
#define GLFW_KEY_KP_5  325
#endif
#ifndef GLFW_KEY_KP_6
#define GLFW_KEY_KP_6  326
#endif
#ifndef GLFW_KEY_KP_7
#define GLFW_KEY_KP_7  327
#endif
#ifndef GLFW_KEY_KP_8
#define GLFW_KEY_KP_8  328
#endif
#ifndef GLFW_KEY_KP_9
#define GLFW_KEY_KP_9  329
#endif
#ifndef GLFW_KEY_KP_DECIMAL
#define GLFW_KEY_KP_DECIMAL  330
#endif
#ifndef GLFW_KEY_LEFT_CONTROL
#define GLFW_KEY_LEFT_CONTROL   341
#endif
#ifndef GLFW_KEY_RIGHT_CONTROL
#define GLFW_KEY_RIGHT_CONTROL  345
#endif
#ifndef GLFW_KEY_CAPS_LOCK
#define GLFW_KEY_CAPS_LOCK   280
#endif
#ifndef GLFW_KEY_F1
#define GLFW_KEY_F1  290
#endif
#ifndef GLFW_KEY_F2
#define GLFW_KEY_F2  291
#endif
#ifndef GLFW_KEY_F3
#define GLFW_KEY_F3  292
#endif
#ifndef GLFW_KEY_F4
#define GLFW_KEY_F4  293
#endif
#ifndef GLFW_KEY_F5
#define GLFW_KEY_F5  294
#endif
#ifndef GLFW_KEY_F6
#define GLFW_KEY_F6  295
#endif
#ifndef GLFW_KEY_F7
#define GLFW_KEY_F7  296
#endif


// ═══ KEY DISPATCH ════════════════════════════════════════════════
//
// Routes each GLFW key event to the module-owned function listed in
// the registry above. Sub-grouped to mirror the registry's order so
// the table and the dispatch read in parallel.

void on_key_down(int key) {
    // Single queue fetch: every queue-using case below reuses this.
    wgpu::Queue q = device_.GetQueue();

    switch (key) {

    // ── Movement ─────────────────────────────────────────────────
    case GLFW_KEY_UP:    keys_.forward = true;  break;
    case GLFW_KEY_DOWN:  keys_.backward = true; break;
    case GLFW_KEY_LEFT:  keys_.left = true;     break;
    case GLFW_KEY_RIGHT: keys_.right = true;    break;

    // ── World / aura toggles ─────────────────────────────────────
    case GLFW_KEY_1:
        gpuState_.toggle_freeze_sphere();
        break;
    case GLFW_KEY_2:
        pawn_state_.aura_height_enabled = !pawn_state_.aura_height_enabled;
        pawn_state_.aura_cfg_dirty = true;
        std::cout << "[Aura] Height extrusion: " << (pawn_state_.aura_height_enabled ? "ON" : "OFF") << "\n";
        break;
    case GLFW_KEY_3:
        pawn_state_.aura_enabled = !pawn_state_.aura_enabled;
        pawn_state_.aura_cfg_dirty = true;
        std::cout << "[Aura] Field: " << (pawn_state_.aura_enabled ? "ON" : "OFF") << "\n";
        break;
    // DONE[input:L1] five copy-paste cases collapsed into one helper
    //   call. request_mood_transition() lives in mood.inl.
    case GLFW_KEY_5: request_mood_transition(MOOD_OPEN_SUNSET);        break;
    case GLFW_KEY_6: request_mood_transition(MOOD_INDOOR_FLAT);        break;
    case GLFW_KEY_7: request_mood_transition(MOOD_INDOOR_VAULT);       break;
    case GLFW_KEY_8: request_mood_transition(MOOD_FINITE_OUTDOOR);     break;
    case GLFW_KEY_9: request_mood_transition(MOOD_FINITE_OUTDOOR_REF); break;
    case GLFW_KEY_0:              cycle_orb_palette(orbs_state_, this, q);          break;
    case GLFW_KEY_LEFT_BRACKET:   set_render_radius(world_state_.active_radius - 1); break;
    case GLFW_KEY_RIGHT_BRACKET:  set_render_radius(world_state_.active_radius + 1); break;

    // ── Musical modes (numpad) ───────────────────────────────────
    case GLFW_KEY_KP_1:       toggle_mmode(musical_state_, this, MMODE_TERRAIN_WAVES);   break;
    case GLFW_KEY_KP_2:       toggle_mmode(musical_state_, this, MMODE_COLOR_SHIFT);     break;
    case GLFW_KEY_KP_3:       toggle_mmode(musical_state_, this, MMODE_CHECKER_SCATTER); break;
    case GLFW_KEY_KP_4:       toggle_mmode(musical_state_, this, MMODE_PALETTE_DRIFT);   break;
    case GLFW_KEY_KP_5:       toggle_mmode(musical_state_, this, MMODE_GOL_TEMPO);       break;
    case GLFW_KEY_KP_6:       toggle_mmode(musical_state_, this, MMODE_AURA_EXPAND);     break;
    case GLFW_KEY_KP_7:       toggle_mmode(musical_state_, this, MMODE_RADIAL_PULSE);    break;
    case GLFW_KEY_KP_8:       cycle_orb_motion_rule(orbs_state_, this, q);            break;
    case GLFW_KEY_KP_9:       toggle_orb_anchor(orbs_state_, this);                 break;
    case GLFW_KEY_KP_DECIMAL: cycle_orb_gesture(orbs_state_, this, q);                break;

    // ── Camera / possession ──────────────────────────────────────
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL:
        toggle_fpv_mode();
        break;
    case GLFW_KEY_CAPS_LOCK:  try_possess_nearest(agent_state_, this, q);  break;

    // ── Diagnostics (function keys) ──────────────────────────────
    case GLFW_KEY_F1: cycle_agent_behavior_override(agent_state_, this, q);  break;
    case GLFW_KEY_F2: cycle_agent_tier_override(agent_state_, this, q);      break;
    case GLFW_KEY_F3: force_respawn_population(agent_state_, this, q);       break;
    case GLFW_KEY_F4: cycle_cube_behavior_override(cube_behaviors_state_, this, q);   break;
    case GLFW_KEY_F5: cycle_floater_coordination(cube_behaviors_state_, this);        break;
    case GLFW_KEY_F6: corral_cubes(cube_behaviors_state_, this, q);                   break;
    case GLFW_KEY_F7: toggle_cube_kite_mode(cube_behaviors_state_, this, q);          break;
    }
    update_movement_intent();
}

void on_key_up(int key) {
    switch (key) {
    case GLFW_KEY_UP:    keys_.forward = false;  break;
    case GLFW_KEY_DOWN:  keys_.backward = false; break;
    case GLFW_KEY_LEFT:  keys_.left = false;     break;
    case GLFW_KEY_RIGHT: keys_.right = false;    break;
    }
    update_movement_intent();
}


// ═══ MOUSE / SCROLL ══════════════════════════════════════════════

void on_mouse_move(float dx, float dy) {
    constexpr float sensitivity = 0.005f;
    if (mouse_.left_dragging) {
        inputState_.look_az_delta += dx * sensitivity;
        inputState_.look_el_delta += dy * sensitivity;
    }
    if (mouse_.right_dragging) {
        inputState_.pan_x_delta += dx * sensitivity;
        inputState_.pan_y_delta -= dy * sensitivity;
    }
}

void on_mouse_button(int button, bool pressed) {
    if (button == 0) mouse_.left_dragging = pressed;
    if (button == 1) mouse_.right_dragging = pressed;
}

void on_scroll(float delta) {
    inputState_.zoom_delta -= delta * 2.0f;
}


// ═══ MOVEMENT INTENT + DELTA CLEAR ═══════════════════════════════

void update_movement_intent() {
    inputState_.move_x = 0.0f;
    inputState_.move_z = 0.0f;

    if (keys_.forward)  inputState_.move_z -= 1.0f;
    if (keys_.backward) inputState_.move_z += 1.0f;
    if (keys_.left)     inputState_.move_x -= 1.0f;
    if (keys_.right)    inputState_.move_x += 1.0f;

    float len = std::sqrt(inputState_.move_x * inputState_.move_x +
        inputState_.move_z * inputState_.move_z);
    if (len > 1.0f) {
        inputState_.move_x /= len;
        inputState_.move_z /= len;
    }
}

void clear_input_deltas() {
    inputState_.look_az_delta = 0.0f;
    inputState_.look_el_delta = 0.0f;
    inputState_.zoom_delta = 0.0f;
    inputState_.pan_x_delta = 0.0f;
    inputState_.pan_y_delta = 0.0f;
}


// ═══ CAMERA / VIEW COMMANDS ══════════════════════════════════════

void toggle_fpv_mode() {
    player_.fpv_mode = !player_.fpv_mode;
    gpuState_.set_fpv_mode(player_.fpv_mode ? 1 : 0);
    std::cout << "[the_board] Camera mode: "
        << (player_.fpv_mode ? "First-Person View" : "Orbit") << std::endl;
}

void set_render_radius(uint32_t r) {
    r = std::max(r, GRID_RADIUS);
    r = std::min(r, PREGEN_RADIUS);
    if (r == world_state_.active_radius) return;
    world_state_.active_radius = r;
    uint32_t side = 2 * r + 1;
    std::cout << "[the_board] Render radius: " << r
        << " (" << side << "x" << side << " = " << side * side << " patches)" << std::endl;
    // Force full re-evaluation on next frame
    world_state_.last_center_x = INT32_MAX;
    world_state_.last_center_z = INT32_MAX;
}
