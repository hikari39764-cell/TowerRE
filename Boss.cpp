#include "Boss.h"
#include "Player.h"
#include "BulletManager.h"
#include "BulletBehaviors_Basic.h" 
#include "HIKARI_Camera.h"
#include "HIKARI_Particle.h"
#include <cmath>
#include <algorithm>
#include "Vector2.h"

static float EaseInOutQuad(float t) { return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f; }
static float LerpFloat(float a, float b, float t) { return a + (b - a) * t; }
static Vector2 LerpVec2(const Vector2& a, const Vector2& b, float t) {
    return { a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}
static float NormalizeAngle(float angle) {
    while (angle > 3.14159265f) angle -= 6.2831853f;
    while (angle < -3.14159265f) angle += 6.2831853f;
    return angle;
}

static void ShakeScreenHeavy(float duration) {
    HIKARI::CAMERA::ShakeParams p{};
    p.ampX = 15.0f; p.ampY = 20.0f;
    p.ampRot = 0.0f;
    p.freqX = 25.0f; p.freqY = 30.0f;
    p.durationSec = duration;
    p.envelope = HIKARI::ANIM::EASE::OutQuad;
    HIKARI::CAMERA::ShakeEx(p);
}

static void ShakeScreenLight(float duration) {
    HIKARI::CAMERA::ShakeParams p{};
    p.ampX = 5.0f; p.ampY = 5.0f;
    p.ampRot = 0.0f;
    p.freqX = 40.0f; p.freqY = 40.0f;
    p.durationSec = duration;
    HIKARI::CAMERA::ShakeEx(p);
}

Boss::Boss() {
    spine_.Load("./Animation/boss.atlas", "./Animation/boss.json");
    spine_.SetAnimation("intro1", true);
    spine_.transform.position = { 640.0f, 360.0f };
}

void Boss::InitPostEffects() {
    if (!fxIce_) {
        fxIce_ = new HIKARI::POST::PostEffect();
        fxIce_->LoadPixelShader(L"./shaders/PS_IceFreeze.hlsl");
        bossChain_.Add(fxIce_);
    }
    if (!fxFire_) {
        fxFire_ = new HIKARI::POST::PostEffect();
        fxFire_->LoadPixelShader(L"./shaders/PS_FireHeat.hlsl");
        bossChain_.Add(fxFire_);
    }
    if (!fxHit_) {
        fxHit_ = new HIKARI::POST::PostEffect();
        fxHit_->LoadPixelShader(L"./shaders/PS_SolidFlash.hlsl");
        hitChain_.Add(fxHit_);
    }
}

void Boss::Init(Player* target, BulletManager* bulletMgr, HIKARI::PARTICLE::ParticleSystem* particleSys) {
    target_ = target;
    bulletMgr_ = bulletMgr;
    particleSys_ = particleSys;

    InitPostEffects();
    InitParticleEffects();

    SetDifficulty(DifficultyLevel::Normal);

    position_ = { 640.0f, 200.0f };
    spine_.transform.position = position_;
    spine_.Update(0.0f);
    anim_.Bind(&spine_.transform);

    state_ = BossState::Intro;
    introAnimPlayed_ = false;
    currentGlobalPhase_ = GlobalPhase::Normal;
    activeModeIndex_ = -1;
    lastSkillName_.clear();

    for (int i = 0; i < 4; i++) phaseTriggered_[i] = false;

    renderScale_ = { 1.0f, 1.0f };
    renderRotation_ = 0.0f;
    velocity_ = { 0.0f, 0.0f };

    executionQueue_.clear();
    waitingForCombo_ = false;
    comboTimer_ = 0.0f;
    patternTime_ = 0.0f;
}

void Boss::InitParticleEffects() {
    if (!particleSys_) return;

    auto chargeCfg = HIKARI::PARTICLE::PRESET::MakeHomingConfig(
        { 0,0 }, { 120, 120 }, &spine_.transform,
        40, 0.4f, 0.8f, 200.0f, 500.0f,
        0xFF88FFFF, 0x00000000, 15.0f
    );
    particleSys_->RegisterEffect("BossCharge", { chargeCfg, HIKARI::PARTICLE::PRESET::MakeCircleDrawer(HIKARI::RENDERER::CameraMode::Inherit), HIKARI::PARTICLE::PRESET::MakeHomingSpawn() });

    auto impactCfg = HIKARI::PARTICLE::PRESET::MakeShockwaveRingConfig(
        { 0,0 }, 64, 0.4f, 0.8f, 80.0f, 700.0f, 0xFFEE88FF, 0xFF000000, 32
    );
    particleSys_->RegisterEffect("BossImpact", { impactCfg, HIKARI::PARTICLE::PRESET::MakeRingDrawer(HIKARI::RENDERER::CameraMode::Inherit), HIKARI::PARTICLE::PRESET::MakeShockwaveRingSpawn() });
}

void Boss::SetDifficulty(DifficultyLevel level) {
    difficulty_ = level;
    if (level == DifficultyLevel::Challenge) {
        maxHp_ = 50000.0f;
        speedMultiplier_ = 1.15f;
        rushSpeed_ = 1500.0f;
        normalDuration_ = 2.5f;
    } else {
        maxHp_ = 35000.0f;
        speedMultiplier_ = 1.0f;
        rushSpeed_ = 950.0f;
        normalDuration_ = 4.0f;
    }
    hp_ = maxHp_;
    InitModes();
}

void Boss::InitModes() {
    modes_.clear();

    ModeDef meleeMode;
    meleeMode.name = "Melee Form";
    meleeMode.switchAnim = "switchPhase1";
    meleeMode.modeIdleAnim = "idle1";
    meleeMode.returnAnim = "returnPhase1";
    meleeMode.weight = 50;
    meleeMode.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Ice, GlobalPhase::Fire, GlobalPhase::Final };

    SkillDef rushCombo;
    rushCombo.name = "Rush Combo";
    rushCombo.weight = 30;
    rushCombo.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Ice };
    rushCombo.sequence = { { ActionPattern::Rush, 0.0f }, { ActionPattern::Wait, 0.3f }, { ActionPattern::Rush, 0.0f } };
    meleeMode.skillPool.push_back(rushCombo);

    SkillDef groundSlam;
    groundSlam.name = "Ground Slam";
    groundSlam.weight = 40;
    groundSlam.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Fire, GlobalPhase::Final };
    groundSlam.sequence = { { ActionPattern::GroundSlam, 2.0f } };
    meleeMode.skillPool.push_back(groundSlam);

    SkillDef flameRushBurst;
    flameRushBurst.name = "Flame Rush Burst";
    flameRushBurst.weight = 25;
    flameRushBurst.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Fire };
    flameRushBurst.sequence = { { ActionPattern::FlameRushBurst, 0.0f } };
    meleeMode.skillPool.push_back(flameRushBurst);

    modes_.push_back(meleeMode);

    ModeDef magicMode;
    magicMode.name = "Magic Form";
    magicMode.switchAnim = "switchPhase2";
    magicMode.modeIdleAnim = "idle2";
    magicMode.returnAnim = "returnPhase2";
    magicMode.weight = 50;
    magicMode.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Ice, GlobalPhase::Fire, GlobalPhase::Final };

    SkillDef apocalypse;
    apocalypse.name = "Apocalypse";
    apocalypse.weight = 25;
    apocalypse.allowedGlobalPhases = { GlobalPhase::Fire, GlobalPhase::Final };
    apocalypse.sequence = { { ActionPattern::Apocalypse, 5.0f } };
    magicMode.skillPool.push_back(apocalypse);

    SkillDef scatterShot;
    scatterShot.name = "Scatter Shot";
    scatterShot.weight = 30;
    scatterShot.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Ice, GlobalPhase::Fire };
    scatterShot.sequence = { { ActionPattern::ScatterShot, 0.0f } };
    magicMode.skillPool.push_back(scatterShot);

    SkillDef icicleRain; icicleRain.name = "Icicle Rain"; icicleRain.weight = 30; icicleRain.allowedGlobalPhases = { GlobalPhase::Ice };
    icicleRain.sequence = { { ActionPattern::IcicleRain, 0.0f } };
    magicMode.skillPool.push_back(icicleRain);

    SkillDef fanShot; fanShot.name = "Fan Shot"; fanShot.weight = 25; fanShot.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Fire };
    fanShot.sequence = { { ActionPattern::FanShot, 0.0f } };
    magicMode.skillPool.push_back(fanShot);

    SkillDef fireSpiral; fireSpiral.name = "FireSpiral"; fireSpiral.weight = 30; fireSpiral.allowedGlobalPhases = { GlobalPhase::Fire };
    fireSpiral.sequence = { {ActionPattern::FireSpiral, 0.0f} };
    magicMode.skillPool.push_back(fireSpiral);

    modes_.push_back(magicMode);
}

