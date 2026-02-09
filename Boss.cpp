#include "Boss.h"
#include "Player.h"
#include "BulletManager.h"
#include "BulletBehaviors_Basic.h" // 假设有这个
#include <cmath>
#include <algorithm>
#include "Vector2.h"


// 辅助函数
static float EaseInOutQuad(float t) { return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f; }


Boss::Boss() {
    spine_.Load("./Animation/boss.atlas", "./Animation/boss.json");
    spine_.SetAnimation("intro1", true);
    spine_.transform.position = { 640.0f,360.0f };
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
void Boss::Init(Player* target, BulletManager* bulletMgr) {
    target_ = target;
    bulletMgr_ = bulletMgr;
    InitPostEffects();
    // 默认先应用一次 Normal，防止数值未初始化
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

    // 重置阶段触发器
    for (int i = 0; i < 4; i++) phaseTriggered_[i] = false;
}

void Boss::SetDifficulty(DifficultyLevel level) {
    difficulty_ = level;

    switch (level) {
    case DifficultyLevel::Challenge:
        maxHp_ = 50000.0f;          // 更高血量
        speedMultiplier_ = 1.1f;    // 原速 (或者更快 1.1f)
        introDuration_ = 2.5f;      // 入场更快
        moveSpeed_ = 90.0f;         // 移动更快
        rushSpeed_ = 1400.0f;       // 冲撞极快
        normalDuration_ = 3.0f;     // 攻击欲望强 (3秒平A就放技能)
        break;
    case DifficultyLevel::Normal:
    default:
        maxHp_ = 3000.0f;
        speedMultiplier_ = 1.0f;    // 整体动作变慢 (原版逻辑)
        introDuration_ = 5.5f;      // 给了很长的入场展示时间
        moveSpeed_ = 80.0f;
        rushSpeed_ = 1000.0f;
        normalDuration_ = 5.0f;
        break;
    }

    hp_ = maxHp_;

    // 重新初始化技能池，因为技能权重或连招可能随难度变化
    InitModes();
}

void Boss::InitModes() {
    modes_.clear();

    // --- Melee Form ---
    ModeDef meleeMode;
    meleeMode.name = "Melee Form";
    meleeMode.switchAnim = "switchPhase1";
    meleeMode.modeIdleAnim = "idle1";
    meleeMode.returnAnim = "returnPhase1";
    meleeMode.weight = 50;
    meleeMode.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Ice, GlobalPhase::Fire };
#if 0
    meleeMode.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Ice, GlobalPhase::Fire, GlobalPhase::Final };
#endif

    // Skill: Rush Combo
    SkillDef rushCombo;
    rushCombo.name = "Rush Combo";
    rushCombo.weight = 35;
    rushCombo.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Ice };

    // ★ 难度差异：Challenge 模式下 Rush 连撞3次，Normal 撞2次
    if (difficulty_ == DifficultyLevel::Challenge) {
        rushCombo.sequence = {
            { ActionPattern::Rush, 0.0f },
            { ActionPattern::Wait, 0.3f }, // 等待更短
            { ActionPattern::Rush, 0.0f },
            { ActionPattern::Wait, 0.3f },
            { ActionPattern::Rush, 0.0f }
        };
    } else {
        rushCombo.sequence = {
            { ActionPattern::Rush, 0.0f },
            { ActionPattern::Wait, 0.5f },
            { ActionPattern::Rush, 0.0f }
        };
    }
    meleeMode.skillPool.push_back(rushCombo);

    // Skill: Flame Rush Burst (Normal)
    SkillDef flameRushBurst;
    flameRushBurst.name = "Flame Rush Burst";
    flameRushBurst.weight = 25;
    flameRushBurst.allowedGlobalPhases = { GlobalPhase::Normal };
    flameRushBurst.sequence = { { ActionPattern::FlameRushBurst, 0.0f } };
    meleeMode.skillPool.push_back(flameRushBurst);

    // Skill: Ice Skate Rush
    SkillDef iceSkateRush;
    iceSkateRush.name = "Ice Skate Rush";
    iceSkateRush.weight = 20;
    iceSkateRush.allowedGlobalPhases = { GlobalPhase::Ice };
    iceSkateRush.sequence = { { ActionPattern::IceSkateRush, 0.0f } };
    meleeMode.skillPool.push_back(iceSkateRush);

    // Skill: Flame Rush Burst (Fire)
    SkillDef flameRushBurstFire;
    flameRushBurstFire.name = "Flame Rush Burst+";
    flameRushBurstFire.weight = 30;
    flameRushBurstFire.allowedGlobalPhases = { GlobalPhase::Fire };
    flameRushBurstFire.sequence = { { ActionPattern::FlameRushBurst, 0.0f } };
    meleeMode.skillPool.push_back(flameRushBurstFire);
    modes_.push_back(meleeMode);

    // --- Magic Form ---
    ModeDef magicMode;
    magicMode.name = "Magic Form";
    magicMode.switchAnim = "switchPhase2";
    magicMode.modeIdleAnim = "idle2";
    magicMode.returnAnim = "returnPhase2";
    magicMode.weight = 50;
    magicMode.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Ice, GlobalPhase::Fire };
