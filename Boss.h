#pragma once
#include "HIKARI/HIKARI.h"
#include <functional>
#include <vector>
#include <deque>
#include <string>
#include "HIKARI.h"
#include "HIKARI_Particle.h"

class BulletManager;
class Player;

enum class DifficultyLevel {
    Normal,
    Challenge
};

enum class GlobalPhase {
    Normal,
    Ice,
    Fire,
    Final
};

enum class ActionPattern {
    Idle,
    Rush,
    GroundSlam,
    ScatterShot,
    RingShot,
    FanShot,
    CrossBurst,
    IcicleRain,
    IceMissile,
    FireSpiral,
    FlameRushBurst,
    IceSkateRush,
    Apocalypse,
    Changing,
    Wait
};

enum class BossState {
    Intro,
    Normal,
    Switching,
    Execute,
    Returning,
    Transitioning,
    Dead
};

struct SkillNode {
    ActionPattern action;
    float duration;
    int param1 = 0;
};

struct SkillDef {
    std::string name;
    int weight;
    std::vector<GlobalPhase> allowedGlobalPhases;
    std::vector<SkillNode> sequence;
};

struct ModeDef {
    std::string name;
    std::vector<GlobalPhase> allowedGlobalPhases;
    int weight;
    std::string switchAnim;
    std::string modeIdleAnim;
    std::string returnAnim;
    std::vector<SkillDef> skillPool;
};

class Boss {
public:
    Boss();
    ~Boss() = default;

    void Init(Player* target, BulletManager* bulletMgr, HIKARI::PARTICLE::ParticleSystem* particleSys);

    void SetDifficulty(DifficultyLevel level);
    void InitPostEffects();
    void Update(float dt);
    void Draw();

    Vector2 GetPos() const { return position_; }
    float GetHp() const { return hp_; }
    float GetMaxHp() const { return maxHp_; }
    GlobalPhase GetGlobalPhase() const { return currentGlobalPhase_; }
    bool IsDead() const;
    void TakeDamage(float amount);
    RectF GetAABB() const;
    using PhaseChangeCallback = std::function<void(GlobalPhase)>;
    void SetPhaseChangeCallback(PhaseChangeCallback cb) { onPhaseChange_ = cb; }

private:
    void InitModes();
    void InitParticleEffects();
    void UpdateState(float dt);
    void UpdateVisuals(float dt);
    void CheckGlobalPhase();
    void SelectModeAndSkill();
    void TryNextComboAction();

    void UpdateNormalBehavior(float dt);
    void FireNormalBarrage();
    std::string GetBulletStyleForPhase() const;

    void PatternRush(float dt);
    void PatternGroundSlam(float dt);
    void PatternScatterShot(float dt);
    void PatternRingShot(float dt);
    void PatternFanShot(float dt);
    void PatternCrossBurst(float dt);
    void PatternIcicleRain(float dt);
    void PatternIceMissile(float dt);
    void PatternFireSpiral(float dt);
    void PatternFlameRushBurst(float dt);
    void PatternIceSkateRush(float dt);
    void PatternApocalypse(float dt);

    void RequestPattern(ActionPattern next, bool ease = true);

private:
    HIKARI::SpineActor spine_;
    HIKARI::ANIM::TransformAnimator anim_;

    HIKARI::POST::PostEffect* fxHit_ = nullptr;
    HIKARI::POST::PostChain  hitChain_;
    HIKARI::POST::PostEffect* fxIce_ = nullptr;
    HIKARI::POST::PostEffect* fxFire_ = nullptr;
    HIKARI::POST::PostChain  bossChain_;

    Vector2 position_{ 640.0f, -200.0f };
    Vector2 size_{ 150.0f, 150.0f };
    Vector2 velocity_{ 0.0f, 0.0f };

    Player* target_ = nullptr;
    BulletManager* bulletMgr_ = nullptr;
    HIKARI::PARTICLE::ParticleSystem* particleSys_ = nullptr;

    DifficultyLevel difficulty_ = DifficultyLevel::Normal;
    float hp_ = 30000.0f;
    float maxHp_ = 30000.0f;
    float hurtTimer_ = 0.0f;
    float hitFlashTimer_ = 0.0f;

    float speedMultiplier_ = 1.0f;
    float introDuration_ = 3.5f;
    bool  introAnimPlayed_ = false;
    float moveSpeed_ = 80.0f;
    float rushSpeed_ = 1000.0f;

    float fireProgress_ = 0.0f;
    float iceProgress_ = 0.0f;

    BossState state_ = BossState::Intro;
    GlobalPhase currentGlobalPhase_ = GlobalPhase::Normal;
    float stateTimer_ = 0.0f;
    bool phaseTriggered_[4] = { false };
    PhaseChangeCallback onPhaseChange_ = nullptr;

    std::vector<ModeDef> modes_;
    int activeModeIndex_ = -1;
    std::deque<SkillNode> executionQueue_;
    bool waitingForCombo_ = false;
    float comboTimer_ = 0.0f;
    std::string lastSkillName_;

    ActionPattern pattern_ = ActionPattern::Idle;
    float patternTime_ = 0.0f;

    Vector2 easeStartPos_, easeTargetPos_;
    float easeTime_ = 0.0f, easeDuration_ = 0.0f;
    ActionPattern easeNextPattern_;

    float normalMoveTime_ = 0.0f;
    float shootTimer_ = 0.0f;
    float normalDuration_ = 5.0f;

    int rushStep_ = 0;
    Vector2 rushDir_{};
    int ringStep_ = 0;
    int fanShotStep_ = 0;
    int crossBurstStep_ = 0;
    float icicleSpawnTimer_ = 0.0f;
    int iceMissileStep_ = 0;
    int iceMissileTotal_ = 0;
    float fireSpiralTimer_ = 0.0f;
    float fireSpiralAngle_ = 0.0f;
    int flameRushStep_ = 0;
    int flameBurstRemaining_ = 0;
    float iceSkateDropTimer_ = 0.0f;
    int iceSkateStep_ = 0;

    int slamStep_ = 0;
    int scatterStep_ = 0;
    int apocalypseStep_ = 0;
    float apocalypseAngle_ = 0.0f;

    Vector2 renderScale_{ 1.0f, 1.0f };
    float renderRotation_ = 0.0f;
    float trailTimer_ = 0.0f;
};