void Boss::Update(float dt) {
    float adjustedDt = dt * speedMultiplier_;
    if (hurtTimer_ > 0.0f) hurtTimer_ -= dt;

    static Vector2 lastPos = position_;
    if (dt > 0.0001f) {
        velocity_ = (position_ - lastPos) * (1.0f / dt);
    }
    lastPos = position_;

    spine_.transform.position = position_;
    spine_.transform.scale = renderScale_;
    spine_.transform.rotation = renderRotation_;
    anim_.Update(dt);
    spine_.Update(adjustedDt);

    UpdateState(adjustedDt);
    UpdateVisuals(dt);

    if (currentGlobalPhase_ == GlobalPhase::Ice) iceProgress_ += dt * 0.8f;
    else iceProgress_ -= dt * 1.0f;
    iceProgress_ = std::clamp(iceProgress_, 0.0f, 1.0f);

    if (currentGlobalPhase_ == GlobalPhase::Fire || currentGlobalPhase_ == GlobalPhase::Final) fireProgress_ += dt * 0.8f;
    else fireProgress_ -= dt * 1.0f;
    fireProgress_ = std::clamp(fireProgress_, 0.0f, 1.0f);

    float screenW = 1280.0f; float screenH = 720.0f;
    DirectX::XMFLOAT4 p1;
    p1.x = position_.x / screenW;
    p1.y = (position_.y - size_.y * 0.5f) / screenH;

    if (fxIce_) { DirectX::XMFLOAT4 p0; p0.x = iceProgress_; fxIce_->SetUser(0, p0); fxIce_->SetUser(1, p1); }
    if (fxFire_) { DirectX::XMFLOAT4 p0; p0.x = fireProgress_; fxFire_->SetUser(0, p0); fxFire_->SetUser(1, p1); }
    if (hitFlashTimer_ > 0.0f) {
        hitFlashTimer_ -= dt * 5.0f;
        if (hitFlashTimer_ < 0.0f) hitFlashTimer_ = 0.0f;
        if (fxHit_) { DirectX::XMFLOAT4 p; p.x = hitFlashTimer_; p.y = 1.0f; p.z = 0.5f; p.w = 0.5f; fxHit_->SetUser(0, p); }
    }
}