#if 0
    magicMode.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Ice, GlobalPhase::Fire, GlobalPhase::Final };
#endif

    // Skill: Fan Shot (Normal)
    SkillDef fanShot;
    fanShot.name = "Fan Shot";
    fanShot.weight = 25;
    fanShot.allowedGlobalPhases = { GlobalPhase::Normal };
    fanShot.sequence = { { ActionPattern::FanShot, 0.0f } };
    magicMode.skillPool.push_back(fanShot);

    // Skill: Cross Burst
    SkillDef crossBurst;
    crossBurst.name = "Cross Burst";
    crossBurst.weight = 20;
    crossBurst.allowedGlobalPhases = { GlobalPhase::Normal };
    crossBurst.sequence = { { ActionPattern::CrossBurst, 0.0f } };
    magicMode.skillPool.push_back(crossBurst);

    // Skill: Ring Shot (Normal)
    SkillDef ringSkill;
    ringSkill.name = "Ring Barrage";
    ringSkill.weight = 20;
    ringSkill.allowedGlobalPhases = { GlobalPhase::Normal };
    ringSkill.sequence = { { ActionPattern::RingShot, 0.0f } };
    magicMode.skillPool.push_back(ringSkill);

    // Skill: Icicle Rain
    SkillDef icicleRain;
    icicleRain.name = "Icicle Rain";
    icicleRain.weight = 35;
    icicleRain.allowedGlobalPhases = { GlobalPhase::Ice };
    icicleRain.sequence = { { ActionPattern::IcicleRain, 0.0f } };
    magicMode.skillPool.push_back(icicleRain);

    // Skill: Ice Missiles
    SkillDef iceMissile;
    iceMissile.name = "Ice Missiles";
    iceMissile.weight = 30;
    iceMissile.allowedGlobalPhases = { GlobalPhase::Ice };
    iceMissile.sequence = { { ActionPattern::IceMissile, 0.0f } };
    magicMode.skillPool.push_back(iceMissile);

    // Skill: Ring Shot (Ice)
    SkillDef ringSkillIce;
    ringSkillIce.name = "Ring Barrage (Ice)";
    ringSkillIce.weight = 15;
    ringSkillIce.allowedGlobalPhases = { GlobalPhase::Ice };
    ringSkillIce.sequence = { { ActionPattern::RingShot, 0.0f } };
    magicMode.skillPool.push_back(ringSkillIce);

    // Skill: Fire Spiral
    SkillDef fireSpiral;
    fireSpiral.name = "Fire Spiral";
    fireSpiral.weight = 40;
    fireSpiral.allowedGlobalPhases = { GlobalPhase::Fire };
    fireSpiral.sequence = { { ActionPattern::FireSpiral, 0.0f } };
    magicMode.skillPool.push_back(fireSpiral);

    // Skill: Fan Shot (Fire)
    SkillDef fanShotFire;
    fanShotFire.name = "Fan Shot (Fire)";
    fanShotFire.weight = 20;
    fanShotFire.allowedGlobalPhases = { GlobalPhase::Fire };
    fanShotFire.sequence = { { ActionPattern::FanShot, 0.0f } };
    magicMode.skillPool.push_back(fanShotFire);

    // Skill: Ring Shot (Fire)
    SkillDef ringSkillFire;
    ringSkillFire.name = "Ring Barrage (Fire)";
    ringSkillFire.weight = 10;
    ringSkillFire.allowedGlobalPhases = { GlobalPhase::Fire };
    ringSkillFire.sequence = { { ActionPattern::RingShot, 0.0f } };
    magicMode.skillPool.push_back(ringSkillFire);
    modes_.push_back(magicMode);
}

