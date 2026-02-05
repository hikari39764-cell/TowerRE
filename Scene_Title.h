#pragma once

#include "IScene.h"

class ScrollBackground;

class Scene_Title : public IScene
{
public:
    explicit Scene_Title(ScrollBackground& bg);

    void OnCreate() override;
    void OnEnter() override;
    void OnExit() override;

    void Update(float dt) override;
    void Draw() override;

private:
    ScrollBackground& bg_;
    bool requested_ = false;
};
