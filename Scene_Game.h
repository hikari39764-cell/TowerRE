#pragma once

#include "IScene.h"

#include "Boss.h"
#include "BulletManager.h"
#include "GameHUD.h"
#include "Player.h"
#include "PlayerAuraLayer.h"

class ScrollBackground;

class Scene_Game : public IScene
{
public:
    explicit Scene_Game(ScrollBackground& bg);
    Scene_Game(const Scene_Game&) = delete;
    Scene_Game& operator=(const Scene_Game&) = delete;
    Scene_Game(Scene_Game&&) = delete;
    Scene_Game& operator=(Scene_Game&&) = delete;

    void OnCreate() override;
    void OnEnter() override;
    void OnExit() override;

    void Update(float dt) override;
    void Draw() override;

private:
    ScrollBackground& bg_;
    Player player_;
    BulletManager bullets_;
    PlayerAuraLayer gAura;
    Boss boss_;
    GameHUD hud_;
    bool requestedResult_ = false;
};