void Boss::UpdateVisuals(float dt) {
    float speed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y);
    float targetScaleX = 1.0f;
    float targetScaleY = 1.0f;
    float targetRot = 0.0f;

    if (speed > 200.0f) {
        float stretch = 1.0f + std::min(speed * 0.0004f, 0.4f);
        targetScaleY = stretch;
        targetScaleX = 1.0f / stretch;

        float tilt = (velocity_.x / 1000.0f) * -0.2f;
        targetRot = std::clamp(tilt, -0.2f, 0.2f);
    }

    renderScale_.x = LerpFloat(renderScale_.x, targetScaleX, dt * 15.0f);
    renderScale_.y = LerpFloat(renderScale_.y, targetScaleY, dt * 15.0f);
    renderRotation_ = LerpFloat(renderRotation_, targetRot, dt * 10.0f);

    if (speed > 700.0f) {
        trailTimer_ += dt;
        if (trailTimer_ > 0.05f) {
            trailTimer_ = 0.0f;
            if (particleSys_) {
                auto cfg = HIKARI::PARTICLE::PRESET::MakeRadialBurstConfig(position_, 2, 0.2f, 0.4f, 10.0f, 40.0f, 0x8888FF66, 0x00FFFFFF);
                particleSys_->CreateEmitter(cfg, HIKARI::PARTICLE::PRESET::MakeCircleDrawer(HIKARI::RENDERER::CameraMode::Inherit), HIKARI::PARTICLE::PRESET::MakeRadialBurstSpawn())->EmitBurst(2);
            }
        }
    }
}

void Boss::UpdateState(float dt) {
    stateTimer_ += dt;

    if (state_ != BossState::Dead && state_ != BossState::Transitioning && state_ != BossState::Intro) {
        CheckGlobalPhase();
    }

    if (state_ == BossState::Execute && patternTime_ > 15.0f) {
        TryNextComboAction();
        patternTime_ = 0.0f;
    }

    switch (state_) {
    case BossState::Intro:
        if (!introAnimPlayed_) {
            spine_.SetAnimation("intro1", false);
            spine_.AddAnimation("idle", true, 0.0f);
            ShakeScreenHeavy(1.5f);
            introAnimPlayed_ = true;
            position_ = { 640.0f, 200.0f };
        }
        if (stateTimer_ > introDuration_) {
            state_ = BossState::Normal;
            stateTimer_ = 0.0f;
            spine_.SetAnimation("idle", true);
        }
        break;

    case BossState::Normal:
        UpdateNormalBehavior(dt);
        break;

    case BossState::Switching:
        if (stateTimer_ > 1.2f) {
            state_ = BossState::Execute;
            stateTimer_ = 0.0f;
            if (activeModeIndex_ >= 0) spine_.SetAnimation(modes_[activeModeIndex_].modeIdleAnim.c_str(), true);
            TryNextComboAction();
        }
        break;

    case BossState::Execute:
        if (waitingForCombo_) {
            comboTimer_ -= dt;
            if (comboTimer_ <= 0.0f) {
                waitingForCombo_ = false;
                TryNextComboAction();
            }
        } else {
            if (pattern_ == ActionPattern::Changing) {
                easeTime_ += dt;
                float t = easeTime_ / easeDuration_;
                if (t >= 1.0f) t = 1.0f;
                position_ = LerpVec2(easeStartPos_, easeTargetPos_, EaseInOutQuad(t));
                if (t >= 1.0f) {
                    pattern_ = easeNextPattern_;
                    patternTime_ = 0.0f;
                    rushStep_ = 0; ringStep_ = 0; fanShotStep_ = 0;
                    crossBurstStep_ = 0; icicleSpawnTimer_ = 0.0f;
                    iceMissileStep_ = 0; iceMissileTotal_ = 0;
                    fireSpiralTimer_ = 0.0f; fireSpiralAngle_ = 0.0f;
                    flameRushStep_ = 0; flameBurstRemaining_ = 0;
                    iceSkateStep_ = 0; iceSkateDropTimer_ = 0.0f;
                    slamStep_ = 0; scatterStep_ = 0; apocalypseStep_ = 0;
                }
            } else if (pattern_ == ActionPattern::Rush) PatternRush(dt);
            else if (pattern_ == ActionPattern::GroundSlam) PatternGroundSlam(dt);
            else if (pattern_ == ActionPattern::ScatterShot) PatternScatterShot(dt);
            else if (pattern_ == ActionPattern::Apocalypse) PatternApocalypse(dt);
            else if (pattern_ == ActionPattern::RingShot) PatternRingShot(dt);
            else if (pattern_ == ActionPattern::FanShot) PatternFanShot(dt);
            else if (pattern_ == ActionPattern::CrossBurst) PatternCrossBurst(dt);
            else if (pattern_ == ActionPattern::IcicleRain) PatternIcicleRain(dt);
            else if (pattern_ == ActionPattern::IceMissile) PatternIceMissile(dt);
            else if (pattern_ == ActionPattern::FireSpiral) PatternFireSpiral(dt);
            else if (pattern_ == ActionPattern::FlameRushBurst) PatternFlameRushBurst(dt);
            else if (pattern_ == ActionPattern::IceSkateRush) PatternIceSkateRush(dt);
        }
        break;

    case BossState::Returning:
        if (pattern_ == ActionPattern::Changing) {
            easeTime_ += dt;
            float t = easeTime_ / easeDuration_;
            if (t >= 1.0f) t = 1.0f;
            position_ = LerpVec2(easeStartPos_, easeTargetPos_, EaseInOutQuad(t));
            if (t >= 1.0f) {
                pattern_ = easeNextPattern_;
                patternTime_ = 0.0f;
            }
        }
        if (stateTimer_ > 0.5f && pattern_ != ActionPattern::Changing) {
            state_ = BossState::Normal;
            stateTimer_ = 0.0f;
            activeModeIndex_ = -1;
            spine_.SetAnimation("idle", true);
        }
        break;

    case BossState::Transitioning:
    {
        Vector2 centerTarget = { 640.0f, 300.0f };
        position_ = LerpVec2(position_, centerTarget, 0.15f);

        if (stateTimer_ > 0.8f && stateTimer_ < 0.85f) {
            ShakeScreenHeavy(0.8f);
        }
        if (stateTimer_ > 2.5f) {
            state_ = BossState::Normal;
            stateTimer_ = 0.0f;
            spine_.SetAnimation("idle", true);
        }
    }
    break;

    case BossState::Dead:
        break;
    }
}