void Boss::Update(float dt) {

    float adjustedDt = dt * speedMultiplier_;


    if (hurtTimer_ > 0.0f) hurtTimer_ -= dt;
    spine_.transform.position = position_;
    anim_.Update(dt);
    spine_.Update(adjustedDt);

    UpdateState(adjustedDt);

    // --- 冰逻辑 ---
    if (currentGlobalPhase_ == GlobalPhase::Ice) {
        iceProgress_ += dt * 0.8f; // 冰冻速度
    } else {
        iceProgress_ -= dt * 1.0f; // 快速解冻
    }
    iceProgress_ = std::clamp(iceProgress_, 0.0f, 1.0f);

    // --- 火逻辑 ---
    if (currentGlobalPhase_ == GlobalPhase::Fire) {
        fireProgress_ += dt * 0.8f; // 燃烧速度
    } else {
        fireProgress_ -= dt * 1.0f; // 熄灭
    }
    fireProgress_ = std::clamp(fireProgress_, 0.0f, 1.0f);

    // 计算 Boss 的 UV 坐标 (0~1)
    float screenW = 1280.0f; float screenH = 720.0f;
    DirectX::XMFLOAT4 p1;
    // 假设 Spine 锚点在脚下，稍微往上一点取中心
    p1.x = position_.x / screenW;
    p1.y = (position_.y - size_.y * 0.5f) / screenH;

    // --- 传参给 Shader ---
    if (fxIce_) {
        DirectX::XMFLOAT4 p0; p0.x = iceProgress_;
        fxIce_->SetUser(0, p0);
        fxIce_->SetUser(1, p1); // 传入 Boss 位置
    }
    if (fxFire_) {
        DirectX::XMFLOAT4 p0; p0.x = fireProgress_;
        fxFire_->SetUser(0, p0);
        fxFire_->SetUser(1, p1); // 传入 Boss 位置
    }

    if (hitFlashTimer_ > 0.0f) {
        hitFlashTimer_ -= dt * 5.0f; // Boss 闪得稍微慢一点，显眼一点
        if (hitFlashTimer_ < 0.0f) hitFlashTimer_ = 0.0f;

        if (fxHit_) {
            DirectX::XMFLOAT4 p;
            p.x = hitFlashTimer_;
            // Boss 受击颜色：可以带点红 (1.0, 0.5, 0.5)
            p.y = 1.0f; p.z = 0.5f; p.w = 0.5f;
            fxHit_->SetUser(0, p);
        }
    }
}

void Boss::UpdateState(float dt) {
    stateTimer_ += dt;

    if (state_ != BossState::Dead && state_ != BossState::Transitioning && state_ != BossState::Intro) {
        CheckGlobalPhase();
    }

    switch (state_) {
        // =================================================================
        // ★ 还原原版入场逻辑
        // =================================================================
    case BossState::Intro:
        if (!introAnimPlayed_) {
            // 播放入场动画
            spine_.SetAnimation("intro1", false);
            spine_.AddAnimation("idle", true, 0.0f); // 播完切idle

            // 震屏
            HIKARI::CAMERA::ShakeParams shake{};
            shake.freqX = 150.0f; shake.freqY = 20.0f; shake.durationSec = 2.0f;
            shake.ampX = 35.0f; shake.ampY = 0.5f; shake.ampRot = 0.0f;
            shake.envelope = HIKARI::ANIM::EASE::InOutQuad;
            HIKARI::CAMERA::ShakeEx(shake);

            // 手柄震动
            HIKARI::HINPUT::SetPadVibration(2.0f, 2.0f, 3.0f);

            introAnimPlayed_ = true;
            position_ = { 640.0f, 200.0f };
        }

        // 等待时间结束
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
        if (stateTimer_ > 1.5f) {
            state_ = BossState::Execute;
            stateTimer_ = 0.0f;
            if (activeModeIndex_ >= 0) {
                spine_.SetAnimation(modes_[activeModeIndex_].modeIdleAnim.c_str(), true);
            }
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
                position_ = Lerp(easeStartPos_, easeTargetPos_, EaseInOutQuad(t));
                if (t >= 1.0f) {
                    pattern_ = easeNextPattern_;
                    patternTime_ = 0.0f;
                    if (pattern_ == ActionPattern::Rush) rushStep_ = 0;
                    if (pattern_ == ActionPattern::RingShot) ringStep_ = 0;
                    if (pattern_ == ActionPattern::FanShot) fanShotStep_ = 0;
                    if (pattern_ == ActionPattern::CrossBurst) crossBurstStep_ = 0;
                    if (pattern_ == ActionPattern::IcicleRain) { icicleSpawnTimer_ = 0.0f; }
                    if (pattern_ == ActionPattern::IceMissile) { iceMissileStep_ = 0; iceMissileTotal_ = 0; }
                    if (pattern_ == ActionPattern::FireSpiral) { fireSpiralTimer_ = 0.0f; fireSpiralAngle_ = 0.0f; }
                    if (pattern_ == ActionPattern::FlameRushBurst) { flameRushStep_ = 0; flameBurstRemaining_ = 0; }
                    if (pattern_ == ActionPattern::IceSkateRush) { iceSkateStep_ = 0; iceSkateDropTimer_ = 0.0f; }
                }
            } else if (pattern_ == ActionPattern::Rush) PatternRush(dt);
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
        // 回归时的平滑移动
        if (pattern_ == ActionPattern::Changing) {
            easeTime_ += dt;
            float t = easeTime_ / easeDuration_;
            if (t >= 1.0f) t = 1.0f;
            position_ = Lerp(easeStartPos_, easeTargetPos_, EaseInOutQuad(t));
            if (t >= 1.0f) {
                pattern_ = easeNextPattern_;
                patternTime_ = 0.0f;
            }
        }

        if (stateTimer_ > 1.0f && pattern_ != ActionPattern::Changing) {
            state_ = BossState::Normal;
            stateTimer_ = 0.0f;
            activeModeIndex_ = -1;
            normalMoveTime_ = 0.0f;
            spine_.SetAnimation("idle", true);
        }
        break;

    case BossState::Transitioning:
    {

        Vector2 centerTarget = { 640.0f, 200.0f }; 


        float t = stateTimer_ * 2.0f;
        if (t > 1.0f) t = 1.0f;
        position_ = Lerp(position_, centerTarget, 0.1f); 


        if (std::abs(position_.x - centerTarget.x) < 5.0f && std::abs(position_.y - centerTarget.y) < 5.0f) {
            position_ = centerTarget;
        }


        if (stateTimer_ > 1.0f && stateTimer_ < 1.1f) {


            HIKARI::CAMERA::ShakeParams shake{};
            shake.durationSec = 1.0f; shake.ampX = 10.0f;
            HIKARI::CAMERA::ShakeEx(shake);
        }

        // 阶段 3: 恢复行动 (3.0s 后)
        if (stateTimer_ > 3.0f) {
            state_ = BossState::Normal;
            stateTimer_ = 0.0f;
            spine_.SetAnimation("idle", true);

            normalMoveTime_ = 0.0f;
        }
        break;
    }

    case BossState::Dead:
        break;
    }
}


