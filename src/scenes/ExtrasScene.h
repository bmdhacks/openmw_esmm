#pragma once
#include "Scene.h"

class ExtrasScene : public Scene {
public:
    ExtrasScene(StateMachine& machine);
    void on_enter() override;
    void render() override;
private:
    bool needs_refresh = true;
};