void Boss::CheckGlobalPhase() {
    float p = hp_ / maxHp_;
    GlobalPhase next = currentGlobalPhase_;
    if (!phaseTriggered_[1] && p <= 0.70f) { next = GlobalPhase::Ice; phaseTriggered_[1] = true; } else if (!phaseTriggered_[2] && p <= 0.40f) { next = GlobalPhase::Fire; phaseTriggered_[2] = true; } else if (!phaseTriggered_[3] && p <= 0.15f) { next = GlobalPhase::Final; phaseTriggered_[3] = true; }

    if (next != currentGlobalPhase_) {
        currentGlobalPhase_ = next;
        state_ = BossState::Transitioning;
        stateTimer_ = 0.0f;
        velocity_ = { 0,0 };
        executionQueue_.clear();
        waitingForCombo_ = false;
        pattern_ = ActionPattern::Idle;
        renderScale_ = { 1,1 };

        if (activeModeIndex_ >= 0) {
            spine_.SetAnimation("idle", true);
            activeModeIndex_ = -1;
        }
        if (onPhaseChange_) onPhaseChange_(currentGlobalPhase_);
    }
}

void Boss::UpdateNormalBehavior(float dt) {
    normalMoveTime_ += dt;
    shootTimer_ -= dt;

    float baseDuration = 5.0f;
    if (currentGlobalPhase_ != GlobalPhase::Normal) baseDuration = 3.5f;
    if (difficulty_ == DifficultyLevel::Challenge) baseDuration *= 0.7f;
    normalDuration_ = baseDuration;

    float ampX = moveSpeed_ * 2.5f;
    float ampY = moveSpeed_ * 0.5f;
    position_.x = 640.0f + std::sin(normalMoveTime_) * ampX;
    position_.y = 200.0f + std::sin(normalMoveTime_ * 2.0f) * ampY;

    if (shootTimer_ <= 0.0f) {
        FireNormalBarrage();
        float baseCd = 1.0f;
        if (currentGlobalPhase_ == GlobalPhase::Ice) baseCd = 0.8f;
        if (currentGlobalPhase_ == GlobalPhase::Fire) baseCd = 0.6f;
        if (currentGlobalPhase_ == GlobalPhase::Final) baseCd = 0.4f;

        if (difficulty_ == DifficultyLevel::Challenge) baseCd *= 0.8f;
        shootTimer_ = baseCd;
    }

    if (stateTimer_ > normalDuration_) {
        SelectModeAndSkill();
    }
}