void Boss::CheckGlobalPhase() {
    float p = hp_ / maxHp_;
    GlobalPhase next = currentGlobalPhase_;

    // 检查阈值
    if (!phaseTriggered_[1] && p <= 0.70f) { next = GlobalPhase::Ice; phaseTriggered_[1] = true; } else if (!phaseTriggered_[2] && p <= 0.40f) { next = GlobalPhase::Fire; phaseTriggered_[2] = true; }
#if 0
    else if (!phaseTriggered_[3] && p <= 0.10f) { next = GlobalPhase::Final; phaseTriggered_[3] = true; }
#endif

    if (next != currentGlobalPhase_) {
        currentGlobalPhase_ = next;

        // 进入转场状态
        state_ = BossState::Transitioning;
        stateTimer_ = 0.0f;
        velocity_ = { 0,0 };


        executionQueue_.clear();          // 清空技能队列
        waitingForCombo_ = false;         // 清除等待标记 (最重要！)
        pattern_ = ActionPattern::Idle;   // 重置当前动作

        // 如果处于变身状态，立刻切回普通形态
        if (activeModeIndex_ >= 0) {
            spine_.SetAnimation("idle", true);
            activeModeIndex_ = -1;
        }

        // 通知外部
        if (onPhaseChange_) onPhaseChange_(currentGlobalPhase_);
    }
}


void Boss::UpdateNormalBehavior(float dt) {
    normalMoveTime_ += dt;
    shootTimer_ -= dt;

    float baseDuration = 4.5f;
    if (currentGlobalPhase_ == GlobalPhase::Ice) baseDuration = 3.5f;
    if (currentGlobalPhase_ == GlobalPhase::Fire) baseDuration = 2.7f;
    if (difficulty_ == DifficultyLevel::Challenge) baseDuration *= 0.75f;
    normalDuration_ = baseDuration;

    // ★ 使用 moveSpeed_ 变量
    float speed = 1.0f;
#if 0
    if (currentGlobalPhase_ == GlobalPhase::Final) speed = 2.0f;
#endif
    float ampX = moveSpeed_ * 2.5f; // 根据 moveSpeed 调整幅度
    float ampY = moveSpeed_ * 0.5f;

    // 8字移动
    position_.x = 640.0f + std::sin(normalMoveTime_ * speed) * ampX;
    position_.y = 200.0f + std::sin(normalMoveTime_ * speed * 2.0f) * ampY;

    if (shootTimer_ <= 0.0f) {
        FireNormalBarrage();
        // 冷却时间
        float baseCd = 1.2f;
        if (currentGlobalPhase_ == GlobalPhase::Ice) baseCd = 1.0f;
        if (currentGlobalPhase_ == GlobalPhase::Fire) baseCd = 0.7f;
#if 0
        if (currentGlobalPhase_ == GlobalPhase::Final) baseCd = 0.4f;
#endif

        // ★ 难度影响射速
        if (difficulty_ == DifficultyLevel::Challenge) baseCd *= 0.7f; // Challenge 射得更快

        shootTimer_ = baseCd;
    }

    if (stateTimer_ > normalDuration_) {
        SelectModeAndSkill();
    }
}


