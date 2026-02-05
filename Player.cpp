#include "Player.h"
#include <cmath>
#include "GameConfig.h"
#include "HIKARI/HIKARI.h"
#include "BulletBehaviors_Basic.h"
#include "BulletPatterns.h"
#include "Vector2.h"
using namespace HIKARI;

static const int kShotIntervalsInFrames[] = {
    10, // Lv0
    8,  // Lv1
    7,  // Lv2
    9,  // Lv3
    7,  // Lv4
    6,  // Lv5
    7,  // Lv6
    6,  // Lv7
    5,  // Lv8
    7,  // Lv9
    5,  // Lv10
    6,  // Lv11
    22  // Lv12
};

static float ClampF(float v, float a, float b)
{
    if (v < a) { return a; }
    if (v > b) { return b; }
    return v;
}

void Player::Init()
{
    state_ = State::IntroIn;
    introT_ = 0.0f;

    radius_ = GAMECFG::kPlayerRadius;
    hp_ = GAMECFG::kPlayerMaxHp;

    mana_ = 0;
    manaTimer_ = 0.0f;

    speedLv_ = 0;
    shotLv_ = 0;

    firing_ = false;
    shotTimer_ = 0.0f;

    dashCd_ = 0.0f;
    dashTimer_ = 0.0f;
    healCd_ = 0.0f;
    invincibleTimer_ = 0.0f;


    if (!fxHit_) {
        fxHit_ = new HIKARI::POST::PostEffect();
        fxHit_->LoadPixelShader(L"./shaders/PS_SolidFlash.hlsl");
        hitChain_.Add(fxHit_);
    }

    flashTimer_ = 0.0f;
    lastMoveDir_ = { 0.0f, 1.0f };
    dashDir_ = { 0.0f, 1.0f };

    playerAni_.Load("./Animation/player.atlas", "./Animation/player.json");
    playerAni_.SetAnimation("idle", true);
    playerT_ = {};
    playerT_.position.x = GAMECFG::kGameW * 0.5f;
    playerT_.position.y = GAMECFG::kGameH + 80.0f;
    playerT_.pivotPx = { radius_ * 0.5f - 7.0f, radius_ * 0.5f };
    anim_.Bind(&playerT_);
    knockbackVel_ = { 0.0f, 0.0f };
    
    for (int i = 0; i < 4; i++)
    {
        isUseMana[i] = false;
    }
}

void Player::Update(float dt, BulletManager& bullets)
{

    for (int i = 0; i < 4; i++)
    {
        isUseMana[i] = false;
    }
    GainMana(dt);
    if (dashCd_ > 0.0f) { dashCd_ -= dt; }
    if (healCd_ > 0.0f) { healCd_ -= dt; }
    if (dashTimer_ > 0.0f) { dashTimer_ -= dt; }
    if (invincibleTimer_ > 0.0f) { invincibleTimer_ -= dt; }

    if (state_ == State::IntroIn) {
        UpdateIntro(dt);
    } else {
        UpdatePlay(dt, bullets);
    }

    ClampToGameArea();

    playerAni_.transform = playerT_;
    SyncPlayerAni_(dt);

    if (flashTimer_ > 0.0f) {
        flashTimer_ -= dt * 10.0f; 
        if (flashTimer_ < 0.0f) flashTimer_ = 0.0f;


        if (fxHit_) {
            DirectX::XMFLOAT4 p;
            p.x = flashTimer_; 
            p.y = 1.0f; p.z = 1.0f; p.w = 1.0f;
            fxHit_->SetUser(0, p);
        }
    }
}

void Player::UpdateIntro(float dt)
{
    introT_ += dt;
    float t = introT_ / GAMECFG::kIntroTime;
    if (t > 1.0f) { t = 1.0f; }

    float fromY = GAMECFG::kGameH + 80.0f;
    float toY = 700.0f;

    playerT_.position.y = fromY + (toY - fromY) * t;

    if (introT_ >= GAMECFG::kIntroTime) {
        state_ = State::Play;
    }
}