void Boss::FireNormalBarrage() {
    if (!bulletMgr_ || !target_) return;
    std::string style = GetBulletStyleForPhase();

    renderScale_.y = 0.95f; renderScale_.x = 1.05f;
    ShakeScreenLight(0.1f);

    Vector2 toPlayer = target_->GetPos() - position_;
    float aimAngle = std::atan2(toPlayer.y, toPlayer.x);

    int aimCount = (difficulty_ == DifficultyLevel::Challenge) ? 5 : 3;
    float spread = 0.15f;
    for (int i = 0; i < aimCount; ++i) {
        float a = aimAngle + (i - (aimCount - 1) * 0.5f) * spread;
        Bullet b; b.pos = position_;
        b.vel = { cos(a) * 450.0f, sin(a) * 450.0f };
        b.isEnemy = true;
        static BulletLinear lin; b.behavior = &lin;
        bulletMgr_->Spawn(b, style);
    }

    static int shotCounter = 0;
    shotCounter++;
    if (shotCounter % 2 == 0) {
        int ringCount = (currentGlobalPhase_ == GlobalPhase::Normal) ? 12 : 18;
        if (currentGlobalPhase_ == GlobalPhase::Final) ringCount = 24;

        float ringStep = 6.28318f / ringCount;
        float offset = (shotCounter * 0.1f);

        for (int i = 0; i < ringCount; ++i) {
            float a = i * ringStep + offset;
            Bullet b; b.pos = position_;
            float spd = 300.0f;
            b.vel = { cos(a) * spd, sin(a) * spd };
            b.isEnemy = true;
            static BulletLinear lin; b.behavior = &lin;
            bulletMgr_->Spawn(b, style);
        }
    }

    if (currentGlobalPhase_ != GlobalPhase::Normal) {
        int rndCount = 4;
        for (int i = 0; i < rndCount; ++i) {
            float rndA = aimAngle + (rand() % 100 - 50) * 0.02f;
            float spd = 200.0f + rand() % 150;
            Bullet b; b.pos = position_;
            b.vel = { cos(rndA) * spd, sin(rndA) * spd };
            b.isEnemy = true;
            static BulletAccel accel; b.behavior = &accel;
            b.acc = b.vel * 0.5f;
            bulletMgr_->Spawn(b, (currentGlobalPhase_ == GlobalPhase::Fire) ? "e_fire" : "e_ice");
        }
    }
}

void Boss::SelectModeAndSkill() {

    std::vector<int> validModeIndices;
    float totalWeight = 0.0f;
    for (int i = 0; i < modes_.size(); ++i) {
        bool allowed = modes_[i].allowedGlobalPhases.empty();
        for (auto p : modes_[i].allowedGlobalPhases) { if (p == currentGlobalPhase_) { allowed = true; break; } }
        if (allowed && modes_[i].weight > 0) { validModeIndices.push_back(i); totalWeight += modes_[i].weight; }
    }
    if (validModeIndices.empty()) { state_ = BossState::Normal; stateTimer_ = 0.0f; return; }

    int selectedIdx = validModeIndices[0];
    float r = (static_cast<float>(rand()) / RAND_MAX) * totalWeight;
    float currentSum = 0.0f;
    for (int idx : validModeIndices) {
        currentSum += modes_[idx].weight;
        if (r < currentSum) { selectedIdx = idx; break; }
    }

    activeModeIndex_ = selectedIdx;
    const auto& mode = modes_[activeModeIndex_];

    float totalSkillWeight = 0.0f;
    std::vector<int> validSkills;
    for (size_t i = 0; i < mode.skillPool.size(); ++i) {
        const auto& s = mode.skillPool[i];
        bool pOk = s.allowedGlobalPhases.empty();
        for (auto p : s.allowedGlobalPhases) if (p == currentGlobalPhase_) pOk = true;
        if (pOk) {
            float w = (float)s.weight;
            if (s.name == lastSkillName_) w *= 0.3f;
            totalSkillWeight += w;
            validSkills.push_back((int)i);
        }
    }

    if (validSkills.empty()) { state_ = BossState::Normal; stateTimer_ = 0.0f; return; }

    int skillIdx = validSkills[0];
    float rS = (static_cast<float>(rand()) / RAND_MAX) * totalSkillWeight;
    float sSum = 0.0f;
    for (int idx : validSkills) {
        float w = (float)mode.skillPool[idx].weight;
        if (mode.skillPool[idx].name == lastSkillName_) w *= 0.3f;
        sSum += w;
        if (rS < sSum) { skillIdx = idx; break; }
    }

    const auto& skill = mode.skillPool[skillIdx];
    lastSkillName_ = skill.name;

    executionQueue_.clear();
    for (const auto& node : skill.sequence) executionQueue_.push_back(node);

    state_ = BossState::Switching;
    stateTimer_ = 0.0f;
    spine_.SetAnimation(mode.switchAnim.c_str(), false);
    RequestPattern(ActionPattern::Idle, true);
}

void Boss::TryNextComboAction() {
    if (executionQueue_.empty()) {
        state_ = BossState::Returning;
        stateTimer_ = 0.0f;
        if (activeModeIndex_ >= 0) spine_.SetAnimation(modes_[activeModeIndex_].returnAnim.c_str(), false);
        RequestPattern(ActionPattern::Idle, true);
        return;
    }
    SkillNode node = executionQueue_.front();
    executionQueue_.pop_front();

    if (node.action == ActionPattern::Wait) {
        waitingForCombo_ = true; comboTimer_ = node.duration;
    } else {
        RequestPattern(node.action, true);
        if (node.duration > 0.0f) { waitingForCombo_ = true; comboTimer_ = node.duration; }
    }
}