void Boss::FireNormalBarrage() {
    if (!bulletMgr_ || !target_) return;
    const std::string styleName = GetBulletStyleForPhase();
    if (currentGlobalPhase_ == GlobalPhase::Normal) {
        Vector2 dir = Normalize(target_->GetPos() - position_);
        Bullet b; b.pos = position_; b.vel = dir * 400.0f; b.isEnemy = true;
        static BulletLinear lin; b.behavior = &lin;
        bulletMgr_->Spawn(b, styleName);
    } else if (currentGlobalPhase_ == GlobalPhase::Ice) {
        float angle = std::atan2(target_->GetPos().y - position_.y, target_->GetPos().x - position_.x);
        for (int i = -1; i <= 1; ++i) {
            float a = angle + i * 0.3f;
            Bullet b; b.pos = position_; b.vel = { std::cos(a) * 350.f, std::sin(a) * 350.f }; b.isEnemy = true;
            static BulletLinear lin; b.behavior = &lin;
            bulletMgr_->Spawn(b, styleName);
        }
    } else {
        static float spinA = 0.0f; spinA += 0.5f;
        Bullet b; b.pos = position_; b.vel = { std::cos(spinA) * 500.f, std::sin(spinA) * 500.f }; b.isEnemy = true;
        static BulletLinear lin; b.behavior = &lin;
        bulletMgr_->Spawn(b, styleName);
    }
}

void Boss::SelectModeAndSkill() {
    std::vector<int> validModeIndices;
    float totalWeight = 0.0f;
    for (int i = 0; i < modes_.size(); ++i) {
        bool allowed = modes_[i].allowedGlobalPhases.empty();
        for (auto p : modes_[i].allowedGlobalPhases) {
            if (p == currentGlobalPhase_) { allowed = true; break; }
        }
        if (allowed && modes_[i].weight > 0) {
            validModeIndices.push_back(i);
            totalWeight += static_cast<float>(modes_[i].weight);
        }
    }
    if (validModeIndices.empty()) { state_ = BossState::Normal; stateTimer_ = 0.0f; return; }
    float r = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * totalWeight;
    float currentSum = 0.0f;
    int selectedIdx = validModeIndices[0];
    for (int idx : validModeIndices) {
        currentSum += static_cast<float>(modes_[idx].weight);
        if (r < currentSum) { selectedIdx = idx; break; }
    }
    activeModeIndex_ = selectedIdx;
    const auto& mode = modes_[activeModeIndex_];
    if (mode.skillPool.empty()) { state_ = BossState::Normal; return; }

    std::vector<int> validSkillIndices;
    std::vector<float> validSkillWeights;
    float totalSkillWeight = 0.0f;
    for (int i = 0; i < mode.skillPool.size(); ++i) {
        const auto& skill = mode.skillPool[i];
        bool allowed = skill.allowedGlobalPhases.empty();
        for (auto p : skill.allowedGlobalPhases) {
            if (p == currentGlobalPhase_) { allowed = true; break; }
        }
        if (!allowed || skill.weight <= 0) continue;
        float effectiveWeight = static_cast<float>(skill.weight);
        if (!lastSkillName_.empty() && skill.name == lastSkillName_) {
            effectiveWeight *= 0.2f;
        }
        if (effectiveWeight <= 0.0f) continue;
        validSkillIndices.push_back(i);
        validSkillWeights.push_back(effectiveWeight);
        totalSkillWeight += effectiveWeight;
    }
    if (validSkillIndices.empty()) { state_ = BossState::Normal; stateTimer_ = 0.0f; return; }

    float rSkill = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * totalSkillWeight;
    float skillSum = 0.0f;
    int selectedSkillIdx = validSkillIndices[0];
    for (int i = 0; i < validSkillIndices.size(); ++i) {
        skillSum += validSkillWeights[i];
        if (rSkill < skillSum) { selectedSkillIdx = validSkillIndices[i]; break; }
    }
    const auto& skill = mode.skillPool[selectedSkillIdx];
    lastSkillName_ = skill.name;

    executionQueue_.clear();
    for (const auto& node : skill.sequence) { executionQueue_.push_back(node); }
    state_ = BossState::Switching; stateTimer_ = 0.0f;
    spine_.SetAnimation(mode.switchAnim.c_str(), false);
    RequestPattern(ActionPattern::Idle, true);
}

void Boss::TryNextComboAction() {
    if (executionQueue_.empty()) {
        state_ = BossState::Returning; stateTimer_ = 0.0f;
        if (activeModeIndex_ >= 0) spine_.SetAnimation(modes_[activeModeIndex_].returnAnim.c_str(), false);
        RequestPattern(ActionPattern::Idle, true);
        return;
    }
    SkillNode node = executionQueue_.front();
    executionQueue_.pop_front();
    if (node.action == ActionPattern::Wait) { waitingForCombo_ = true; comboTimer_ = node.duration; } else {
        RequestPattern(node.action, true);
        if (node.duration > 0.0f) { waitingForCombo_ = true; comboTimer_ = node.duration; }
    }
}