void Player::GainMana(float dt)
{
    if (mana_ >= GAMECFG::kManaMax) {
        manaTimer_ = 0.0f;
        return;
    }

    manaTimer_ += dt;
    if (manaTimer_ >= GAMECFG::kManaInterval) {
        manaTimer_ -= GAMECFG::kManaInterval;
        mana_ += 1;
        if (mana_ > GAMECFG::kManaMax) { mana_ = GAMECFG::kManaMax; }
    }
}

bool Player::ConsumeMana(int cost)
{
    if (mana_ < cost) { return false; }
    mana_ -= cost;
    if (mana_ < 0) { mana_ = 0; }
    return true;
}

float Player::MoveSpeed() const
{
    return GAMECFG::kPlayerBaseSpeed + (float)speedLv_ * GAMECFG::kPlayerSpeedStep;
}

void Player::ToggleFire()
{
    if (HINPUT::IsPressed("Space")) {
        firing_ = !firing_;
    }
}

void Player::TryUpgradeSpeed()
{
    if (HINPUT::IsPressed("I")) {
        if (speedLv_ < GAMECFG::kPlayerMaxSpeedLv) {
            if (ConsumeMana(GAMECFG::kCostUpgradeSpeed)) {
                speedLv_ += 1;
                isUseMana[1] = true;
            }
        }
    }
}

void Player::TryUpgradeShot()
{
    if (HINPUT::IsPressed("U")) {
        if (shotLv_ < 12) {
            if (ConsumeMana(GAMECFG::kCostUpgradeShot)) {
                shotLv_ += 1;
                isUseMana[0] = true;
            }
        }
    }
}

float Player::ShotInterval() const
{
    int lv = shotLv_;
    if (lv < 0) lv = 0;
    if (lv > 12) lv = 12;
    return (float)kShotIntervalsInFrames[lv] / 60.0f;
}

void Player::Shoot(float dt, BulletManager& bullets)
{
    if (!firing_) {
        if (shotTimer_ > 0.0f) {
            shotTimer_ -= dt;
            if (shotTimer_ < 0.0f) shotTimer_ = 0.0f;
        }
        return;
    }

    shotTimer_ += dt;
    float interval = ShotInterval();

    while (shotTimer_ >= interval) {
        shotTimer_ -= interval;

        // 统一用 Transform2D 位置
        BulletPatterns::PlayerShoot(bullets, shotLv_, playerT_.position.x, playerT_.position.y);
    }
}

void Player::TryDash()
{
    // 兼容：你可以用 Dash，也可以沿用 Player1 的 Capture
    if (!HINPUT::IsPressed("J")) {
        return;
    }

    if (dashCd_ > 0.0f) {
        return;
    }

    // 统一移动向量：优先手柄左摇杆 + 已归一化（输入空间：Y 上为 +）
    Vector2 moveIn = HINPUT::GetMoveVectorNormalized();

    // 没有输入 -> 用最近方向（原地 dash）
    if (moveIn.x == 0.0f && moveIn.y == 0.0f) {
        moveIn = lastMoveDir_;
    }

    float lenSq = moveIn.x * moveIn.x + moveIn.y * moveIn.y;
    if (lenSq <= 0.0001f) {
        return;
    }

    // 启动 dash 的这一帧才扣魔力点（不够就不启动）
    if (!ConsumeMana(GAMECFG::kCostDash)) {
        return;
    }
    isUseMana[3] = true;
    // 方向归一化后锁定
    float invLen = 1.0f / std::sqrt(lenSq);
    dashDir_.x = moveIn.x * invLen;
    dashDir_.y = moveIn.y * invLen;

    dashCd_ = GAMECFG::kDashCooldown;
    dashTimer_ = GAMECFG::kDashDuration;
    invincibleTimer_ = GAMECFG::kDashDuration;
}