void Boss::RequestPattern(ActionPattern next, bool ease) {
    if (next == ActionPattern::GroundSlam) ease = false;

    if (ease) {
        pattern_ = ActionPattern::Changing;
        easeStartPos_ = position_;
        easeNextPattern_ = next;
        easeDuration_ = 0.3f;
        easeTime_ = 0.0f;

        if (next == ActionPattern::Rush) easeTargetPos_ = { 640.0f, 200.0f };
        else if (next == ActionPattern::Apocalypse) { easeTargetPos_ = { 640.0f, 360.0f }; easeDuration_ = 0.6f; } else if (next == ActionPattern::ScatterShot) { easeTargetPos_ = { 640.0f, 300.0f }; easeDuration_ = 0.4f; } else easeTargetPos_ = { 640.0f, 230.0f };
    } else {
        pattern_ = next;

        rushStep_ = 0; ringStep_ = 0; fanShotStep_ = 0;
        crossBurstStep_ = 0; icicleSpawnTimer_ = 0.0f;
        iceMissileStep_ = 0; fireSpiralTimer_ = 0.0f;
        flameRushStep_ = 0; iceSkateStep_ = 0;
        slamStep_ = 0; scatterStep_ = 0; apocalypseStep_ = 0;
    }
}

void Boss::PatternRush(float dt) {
    patternTime_ += dt;
    if (rushStep_ == 0) {
        position_.y -= 20.0f * dt;
        renderScale_.x = 1.3f; renderScale_.y = 0.7f;
        if (particleSys_ && rand() % 5 == 0) particleSys_->PlayOneShot("BossCharge", position_, 1);

        if (patternTime_ > 0.5f) {
            rushStep_ = 1; patternTime_ = 0.0f;
            if (target_) rushDir_ = Normalize(target_->GetPos() - position_);
            else rushDir_ = { 0, 1 };
            ShakeScreenHeavy(0.3f);
        }
    } else if (rushStep_ == 1) {
        position_ += rushDir_ * rushSpeed_ * dt;
        if (position_.x < -200 || position_.x > 1480 || position_.y > 900 || position_.y < -200 || patternTime_ > 2.0f) {
            waitingForCombo_ = false; TryNextComboAction();
            renderScale_ = { 1.0f, 1.0f };
        }
    }
}

void Boss::PatternGroundSlam(float dt) {
    patternTime_ += dt;

    if (slamStep_ == 0) {
        Vector2 targetPos = { 640.0f, -150.0f };
        if (target_) targetPos.x = target_->GetPos().x;

        position_ = LerpVec2(position_, targetPos, 0.1f);

        if (patternTime_ > 0.5f) {
            slamStep_ = 1;
            patternTime_ = 0.0f;
        }
    } else if (slamStep_ == 1) {
        position_.y += 1200.0f * dt;

        if (position_.y >= 500.0f) {
            position_.y = 500.0f;
            ShakeScreenHeavy(0.5f);
            if (particleSys_) particleSys_->PlayOneShot("BossImpact", position_, 50);

            if (bulletMgr_) {
                for (int i = 0; i < 20; ++i) {
                    float a = (6.28f / 20.0f) * i;
                    Bullet b; b.pos = position_; b.vel = { cos(a) * 400.0f, sin(a) * 400.0f }; b.isEnemy = true;
                    static BulletLinear lin; b.behavior = &lin;
                    bulletMgr_->Spawn(b, "e_fire");
                }
            }

            slamStep_ = 2;
            patternTime_ = 0.0f;
        }
    } else if (slamStep_ == 2) {
        if (patternTime_ > 0.5f) {
            TryNextComboAction();
        }
    }
}

void Boss::PatternScatterShot(float dt) {
    patternTime_ += dt;

    if (scatterStep_ == 0) {
        if (patternTime_ > 0.2f) {
            if (bulletMgr_) {
                for (int k = 0; k < 5; ++k) {
                    float angle = (rand() % 360) * 0.01745f;
                    float speed = 200.0f + (rand() % 200);
                    Bullet b; b.pos = position_; b.vel = { cos(angle) * speed, sin(angle) * speed }; b.isEnemy = true;
                    static BulletLinear lin; b.behavior = &lin;
                    bulletMgr_->Spawn(b, GetBulletStyleForPhase());
                }
            }
            ShakeScreenLight(0.1f);
            scatterStep_++;
            patternTime_ = 0.0f;
        }
    } else if (scatterStep_ < 5) {
        if (patternTime_ > 0.15f) {
            scatterStep_ = 0;
            patternTime_ = 0.0f;
        }
    } else {
        TryNextComboAction();
    }
}

