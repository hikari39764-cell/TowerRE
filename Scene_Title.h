#pragma once
#include "IScene.h"

class ScrollBackground;

class Scene_Title : public IScene {
public:
    explicit Scene_Title(ScrollBackground& bg);

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    bool WantsNext(SceneId& outNext) override;

private:
    ScrollBackground& bg_;
    bool wantNext_ = false;
};
