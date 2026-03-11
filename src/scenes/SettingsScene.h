#pragma once
#include "Scene.h"

class SettingsScene : public Scene {
public:
    SettingsScene(StateMachine& machine);
    void render() override;
};
