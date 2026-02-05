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
    meleeMode.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Ice, GlobalPhase::Final };

    // Skill: Rush
    SkillDef rushSkill;
    rushSkill.name = "Rush Attack";
    rushSkill.weight = 100;

    // ★ 难度差异：Challenge 模式下 Rush 连撞3次，Normal 撞2次
    if (difficulty_ == DifficultyLevel::Challenge) {
        rushSkill.sequence = {
            { ActionPattern::Rush, 0.0f },
            { ActionPattern::Wait, 0.3f }, // 等待更短
            { ActionPattern::Rush, 0.0f },
            { ActionPattern::Wait, 0.3f },
            { ActionPattern::Rush, 0.0f }
        };
    } else {
        rushSkill.sequence = {
            { ActionPattern::Rush, 0.0f },
            { ActionPattern::Wait, 0.5f },
            { ActionPattern::Rush, 0.0f }
        };
    }
    meleeMode.skillPool.push_back(rushSkill);
    modes_.push_back(meleeMode);

    // --- Magic Form ---
    ModeDef magicMode;
    magicMode.name = "Magic Form";
    magicMode.switchAnim = "switchPhase2";
    magicMode.modeIdleAnim = "idle2";
    magicMode.returnAnim = "returnPhase2";
    magicMode.weight = 50;
    magicMode.allowedGlobalPhases = { GlobalPhase::Normal, GlobalPhase::Fire, GlobalPhase::Final };

    // Skill: Ring Shot
    SkillDef ringSkill;
    ringSkill.name = "Ring Barrage";
    ringSkill.weight = 100;
    ringSkill.sequence = { { ActionPattern::RingShot, 0.0f } };
    magicMode.skillPool.push_back(ringSkill);
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
                }
            } else if (pattern_ == ActionPattern::Rush) PatternRush(dt);
            else if (pattern_ == ActionPattern::RingShot) PatternRingShot(dt);
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
    if (!phaseTriggered_[1] && p <= 0.70f) { next = GlobalPhase::Ice; phaseTriggered_[1] = true; } else if (!phaseTriggered_[2] && p <= 0.40f) { next = GlobalPhase::Fire; phaseTriggered_[2] = true; } else if (!phaseTriggered_[3] && p <= 0.10f) { next = GlobalPhase::Final; phaseTriggered_[3] = true; }

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

    // ★ 使用 moveSpeed_ 变量
    float speed = (currentGlobalPhase_ == GlobalPhase::Final) ? 2.0f : 1.0f;
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
        if (currentGlobalPhase_ == GlobalPhase::Final) baseCd = 0.4f;

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
    if (currentGlobalPhase_ == GlobalPhase::Normal) {
        Vector2 dir = Normalize(target_->GetPos() - position_);
        Bullet b; b.pos = position_; b.vel = dir * 400.0f; b.isEnemy = true;
        static BulletLinear lin; b.behavior = &lin;
        bulletMgr_->Spawn(b, "normal");
    } else if (currentGlobalPhase_ == GlobalPhase::Ice) {
        float angle = std::atan2(target_->GetPos().y - position_.y, target_->GetPos().x - position_.x);
        for (int i = -1; i <= 1; ++i) {
            float a = angle + i * 0.3f;
            Bullet b; b.pos = position_; b.vel = { std::cos(a) * 350.f, std::sin(a) * 350.f }; b.isEnemy = true;
            static BulletLinear lin; b.behavior = &lin;
            bulletMgr_->Spawn(b, "normal");
        }
    } else {
        static float spinA = 0.0f; spinA += 0.5f;
        Bullet b; b.pos = position_; b.vel = { std::cos(spinA) * 500.f, std::sin(spinA) * 500.f }; b.isEnemy = true;
        static BulletLinear lin; b.behavior = &lin;
        bulletMgr_->Spawn(b, "normal");
    }
}

void Boss::SelectModeAndSkill() {
    std::vector<int> validModeIndices;
    int totalWeight = 0;
    for (int i = 0; i < modes_.size(); ++i) {
        bool allowed = false;
        for (auto p : modes_[i].allowedGlobalPhases) {
            if (p == currentGlobalPhase_) { allowed = true; break; }
        }
        if (allowed) { validModeIndices.push_back(i); totalWeight += modes_[i].weight; }
    }
    if (validModeIndices.empty()) { state_ = BossState::Normal; stateTimer_ = 0.0f; return; }
    int r = rand() % totalWeight;
    int currentSum = 0;
    int selectedIdx = validModeIndices[0];
    for (int idx : validModeIndices) {
        currentSum += modes_[idx].weight;
        if (r < currentSum) { selectedIdx = idx; break; }
    }
    activeModeIndex_ = selectedIdx;
    const auto& mode = modes_[activeModeIndex_];
    if (mode.skillPool.empty()) { state_ = BossState::Normal; return; }
    int skillIdx = rand() % mode.skillPool.size();
    const auto& skill = mode.skillPool[skillIdx];
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
        else easeTargetPos_ = { 640.0f, 230.0f }; // Idle return position
    } else {
        pattern_ = next;
        if (next == ActionPattern::Rush) rushStep_ = 0;
        if (next == ActionPattern::RingShot) ringStep_ = 0;
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
                bulletMgr_->Spawn(b, "normal");
            }
        }
        int waves = (difficulty_ == DifficultyLevel::Challenge) ? 8 : 5;
        if (ringStep_ >= waves) TryNextComboAction();
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