// TweaksScene.h
#pragma once
#include "Scene.h"

class TweaksScene : public Scene {
public:
    TweaksScene(StateMachine& machine);
    void render() override;
};