void Boss::RequestPattern(ActionPattern next, bool ease) {
    if (ease) {
        pattern_ = ActionPattern::Changing;
        easeStartPos_ = position_;
        easeNextPattern_ = next;
        easeDuration_ = 0.6f;
        easeTime_ = 0.0f;
        if (next == ActionPattern::Rush) easeTargetPos_ = { 640.0f, 200.0f };
        else if (next == ActionPattern::RingShot) easeTargetPos_ = { 640.0f, 300.0f };
        else if (next == ActionPattern::FanShot || next == ActionPattern::CrossBurst) {
            float y = 260.0f + static_cast<float>(rand() % 61);
            easeTargetPos_ = { 640.0f, y };
        } else if (next == ActionPattern::IcicleRain) {
            float y = 140.0f + static_cast<float>(rand() % 41);
            easeTargetPos_ = { 640.0f, y };
        } else if (next == ActionPattern::IceMissile) {
            easeTargetPos_ = { 640.0f, 220.0f };
        } else if (next == ActionPattern::FireSpiral) {
            easeTargetPos_ = { 640.0f, 260.0f };
        } else if (next == ActionPattern::FlameRushBurst || next == ActionPattern::IceSkateRush) {
            easeTargetPos_ = { 640.0f, 200.0f };
        }
        else easeTargetPos_ = { 640.0f, 230.0f }; // Idle return position
    } else {
        pattern_ = next;
        if (next == ActionPattern::Rush) rushStep_ = 0;
        if (next == ActionPattern::RingShot) ringStep_ = 0;
        if (next == ActionPattern::FanShot) fanShotStep_ = 0;
        if (next == ActionPattern::CrossBurst) crossBurstStep_ = 0;
        if (next == ActionPattern::IcicleRain) icicleSpawnTimer_ = 0.0f;
        if (next == ActionPattern::IceMissile) { iceMissileStep_ = 0; iceMissileTotal_ = 0; }
        if (next == ActionPattern::FireSpiral) { fireSpiralTimer_ = 0.0f; fireSpiralAngle_ = 0.0f; }
        if (next == ActionPattern::FlameRushBurst) { flameRushStep_ = 0; flameBurstRemaining_ = 0; }
        if (next == ActionPattern::IceSkateRush) { iceSkateStep_ = 0; iceSkateDropTimer_ = 0.0f; }
    }
}

// ... Rush 技能使用变量 ...
void Boss::PatternRush(float dt) {
    patternTime_ += dt;
    if (rushStep_ == 0) {
        position_.y -= 20.0f * dt;
        float shake = std::sin(patternTime_ * 50.0f) * 5.0f;
        spine_.transform.position.x = position_.x + shake;
        if (patternTime_ > 0.8f) {
            rushStep_ = 1; patternTime_ = 0.0f;
            if (target_) rushDir_ = Normalize(target_->GetPos() - position_);
            else rushDir_ = { 0, 1 };
        }
    } else if (rushStep_ == 1) {
        // ★ 使用 rushSpeed_ 变量
        position_ += rushDir_ * rushSpeed_ * dt;
        if (position_.x < -100 || position_.x > 1380 || position_.y > 800) {
            waitingForCombo_ = false; TryNextComboAction();
        }
    }
}

// ... PatternRingShot ...
void Boss::PatternRingShot(float dt) {
    patternTime_ += dt;
    float interval = (difficulty_ == DifficultyLevel::Challenge) ? 0.15f : 0.2f;
    if (patternTime_ > interval) {
        patternTime_ -= interval;
        ringStep_++;
        if (bulletMgr_) {
            int count = 20;
            float step = 6.28318f / count;
            float offset = ringStep_ * 0.1f;
            for (int i = 0; i < count; ++i) {
                float a = i * step + offset;
                Bullet b; b.pos = position_; b.vel = { std::cos(a) * 300.f, std::sin(a) * 300.f }; b.isEnemy = true;
                static BulletLinear lin; b.behavior = &lin;
                bulletMgr_->Spawn(b, GetBulletStyleForPhase());
            }
        }
        int waves = (difficulty_ == DifficultyLevel::Challenge) ? 8 : 5;
        if (ringStep_ >= waves) TryNextComboAction();
    }
}

void Boss::PatternFanShot(float dt) {
    patternTime_ += dt;
    float interval = (currentGlobalPhase_ == GlobalPhase::Fire) ? 0.3f : 0.35f;
    if (patternTime_ >= interval) {
        patternTime_ -= interval;
        fanShotStep_++;

        if (bulletMgr_ && target_) {
            float angle = std::atan2(target_->GetPos().y - position_.y, target_->GetPos().x - position_.x);
            int count = (currentGlobalPhase_ == GlobalPhase::Fire) ? 9 : 10;
            float spread = (currentGlobalPhase_ == GlobalPhase::Fire) ? 1.2f : 0.9f;
            float speed = (currentGlobalPhase_ == GlobalPhase::Fire) ? 420.0f : 320.0f;
            float step = (count > 1) ? (spread / static_cast<float>(count - 1)) : 0.0f;
            float start = angle - spread * 0.5f;
            for (int i = 0; i < count; ++i) {
                float a = start + step * i;
                Bullet b; b.pos = position_; b.vel = { std::cos(a) * speed, std::sin(a) * speed }; b.isEnemy = true;
                static BulletLinear lin; b.behavior = &lin;
                bulletMgr_->Spawn(b, GetBulletStyleForPhase());
            }
        }

        int waves = (currentGlobalPhase_ == GlobalPhase::Fire) ? 2 : 3;
        if (fanShotStep_ >= waves) TryNextComboAction();
    }
}

