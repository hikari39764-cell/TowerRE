#pragma once
#include "HIKARI/HIKARI_Utility.h"
#include "BulletManager.h"
#include "HIKARI.h"
#include "GameConfig.h"

using namespace HIKARI;

class Player {
public:
    Player() = default;

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    enum class State { IntroIn, Play };
    void Init();
    void Update(float dt, BulletManager& bullets);
    void Draw();
    void OnHit(const Vector2* attackerPos = nullptr, float knockbackForce = 0.0f);
    bool IsDead() const { return hp_ <= 0; }
    Vector2 Pos() const { return playerT_.position; }
    Vector2 GetPos() const { return playerT_.position; }

    float   Radius() const { return radius_; }
    float   GetRadius() const { return radius_; }

    bool  Invincible() const { return invincibleTimer_ > 0.0f; }
    int   Mana() const { return mana_; }

    float GetHp() const { return (float)hp_; }
    float GetMaxHp() const { return (float)GAMECFG::kPlayerMaxHp; }

    int   GetMana() const { return mana_; }
    float GetManaTimer() const { return manaTimer_; }
    float GetManaInterval() const { return GAMECFG::kManaInterval; }

    int   GetSpeedLv() const { return speedLv_; }
    int   GetShotLv() const { return shotLv_; }

    float GetDashCd() const { return dashCd_; }
    float GetMaxDashCd() const { return GAMECFG::kDashCooldown; }

    float GetHealCd() const { return healCd_; }
    float GetMaxHealCd() const { return GAMECFG::kHealCooldown; } // 假设 GameConfig 有
    const Transform2D& Transform() const { return playerT_; }
    bool isUseMana[4];
private:
    HIKARI::SpineActor playerAni_;
    HIKARI::ANIM::TransformAnimator anim_;
    Vector2 knockbackVel_{ 0.0f, 0.0f };
    float knockbackDrag_ = 5.0f; // 阻力
    State state_ = State::IntroIn;
    float introT_ = 0.0f;

    Transform2D playerT_{};

    float radius_ = 14.0f;
    int hp_ = 5;

    // 魔力点
    int mana_ = 0;
    float manaTimer_ = 0.0f;

    // 本局持续升级
    int speedLv_ = 0;
    int shotLv_ = 0;

    // 射击切换
    bool firing_ = false;
    float shotTimer_ = 0.0f;

    // dash/heal
    float dashCd_ = 0.0f;
    float dashTimer_ = 0.0f;
    float healCd_ = 0.0f;
    float invincibleTimer_ = 0.0f;

    // 受击闪光相关
    HIKARI::POST::PostEffect* fxHit_ = nullptr;
    HIKARI::POST::PostChain   hitChain_;
    float flashTimer_ = 0.0f; // 闪光计时器

    Vector2 lastMoveDir_{ 0.0f, 1.0f }; 
    Vector2 dashDir_{ 0.0f, 1.0f };

private:
    void UpdateIntro(float dt);
    void UpdatePlay(float dt, BulletManager& bullets);

    void GainMana(float dt);
    bool ConsumeMana(int cost);

    float MoveSpeed() const;
    float ShotInterval() const;

    void ToggleFire();
    void TryUpgradeSpeed();
    void TryUpgradeShot();
    void TryDash();
    void TryHeal();

    void Shoot(float dt, BulletManager& bullets);
    void ClampToGameArea();

    void SyncPlayerAni_(float dt);
};
