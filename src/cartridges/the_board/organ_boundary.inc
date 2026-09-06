// ORGAN — THE FRAME BOUNDARY. Included inside class Cartridge (cartridge.hpp):
// these are member functions and reach the cartridge's deps and queue directly.
// pawn.cpp's frame() calls them once per frame, before input and update.
// organ_registry.hpp owns the flags and masks consumed here and knows neither
// the deps nor the queue, which is why the consumption lives at this tier.

            // ORGAN — the frame boundary: doors, definition re-speaks and
            // the flush, taken once per frame for the LIVE mood only.
            void organ_flush(wgpu::Queue& queue) {
                // DOOR 0, Re-speak definitions: it raises the flags the
                // lines below already consume, so the boundary then does
                // what it does every frame. A definition write is durable
                // but not immediate, and this is that round trip.
                const uint32_t doors = t7::organ::take_doors_pending();
                if (doors & (1u << t7::organ::ORGAN_DOOR_RESPEAK)) {
                    t7::organ::g_def_dirty      = true;
                    t7::organ::g_def_dirty_mood = mood_state_.active;
                    t7::organ::g_tier_def_dirty = true;
                    // The door promises the LIVE definitions, so it names
                    // the live mood.
                    t7::organ::raise_orb_definition(mood_state_.active);
                }
                // THE SKY'S TWO PLAYER-OWNED FACTS, reached as doors: each
                // calls the function its key calls — the same KP_8 / KP_7
                // commands, guards and partial upload unchanged.
                if (doors & (1u << t7::organ::ORGAN_DOOR_ORB_RULE))
                    cycle_orb_motion_rule(orbs_state_, &orbs_deps_, queue);
                if (doors & (1u << t7::organ::ORGAN_DOOR_ORB_GESTURE))
                    cycle_orb_gesture(orbs_state_, &orbs_deps_, queue);

                // THE MOOD DOOR — the same request keys 5-9 press, with its
                // own guards. A door with a parameter, taken once.
                {
                    uint32_t go = 0;
                    if (t7::organ::take_go_mood(go))
                        request_mood_transition(transitionPhase_, pendingDestination_,
                                                mood_state_, world_state_, go);
                }

                // THE HOST DOOR (RIBBON_1) — the same transaction key R
                // presses, with its own guards. A door with a parameter,
                // taken once; possess() early-returns on a no-op and on a
                // RIBBON request with no ribbon to ride.
                {
                    uint32_t host = 0;
                    if (t7::organ::take_go_host(host))
                        possess(&input_deps_, static_cast<PointHost>(host));
                }

                // THE VISIT DOOR (VISIT_0) — a door with a parameter, taken
                // once: the shell names a hung photograph; begin_visit turns
                // its slot into a standing point the pilot walks to. An
                // empty slot is refused here, in words, because the roll the
                // shell pressed may be two seconds older than the wall.
                {
                    uint32_t slot = 0;
                    if (t7::organ::take_visit(slot)) {
                        const GPUPaintingSlot& s = gallery_state_.painting_slots[slot];
                        if (s.is_active == 0u)
                            std::cout << "[Visit] refused: slot " << slot << " is empty now\n";
                        else
                            begin_visit(&input_deps_, slot,
                                        s.position[0], s.position[2],
                                        s.forward[0],  s.forward[2],
                                        std::max(s.scale_x, s.scale_y),
                                        time_state_.seconds);
                    }
                }
                // THE VISIT WINDOW, refreshed here and nowhere else (the rule
                // window's charter): the pane reads whether a walk is on.
                t7::organ::set_visit_view(pilot_.active, pilot_.slot);

                uint32_t def_mood = 0;
                if (t7::organ::take_definition_dirty(def_mood)
                    && def_mood == mood_state_.active) {
                    // The roll, then the sky: a weight dial moves this
                    // world's regime under the same seed, and the sky is
                    // re-drawn from the regime it landed in.
                    apply_mood_regime(&mood_deps_, mood_def(def_mood));
                    apply_mood_lighting(&mood_deps_, mood_def(def_mood), queue);
                }
                // The tier bank changed, so its author re-speaks the whole
                // registry in its own voice — idempotent by construction.
                // Once per frame however many edits arrived.
                if (t7::organ::take_tier_definition_dirty()) {
                    upload_agent_registries_to_gpu(&agents_deps_, queue);
                }
                // THE CONSOLE DIALS LAND BY THEIR READERS' CADENCES: dome,
                // noise and speed take targeted partials, base size raises
                // the orb re-speak. This sits IMMEDIATELY BEFORE the
                // take_orb_definition_dirty() block.

                // base_size is an OrbConsole field, so its offset means
                // nothing in OrbMoodConfig's bit space: it raises the FLAG
                // with no touched bit, and this local is what keeps a
                // same-frame flock drag from swallowing it.
                bool console_reseed = false;
                if (const uint32_t cm = t7::organ::take_orb_console_dirty()) {
                    if (cm & 1u) gpuState_.upload_orb_dome_radius(queue,
                                     ORB_CONSOLE_LIVE.dome_radius);
                    if (cm & 4u) gpuState_.upload_orb_noise(queue,
                                     ORB_CONSOLE_LIVE.noise_floor);
                    // The master motion strength: a per-frame GPU read
                    // like dome and noise, so a targeted 4-byte partial and
                    // NO re-seed — it scales the sky's energy under the
                    // finger without replacing one orb.
                    if (cm & 8u) gpuState_.upload_orb_speed_mult(queue,
                                     ORB_CONSOLE_LIVE.speed_mult);
                    // A console edit is about the sky we are looking at,
                    // so the live mood is the honest answer.
                    if (cm & 2u) {
                        t7::organ::raise_orb_definition(mood_state_.active);
                        console_reseed = true;
                    }
                }
                // The orb mood bank changed, so its applier re-speaks — for
                // the LIVE mood only, because applying another mood's orb
                // definition would populate this sky from a world we are
                // not in. The call mirrors the mood fan's own site.
                uint32_t orb_mood = 0;
                if (t7::organ::take_orb_definition_dirty(orb_mood)) {
                    // AN AUTHOR RE-SPEAKS NO MORE THAN THE EDIT REQUIRES.
                    // Four of the nineteen orb-mood facts are baked into
                    // orb_state at init, so touching any of them re-seeds;
                    // the other fifteen ride the uniform upload.

                    // A RAISE WITH NO TOUCHED BITS MEANS EVERYTHING — the
                    // RESPEAK door, or a caller that did not say. AND ONLY
                    // WHEN THIS WORLD'S: the mask is taken either way, so a
                    // dormant mood's bits cannot reach the live reseed.
                    const uint32_t tm = t7::organ::take_orb_def_touched();
                    if (orb_mood == mood_state_.active) {
                        const bool reseed = console_reseed
                            || (tm == 0u)
                            || ((tm & t7::organ::ORB_RESEED_BITS) != 0u);
                        // THE MASK DRAINS EITHER WAY — take_orb_def_touched
                        // above is unconditional, so a dormant mood's bits
                        // cannot outlive a disabled piece; only the applier
                        // is gated.
                        if constexpr (ROSTER.orbs)  // ROSTER-GATE orbs (b) — sky dome never configured
                            configure_orbs(orbs_state_, &orbs_deps_,
                                ORB_MOOD_LIVE[orb_mood % MOOD_COUNT], queue,
                                reseed);
                    }
                }
                // THE RULE WINDOW, refreshed here and nowhere else. The
                // sky's rule and gesture are player-owned and live in
                // OrbsState; the panel READS them.

                // ONE WRITER, ONE SITE — the boundary, not the door
                // handlers: cycle_orb_motion_rule answers to the door above
                // and to KP_8, cycle_orb_gesture to KP_7, and a refresh
                // here cannot go stale whoever turned the rule.
                t7::organ::set_orb_rule_view(
                    orbs_state_.current_motion_rule,
                    orbs_state_.gesture_idx[orbs_state_.current_motion_rule & 3u],
                    orbs_state_.active, orbs_state_.count);
                gpuState_.organ_flush(queue);
            }
