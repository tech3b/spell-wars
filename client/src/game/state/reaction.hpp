#pragma once

#include <functional>

class Reaction {
    bool reacted;
public:
    Reaction() : reacted(false) {
    }

    Reaction(bool _reacted) : reacted(_reacted) {
    }

    void react_once(const std::function<void()>& f) {
        if(!reacted) {
            f();
            reacted = true;
        }
    }
};