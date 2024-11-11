#pragma once

enum Key
{
    NONE,
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    N0, N1, N2, N3, N4, N5, N6, N7, N8, N9,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    UP, DOWN, LEFT, RIGHT,
    SPACE, TAB, SHIFT, CTRL, INS, DEL, HOME, END, PGUP, PGDN,
    BACK, ESCAPE, RETURN, ENTER, PAUSE, SCROLL,
    NP0, NP1, NP2, NP3, NP4, NP5, NP6, NP7, NP8, NP9,
    NP_MUL, NP_DIV, NP_ADD, NP_SUB, NP_DECIMAL, PERIOD,
    EQUALS, COMMA, MINUS,
    OEM_1, OEM_2, OEM_3, OEM_4, OEM_5, OEM_6, OEM_7, OEM_8,
    CAPS_LOCK, ENUM_END
};

class InputState {
private:
    bool key_states[256];
public:
    InputState() {
        for(int i = 0; i < 256; i++) {
            key_states[i] = false;
        }
    }

    bool state_by_key(Key key) {
        return key_states[key];
    }

    void update_state(Key key, bool value) {
        key_states[key] = value;
    }
};