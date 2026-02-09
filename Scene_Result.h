#pragma once

#include "IScene.h"

class Scene_Result : public IScene
{
public:
    Scene_Result() = default;

    void OnCreate() override;
    void OnEnter() override;
    void OnExit() override;

    void Update(float dt) override;
    void Draw() override;

private:
    bool requested_ = false;
};
