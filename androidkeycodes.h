#ifndef ANDROIDKEYCODES_H
#define ANDROIDKEYCODES_H

/**
 * @file androidkeycodes.h
 * @brief Defines enumerations for Android keycodes and meta states.
 *
 * This file provides a mapping of common keyboard and device buttons
 * to their corresponding integer values as defined by the Android OS.
 * These values are typically used for simulating key events, for example,
 * with the `adb shell input keyevent` command.
 *
 * It also defines meta states (like Shift, Alt, Ctrl) used to modify
 * key events.
 */

/**
 * @enum AndroidKeycode
 * @brief Enumeration of key and button codes for the Android system.
 *
 * This corresponds to the key codes defined in Android's `android.view.KeyEvent` class.
 * It includes system navigation keys, alphanumeric keys, and other special function keys.
 */
enum AndroidKeycode {
    // --- Special & System Keys ---
    AKEYCODE_UNKNOWN = 0,      // Represents an unknown key code.
    AKEYCODE_HOME = 3,         // Home button.
    AKEYCODE_BACK = 4,         // Back button.
    AKEYCODE_POWER = 26,       // Power button.
    AKEYCODE_APP_SWITCH = 187, // App switch / Recents / Overview button.
    AKEYCODE_MENU = 82,        // Menu button.

    // --- Directional Pad (DPAD) Keys ---
    AKEYCODE_DPAD_UP = 19,
    AKEYCODE_DPAD_DOWN = 20,
    AKEYCODE_DPAD_LEFT = 21,
    AKEYCODE_DPAD_RIGHT = 22,

    // --- Hardware Control Keys ---
    AKEYCODE_VOLUME_UP = 24,
    AKEYCODE_VOLUME_DOWN = 25,

    // --- Alphanumeric Keys (A-Z) ---
    AKEYCODE_A = 29,
    AKEYCODE_B = 30,
    AKEYCODE_C = 31,
    AKEYCODE_D = 32,
    AKEYCODE_E = 33,
    AKEYCODE_F = 34,
    AKEYCODE_G = 35,
    AKEYCODE_H = 36,
    AKEYCODE_I = 37,
    AKEYCODE_J = 38,
    AKEYCODE_K = 39,
    AKEYCODE_L = 40,
    AKEYCODE_M = 41,
    AKEYCODE_N = 42,
    AKEYCODE_O = 43,
    AKEYCODE_P = 44,
    AKEYCODE_Q = 45,
    AKEYCODE_R = 46,
    AKEYCODE_S = 47,
    AKEYCODE_T = 48,
    AKEYCODE_U = 49,
    AKEYCODE_V = 50,
    AKEYCODE_W = 51,
    AKEYCODE_X = 52,
    AKEYCODE_Y = 53,
    AKEYCODE_Z = 54,

    // --- Symbol & Whitespace Keys ---
    AKEYCODE_COMMA = 55,
    AKEYCODE_PERIOD = 56,
    AKEYCODE_SPACE = 62,
    AKEYCODE_ENTER = 66,

    // --- Editing & Modifier Keys ---
    AKEYCODE_TAB = 61,
    AKEYCODE_DEL = 67,           // Backspace key (deletes character before the cursor).
    AKEYCODE_FORWARD_DEL = 112,  // Forward delete key (deletes character after the cursor).
    AKEYCODE_ESCAPE = 111,
    AKEYCODE_SHIFT_LEFT = 59,
    AKEYCODE_SHIFT_RIGHT = 60,
    AKEYCODE_CTRL_LEFT = 113,
    AKEYCODE_CTRL_RIGHT = 114,
    AKEYCODE_ALT_LEFT = 57,
    AKEYCODE_ALT_RIGHT = 58,
};

/**
 * @enum AndroidMetaState
 * @brief Enumeration of meta key states for modifying key events.
 *
 * These values are bit flags and can be combined (OR'd together) to represent
 * multiple active meta keys at once (e.g., Ctrl + Shift). They are used
 * with commands that need to simulate holding down a modifier key.
 */
enum AndroidMetaState {
    AMETA_NONE = 0,         // No meta key is pressed.
    AMETA_SHIFT_ON = 0x01,  // The Shift key is pressed.
    AMETA_ALT_ON = 0x02,    // The Alt key is pressed.
    AMETA_CTRL_ON = 0x1000, // The Control (Ctrl) key is pressed.
};

#endif // ANDROIDKEYCODES_H