void Boss::PatternApocalypse(float dt) {
    patternTime_ += dt;
    if (apocalypseStep_ == 0) {
        if (patternTime_ > 1.0f) {
            apocalypseStep_ = 1; patternTime_ = 0.0f;
            ShakeScreenHeavy(2.0f);
        }
    } else if (apocalypseStep_ == 1) {
        apocalypseAngle_ += dt * 4.0f;
        static float t = 0; t += dt;
        if (t > 0.05f) {
            t = 0;
            if (bulletMgr_) {
                for (int i = 0; i < 4; ++i) {
                    float a = apocalypseAngle_ + i * 1.57f;
                    Bullet b; b.pos = position_; b.vel = { cos(a) * 400.f, sin(a) * 400.f }; b.isEnemy = true;
                    static BulletLinear lin; b.behavior = &lin;
                    bulletMgr_->Spawn(b, "e_fire");
                }
                for (int i = 0; i < 4; ++i) {
                    float a = -apocalypseAngle_ + i * 1.57f + 0.78f;
                    Bullet b; b.pos = position_; b.vel = { cos(a) * 300.f, sin(a) * 300.f }; b.isEnemy = true;
                    static BulletLinear lin; b.behavior = &lin;
                    bulletMgr_->Spawn(b, "e_ice");
                }
            }
        }
        if (patternTime_ > 5.0f) {
            apocalypseStep_ = 2; patternTime_ = 0.0f;
        }
    } else if (apocalypseStep_ == 2) {
        if (patternTime_ > 0.5f) TryNextComboAction();
    }
}

void Boss::PatternRingShot(float dt) {
    patternTime_ += dt;
    if (patternTime_ > 0.15f) {
        patternTime_ = 0.0f;
        ringStep_++;
        if (bulletMgr_) {
            int count = 24;
            float step = 6.283f / count;
            for (int i = 0; i < count; ++i) {
                float a = i * step + ringStep_ * 0.1f;
                Bullet b; b.pos = position_; b.vel = { cos(a) * 280.f, sin(a) * 280.f }; b.isEnemy = true;
                static BulletLinear l; b.behavior = &l;
                bulletMgr_->Spawn(b, GetBulletStyleForPhase());
            }
            ShakeScreenLight(0.1f);
        }
        if (ringStep_ >= 6) TryNextComboAction();
    }
}

void Boss::PatternFanShot(float dt) {
    patternTime_ += dt;
    if (patternTime_ > 0.3f) {
        patternTime_ = 0.0f;
        fanShotStep_++;
        if (bulletMgr_ && target_) {
            float baseA = std::atan2(target_->GetPos().y - position_.y, target_->GetPos().x - position_.x);
            int count = 15;
            float spread = 1.2f;
            for (int i = 0; i < count; ++i) {
                float a = baseA - spread * 0.5f + (spread / (count - 1)) * i;
                Bullet b; b.pos = position_; b.vel = { cos(a) * 350.f, sin(a) * 350.f }; b.isEnemy = true;
                static BulletLinear l; b.behavior = &l;
                bulletMgr_->Spawn(b, GetBulletStyleForPhase());
            }
            ShakeScreenLight(0.1f);
        }
        if (fanShotStep_ >= 3) TryNextComboAction();
    }
}
void Boss::PatternCrossBurst(float dt) {
    patternTime_ += dt;
    if (patternTime_ > 0.25f) {
        patternTime_ = 0; crossBurstStep_++;
        if (bulletMgr_) {
            int c = 16;
            for (int i = 0; i < c; ++i) {
                float a = i * (6.28f / c) + crossBurstStep_ * 0.2f;
                Bullet b; b.pos = position_; b.vel = { cos(a) * 320.f, sin(a) * 320.f }; b.isEnemy = true;
                static BulletLinear l; b.behavior = &l; bulletMgr_->Spawn(b, GetBulletStyleForPhase());
            }
            ShakeScreenLight(0.1f);
        }
        if (crossBurstStep_ >= 3) TryNextComboAction();
    }
}
void Boss::PatternIcicleRain(float dt) {
    patternTime_ += dt; icicleSpawnTimer_ -= dt;
    if (icicleSpawnTimer_ <= 0) {
        icicleSpawnTimer_ = 0.05f;
        if (bulletMgr_) {
            float x = 50.f + rand() % 1180;
            Bullet b; b.pos = { x,-50 }; b.vel = { 0, 450.f + rand() % 100 }; b.isEnemy = true;
            static BulletLinear l; b.behavior = &l; bulletMgr_->Spawn(b, "e_ice");
        }
    }
    if (patternTime_ > 2.5f) TryNextComboAction();
}
void Boss::PatternIceMissile(float dt) {
    patternTime_ += dt;
    if (iceMissileTotal_ == 0) iceMissileTotal_ = 5;
    if (patternTime_ > 0.2f) {
        patternTime_ = 0; iceMissileStep_++;
        if (bulletMgr_ && target_) {
            Vector2 d = Normalize(target_->GetPos() - position_);
            Bullet b; b.pos = position_; b.vel = d * 200.f; b.acc = d * 150.f; b.isEnemy = true;
            static BulletAccel l; b.behavior = &l; bulletMgr_->Spawn(b, "e_ice");
            ShakeScreenLight(0.1f);
        }
        if (iceMissileStep_ >= iceMissileTotal_) TryNextComboAction();
    }
}
void Boss::PatternFireSpiral(float dt) {
    patternTime_ += dt; fireSpiralTimer_ -= dt;
    if (fireSpiralTimer_ <= 0) {
        fireSpiralTimer_ = 0.06f;
        fireSpiralAngle_ += 0.4f;
        if (bulletMgr_) {
            for (int i = 0; i < 4; ++i) {
                float a = fireSpiralAngle_ + i * 1.57f;
                Bullet b; b.pos = position_; b.vel = { cos(a) * 300.f, sin(a) * 300.f }; b.isEnemy = true;
                static BulletLinear l; b.behavior = &l; bulletMgr_->Spawn(b, "e_fire");
            }
        }
    }
    if (patternTime_ > 2.5f) TryNextComboAction();
}
void Boss::PatternFlameRushBurst(float dt) {
    patternTime_ += dt;
    if (flameRushStep_ == 0) {
        position_.y -= 30.f * dt;
        if (patternTime_ > 0.6f) {
            flameRushStep_ = 1; patternTime_ = 0;
            if (target_) rushDir_ = Normalize(target_->GetPos() - position_); else rushDir_ = { 0,1 };
            ShakeScreenHeavy(0.4f);
        }
    } else if (flameRushStep_ == 1) {
        position_ += rushDir_ * rushSpeed_ * dt;
        if (position_.x < -100 || position_.x>1380 || position_.y > 800 || patternTime_ > 1.5f) {
            flameRushStep_ = 2; patternTime_ = 0; flameBurstRemaining_ = 3;
            position_ = { 640,200 }; ShakeScreenHeavy(0.5f);
            if (particleSys_) particleSys_->PlayOneShot("BossImpact", position_, 1);
        }
    } else if (flameRushStep_ == 2) {
        if (patternTime_ > 0.25f) {
            patternTime_ = 0; flameBurstRemaining_--;
            if (bulletMgr_) {
                int c = 30;
                for (int i = 0; i < c; ++i) {
                    float a = i * (6.28f / c);
                    Bullet b; b.pos = position_; b.vel = { cos(a) * 400.f, sin(a) * 400.f }; b.isEnemy = true;
                    static BulletLinear l; b.behavior = &l; bulletMgr_->Spawn(b, "e_fire");
                }
                ShakeScreenLight(0.2f);
            }
            if (flameBurstRemaining_ <= 0) TryNextComboAction();
        }
    }
}
void Boss::PatternIceSkateRush(float dt) {
    patternTime_ += dt;
    if (iceSkateStep_ == 0) {
        position_.y -= 20.f * dt;
        if (patternTime_ > 0.5f) {
            iceSkateStep_ = 1; patternTime_ = 0;
            if (target_) rushDir_ = Normalize(target_->GetPos() - position_); else rushDir_ = { 0,1 };
            ShakeScreenHeavy(0.3f);
        }
    } else if (iceSkateStep_ == 1) {
        position_ += rushDir_ * rushSpeed_ * dt;
        iceSkateDropTimer_ -= dt;
        if (iceSkateDropTimer_ <= 0) {
            iceSkateDropTimer_ = 0.05f;
            if (bulletMgr_) {
                Vector2 dPos = position_ - rushDir_ * 40.f;
                Bullet b; b.pos = dPos;
                float a = std::atan2(rushDir_.y, rushDir_.x) + 3.14f + (rand() % 100 - 50) * 0.01f;
                b.vel = { cos(a) * 150.f, sin(a) * 150.f }; b.isEnemy = true;
                static BulletLinear l; b.behavior = &l; bulletMgr_->Spawn(b, "e_ice");
            }
        }
        if (position_.x < -100 || position_.x>1380 || position_.y > 800 || patternTime_ > 2.0f) TryNextComboAction();
    }
}