void Player::TryHeal()
{
    if (HINPUT::IsPressed("K")) {
        if (healCd_ <= 0.0f) {
            if (hp_ < GAMECFG::kPlayerMaxHp + 10) {
                if (ConsumeMana(GAMECFG::kCostHeal)) {
                    hp_ += 1;
                    if (hp_ > GAMECFG::kPlayerMaxHp) { hp_ = GAMECFG::kPlayerMaxHp; }
                    healCd_ = GAMECFG::kHealCooldown;
                    isUseMana[2] = true;
                }
            }
        }
    }
}

void Player::UpdatePlay(float dt, BulletManager& bullets)
{
    ToggleFire();
    TryUpgradeSpeed();
    TryUpgradeShot();
    TryHeal();
    TryDash();


    Vector2 moveIn = HINPUT::GetMoveVectorNormalized();

    if (moveIn.x != 0.0f || moveIn.y != 0.0f) {
        lastMoveDir_ = moveIn;
    }


    Vector2 useIn = (dashTimer_ > 0.0f) ? dashDir_ : moveIn;


    Vector2 moveWorld{ useIn.x, -useIn.y };


    float dirXForAnim = useIn.x;
    if (dirXForAnim > 0.0f) {
        playerAni_.SetAnimation("moveRight", true);
    } else if (dirXForAnim < 0.0f) {
        playerAni_.SetAnimation("moveLeft", true);
    } else {
        playerAni_.SetAnimation("idle", true);
    }

    float spd = MoveSpeed();
    if (dashTimer_ > 0.0f) { spd *= GAMECFG::kDashSpeedMul; }

    playerT_.position.x += moveWorld.x * spd * dt;
    playerT_.position.y += moveWorld.y * spd * dt;

    Shoot(dt, bullets);

    if (std::abs(knockbackVel_.x) > 0.1f || std::abs(knockbackVel_.y) > 0.1f) {
        playerT_.position += knockbackVel_ * dt;
        // 阻力减速 (Lerp to zero)
        knockbackVel_.x = Lerp(knockbackVel_.x, 0.0f, knockbackDrag_ * dt);
        knockbackVel_.y = Lerp(knockbackVel_.y, 0.0f, knockbackDrag_ * dt);

        // 如果速度很小就归零
        if (LengthSq(knockbackVel_) < 10.0f) knockbackVel_ = { 0,0 };
    }

    anim_.Update(dt);

    ClampToGameArea();
}

void Player::OnHit(const Vector2* attackerPos, float knockbackForce) {
    if (invincibleTimer_ > 0.0f && flashTimer_ <= 0.0f) return;
    flashTimer_ = 1.0f;
    hp_ -= 1;
    invincibleTimer_ = 1.0f;

    anim_.PlayShakeEx(5.0f, 3.0f, 0.0f, 20.0f, 15.0f, 0.0f, 0.3f, HIKARI::ANIM::EASE::OutQuad);

    if (attackerPos && knockbackForce > 0.0f) {
        Vector2 dir = Normalize(playerT_.position - *attackerPos);
        knockbackVel_ = dir * knockbackForce;

        anim_.PlayShakeEx(10.0f, 10.0f, 0.0f, 15.0f, 15.0f, 0.0f, 0.5f, HIKARI::ANIM::EASE::OutQuad);
    }
}

void Player::ClampToGameArea()
{
    playerT_.position.x = ClampF(playerT_.position.x, radius_, (float)GAMECFG::kGameW - radius_);
    playerT_.position.y = ClampF(playerT_.position.y, radius_, (float)GAMECFG::kGameH - radius_);
}

void Player::SyncPlayerAni_(float dt)
{
    playerAni_.Update(dt);
}

void Player::Draw() {
    bool useFlash = (flashTimer_ > 0.0f);

    if (useFlash) {
        HIKARI::POST::PostSystem::BeginLayer(hitChain_, 0, 0, 0, 0);
    }

    playerAni_.Draw();

    if (useFlash) {
        HIKARI::POST::PostSystem::EndLayer();
    }
}