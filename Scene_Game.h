#pragma once
#include "IScene.h"
#include "Player.h"
#include "BulletManager.h"
#include "PlayerAuraLayer.h"
#include "Boss.h"
#include "GameHUD.h"
class ScrollBackground;

class Scene_Game : public IScene {
public:
    explicit Scene_Game(ScrollBackground& bg);
    Scene_Game(const Scene_Game&) = delete;
    Scene_Game& operator=(const Scene_Game&) = delete;
    Scene_Game(Scene_Game&&) = delete;
    Scene_Game& operator=(Scene_Game&&) = delete;

    void Init() override;
    void Update(float dt) override;
    void Draw() override;
    bool WantsNext(SceneId& outNext) override;

private:
    ScrollBackground& bg_;
    Player player_;
    BulletManager bullets_;
    PlayerAuraLayer gAura;
    Boss boss_;
    GameHUD hud_;
};