std::string Boss::GetBulletStyleForPhase() const {
    switch (currentGlobalPhase_) {
    case GlobalPhase::Ice: return "e_ice";
    case GlobalPhase::Fire: return "e_fire";
    case GlobalPhase::Final: return "e_fire";
    case GlobalPhase::Normal: default: return "e_normal";
    }
}

void Boss::Draw() {
    bool useIceFire = (iceProgress_ > 0.0f || fireProgress_ > 0.0f);
    bool useHit = (hitFlashTimer_ > 0.0f);

    if (useHit) HIKARI::POST::PostSystem::BeginLayer(hitChain_, 0, 0, 0, 0);
    if (useIceFire) HIKARI::POST::PostSystem::BeginLayer(bossChain_, 0, 0, 0, 0);

    spine_.Draw();

    if (useIceFire) HIKARI::POST::PostSystem::EndLayer();
    if (useHit) HIKARI::POST::PostSystem::EndLayer();
}

void Boss::TakeDamage(float amount) {
    if (state_ == BossState::Intro || state_ == BossState::Transitioning || state_ == BossState::Dead) return;
    hp_ -= amount;
    hitFlashTimer_ = 0.8f;
    if (state_ != BossState::Intro && state_ != BossState::Dead) {
        anim_.PlayShakeEx(10.0f, 2.0f, 0.0f, 10.0f, 8.0f, 0.0f, 0.1f, HIKARI::ANIM::EASE::OutQuad);
    }
    hurtTimer_ = 0.1f;
    if (hp_ <= 0) { hp_ = 0; state_ = BossState::Dead; }
}

RectF Boss::GetAABB() const {
    return { position_.x - size_.x / 2, position_.y - size_.y / 2, size_.x, size_.y };
}

bool Boss::IsDead() const {
    return state_ == BossState::Dead || hp_ <= 0.0f;
}