void Boss::PatternCrossBurst(float dt) {
    patternTime_ += dt;
    float interval = 0.35f;
    if (patternTime_ >= interval) {
        patternTime_ -= interval;
        crossBurstStep_++;

        if (bulletMgr_) {
            int count = 8;
            float speed = 320.0f;
            float base = crossBurstStep_ * 0.2f;
            for (int i = 0; i < count; ++i) {
                float a = base + (6.28318f / count) * i;
                Bullet b; b.pos = position_; b.vel = { std::cos(a) * speed, std::sin(a) * speed }; b.isEnemy = true;
                static BulletLinear lin; b.behavior = &lin;
                bulletMgr_->Spawn(b, GetBulletStyleForPhase());
            }
        }

        if (crossBurstStep_ >= 3) TryNextComboAction();
    }
}

void Boss::PatternIcicleRain(float dt) {
    patternTime_ += dt;
    icicleSpawnTimer_ -= dt;
    const float duration = 2.2f;
    if (icicleSpawnTimer_ <= 0.0f) {
        icicleSpawnTimer_ = (difficulty_ == DifficultyLevel::Challenge) ? 0.07f : 0.09f;
        if (bulletMgr_) {
            float x = 200.0f + static_cast<float>(rand() % 881);
            float drift = -40.0f + static_cast<float>(rand() % 81);
            float speed = 420.0f + static_cast<float>(rand() % 101);
            Bullet b; b.pos = { x, -20.0f }; b.vel = { drift, speed }; b.isEnemy = true;
            static BulletLinear lin; b.behavior = &lin;
            bulletMgr_->Spawn(b, GetBulletStyleForPhase());
        }
    }
    if (patternTime_ >= duration) {
        TryNextComboAction();
    }
}

void Boss::PatternIceMissile(float dt) {
    patternTime_ += dt;
    float interval = (difficulty_ == DifficultyLevel::Challenge) ? 0.22f : 0.26f;
    if (iceMissileTotal_ == 0) {
        iceMissileTotal_ = 3 + (rand() % 3);
    }
    if (patternTime_ >= interval) {
        patternTime_ -= interval;
        if (iceMissileStep_ < iceMissileTotal_) {
            if (bulletMgr_ && target_) {
                Vector2 dir = Normalize(target_->GetPos() - position_);
                Bullet b; b.pos = position_; b.vel = dir * 180.0f; b.acc = dir * 140.0f; b.isEnemy = true;
                static BulletAccel accel; b.behavior = &accel;
                bulletMgr_->Spawn(b, GetBulletStyleForPhase());
            }
            iceMissileStep_++;
        }
        if (iceMissileStep_ >= iceMissileTotal_) {
            TryNextComboAction();
        }
    }
}

void Boss::PatternFireSpiral(float dt) {
    patternTime_ += dt;
    fireSpiralTimer_ -= dt;
    float interval = (difficulty_ == DifficultyLevel::Challenge) ? 0.08f : 0.1f;
    if (fireSpiralTimer_ <= 0.0f) {
        fireSpiralTimer_ = interval;
        if (bulletMgr_) {
            int count = (difficulty_ == DifficultyLevel::Challenge) ? 4 : 3;
            float spread = 0.25f;
            float speed = 300.0f + static_cast<float>(rand() % 81);
            for (int i = 0; i < count; ++i) {
                float a = fireSpiralAngle_ + (i - (count - 1) * 0.5f) * spread;
                Vector2 dir = { std::cos(a), std::sin(a) };
                Bullet b; b.pos = position_; b.vel = dir * speed; b.acc = dir * 60.0f; b.isEnemy = true;
                static BulletAccel accel; b.behavior = &accel;
                bulletMgr_->Spawn(b, GetBulletStyleForPhase());
            }
        }
        fireSpiralAngle_ += 0.35f;
    }
    if (patternTime_ >= 2.0f) {
        TryNextComboAction();
    }
}

