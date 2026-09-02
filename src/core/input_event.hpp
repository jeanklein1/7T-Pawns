#pragma once

// ─── input_event.hpp ─────────────────────────────────────────────
//
// Console input abstraction: platform-agnostic input events produced by
// the console and consumed by cartridges. The console translates
// platform-specific input (GLFW, SDL, etc.) into these events;
// cartridges receive them and decide what each input means in their
// domain — a key might be "move forward" to a render cartridge or "play
// note C" to an analysis cartridge.
//
// Design principle: the console knows HOW input arrives (which key,
// which button); the cartridge knows WHAT input means (movement, music,
// camera control). This struct carries the "how" across the boundary.

namespace t7 {

// SHIP_1 — THE TOUCH TYPES CARRY INTENT, NOT A DEVICE. A mouse event
// says "the pointer moved by dx"; the console then decides, from a drag
// flag, whether that meant look or pan. A finger has no buttons to hold,
// so the console resolves the gesture first and the event that crosses
// this boundary is already the ANSWER: this much move, this much look,
// this much zoom. That is why there are no synthetic mouse or key events
// here — a synthesized keypress would be a second home for an intent the
// analog axes already hold.
struct InputEvent {
    enum class Type {
        KeyDown,
        KeyUp,
        MouseMove,
        MouseButton,
        Scroll,
        TouchMove,      // x, y = analog move vector, already dead-zoned and unit-clamped
        TouchLook,      // x, y = look delta, LOOK_SENS_TOUCH already applied
        TouchZoom,      // y    = zoom delta, PINCH_SENS already applied
        TouchTapLeft,   // a clean two-finger-left tap  — the aura verb
        TouchTapRight,  // a clean two-finger-right tap — the possession verb
        TouchTapPulse   // a clean LONE-finger tap, either half — the pulse verb
    };
    
    Type type;
    
    // Key events
    int key;              // Platform key code (e.g., GLFW_KEY_A)
    char character;       // ASCII character if printable, 0 otherwise
    
    // Mouse events
    float x, y;           // Position or delta depending on event type
    int button;           // Which button (0=left, 1=right, 2=middle)
    bool pressed;         // For MouseButton: true=pressed, false=released
};

} // namespace t7