void Boss::PatternFlameRushBurst(float dt) {
    patternTime_ += dt;
    if (flameRushStep_ == 0) {
        position_.y -= 20.0f * dt;
        float shake = std::sin(patternTime_ * 50.0f) * 5.0f;
        spine_.transform.position.x = position_.x + shake;
        if (patternTime_ > 0.7f) {
            flameRushStep_ = 1;
            patternTime_ = 0.0f;
            if (target_) rushDir_ = Normalize(target_->GetPos() - position_);
            else rushDir_ = { 0, 1 };
        }
    } else if (flameRushStep_ == 1) {
        position_ += rushDir_ * rushSpeed_ * dt;
        if (position_.x < -100 || position_.x > 1380 || position_.y > 800) {
            flameRushStep_ = 2;
            patternTime_ = 0.0f;
            flameBurstRemaining_ = (currentGlobalPhase_ == GlobalPhase::Fire) ? 2 : 1;
            position_ = { 640.0f, 200.0f };
        }
    } else if (flameRushStep_ == 2) {
        float interval = (currentGlobalPhase_ == GlobalPhase::Fire) ? 0.25f : 0.3f;
        if (patternTime_ >= interval) {
            patternTime_ = 0.0f;
            if (bulletMgr_) {
                int minCount = (currentGlobalPhase_ == GlobalPhase::Fire) ? 24 : 12;
                int maxCount = (currentGlobalPhase_ == GlobalPhase::Fire) ? 32 : 16;
                int count = minCount + (rand() % (maxCount - minCount + 1));
                float step = 6.28318f / static_cast<float>(count);
                for (int i = 0; i < count; ++i) {
                    float a = step * i;
                    Bullet b; b.pos = position_; b.vel = { std::cos(a) * 360.f, std::sin(a) * 360.f }; b.isEnemy = true;
                    static BulletLinear lin; b.behavior = &lin;
                    bulletMgr_->Spawn(b, GetBulletStyleForPhase());
                }
            }
            flameBurstRemaining_--;
            if (flameBurstRemaining_ <= 0) {
                TryNextComboAction();
            }
        }
    }
}

void Boss::PatternIceSkateRush(float dt) {
    patternTime_ += dt;
    if (iceSkateStep_ == 0) {
        position_.y -= 20.0f * dt;
        float shake = std::sin(patternTime_ * 50.0f) * 5.0f;
        spine_.transform.position.x = position_.x + shake;
        if (patternTime_ > 0.7f) {
            iceSkateStep_ = 1;
            patternTime_ = 0.0f;
            iceSkateDropTimer_ = 0.0f;
            if (target_) rushDir_ = Normalize(target_->GetPos() - position_);
            else rushDir_ = { 0, 1 };
        }
    } else if (iceSkateStep_ == 1) {
        position_ += rushDir_ * rushSpeed_ * dt;
        iceSkateDropTimer_ -= dt;
        if (iceSkateDropTimer_ <= 0.0f) {
            iceSkateDropTimer_ = 0.06f;
            if (bulletMgr_) {
                Vector2 dropPos = position_ - rushDir_ * 30.0f;
                float drift = -0.3f + static_cast<float>(rand() % 61) / 100.0f;
                float angle = std::atan2(rushDir_.y, rushDir_.x) + 3.14159265f + drift;
                Bullet b; b.pos = dropPos; b.vel = { std::cos(angle) * 260.f, std::sin(angle) * 260.f }; b.isEnemy = true;
                static BulletLinear lin; b.behavior = &lin;
                bulletMgr_->Spawn(b, GetBulletStyleForPhase());
            }
        }
        if (position_.x < -100 || position_.x > 1380 || position_.y > 800) {
            TryNextComboAction();
        }
    }
}

std::string Boss::GetBulletStyleForPhase() const {
    switch (currentGlobalPhase_) {
    case GlobalPhase::Ice:
        return "e_ice";
    case GlobalPhase::Fire:
        return "e_fire";
    case GlobalPhase::Normal:
    default:
        return "e_normal";
    }
}

void Boss::Draw() {

    bool useIceFire = (iceProgress_ > 0.0f || fireProgress_ > 0.0f);
    bool useHit = (hitFlashTimer_ > 0.0f);

    if (useHit) {
        HIKARI::POST::PostSystem::BeginLayer(hitChain_, 0, 0, 0, 0);
    }

    if (useIceFire) {
        HIKARI::POST::PostSystem::BeginLayer(bossChain_, 0, 0, 0, 0);
    }

    spine_.Draw();

    if (useIceFire) {
        HIKARI::POST::PostSystem::EndLayer();
    }

    if (useHit) {
        HIKARI::POST::PostSystem::EndLayer();
    }
}

void Boss::TakeDamage(float amount) {
    if (state_ == BossState::Intro || state_ == BossState::Transitioning || state_ == BossState::Dead) return;
    hp_ -= amount;
    hitFlashTimer_ = 0.8f;
    if (state_ != BossState::Intro && state_ != BossState::Dead) {
        anim_.PlayShakeEx(15.0f, 2.0f, 0.0f, 10.0f, 8.0f, 0.0f, 0.1f,HIKARI::ANIM::EASE::OutQuad);
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
