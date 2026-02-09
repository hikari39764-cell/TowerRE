#pragma once
#include "HIKARI/HIKARI.h"
#include <functional>
#include <vector>
#include <deque>
#include <string>
#include "HIKARI.h"

class BulletManager;
class Player;


enum class DifficultyLevel {
    Normal,
    Challenge
};


enum class GlobalPhase {
    Normal, // 100% ~ 70%
    Ice,    // 70% ~ 40%
    Fire,   // 40% ~ 10%
    Final   // 10% ~ 0%
};


enum class ActionPattern {
    Idle,       // 待机/回中
    Rush,       // 技能：冲撞
    RingShot,   // 技能：环形弹幕
    FanShot,    // 技能：扇形弹
    CrossBurst, // 技能：十字/交叉弹
    IcicleRain, // 技能：冰锥雨
    IceMissile, // 技能：冰导弹齐射
    FireSpiral, // 技能：火焰螺旋散弹
    FlameRushBurst, // 技能：冲撞后爆发
    IceSkateRush,   // 技能：冲撞冰尾迹
    Changing,   // 位置插值 (内部使用)
    Wait        // 等待
};


enum class BossState {
    Intro,
    Normal,         // 平A状态 (受 GlobalPhase 影响)
    Switching,      // 变身动画中
    Execute,        // 技能执行中 (处于变身后的形态)
    Returning,      // 解除变身动画中
    Transitioning,  // 大阶段切换演出 (转场无敌)
    Dead
};


struct SkillNode {
    ActionPattern action;
    float duration; // 持续时间 或 等待时间
    // 可以在这里加 int loopCount 实现连击
};


struct SkillDef {
    std::string name;
    int weight;
    std::vector<GlobalPhase> allowedGlobalPhases;
    std::vector<SkillNode> sequence;
};


struct ModeDef {
    std::string name;

    // 关键：该变身形态只在这些大阶段允许出现
    std::vector<GlobalPhase> allowedGlobalPhases;
    int weight; // 抽选权重

    // 动画资源
    std::string switchAnim;     // 变身动画
    std::string modeIdleAnim;   // 变身后的待机动画
    std::string returnAnim;     // 解除变身动画

    // 该形态下的技能池
    std::vector<SkillDef> skillPool;
};

class Boss {
public:
    Boss();
    ~Boss() = default;

    // 初始化
    void Init(Player* target, BulletManager* bulletMgr);
    // 设置难度
    void SetDifficulty(DifficultyLevel level);
    void InitPostEffects();
    void Update(float dt);
    void Draw();
    void DrawWithEffects();
    // 信息获取
    Vector2 GetPos() const { return position_; }
    float GetHp() const { return hp_; }
    float GetMaxHp() const { return maxHp_; }
    GlobalPhase GetGlobalPhase() const { return currentGlobalPhase_; }
    bool IsDead() const;

    // 伤害与碰撞
    void TakeDamage(float amount);
    RectF GetAABB() const;

    // 回调：大阶段切换时触发 (用于通知背景层)
    using PhaseChangeCallback = std::function<void(GlobalPhase)>;
    void SetPhaseChangeCallback(PhaseChangeCallback cb) { onPhaseChange_ = cb; }

private:
    // --- 核心逻辑 ---
    void InitModes();                   // 注册形态和技能数据 (受难度影响)
    void UpdateState(float dt);         // 状态机更新
    void CheckGlobalPhase();            // 检查 HP 触发转场

    void SelectModeAndSkill();          // ★ 核心：筛选并选择形态+技能
    void TryNextComboAction();          // 执行技能队列

    // --- 行为实现 ---
    void UpdateNormalBehavior(float dt);// 平A逻辑 (受 GlobalPhase 影响)
    void FireNormalBarrage();           // 发射平A弹幕
    std::string GetBulletStyleForPhase() const;

    // 具体技能动作
    void PatternRush(float dt);
    void PatternRingShot(float dt);
    void PatternFanShot(float dt);
    void PatternCrossBurst(float dt);
    void PatternIcicleRain(float dt);
    void PatternIceMissile(float dt);
    void PatternFireSpiral(float dt);
    void PatternFlameRushBurst(float dt);
    void PatternIceSkateRush(float dt);
    void RequestPattern(ActionPattern next, bool ease = true);

private:
    // 基础对象
    HIKARI::SpineActor spine_;
    HIKARI::ANIM::TransformAnimator anim_;

    HIKARI::POST::PostEffect* fxHit_ = nullptr;
    float hitFlashTimer_ = 0.0f;
    HIKARI::POST::PostChain  hitChain_;

    Vector2 position_{ 640.0f, -200.0f };
    Vector2 size_{ 150.0f, 150.0f };
    Vector2 velocity_{ 0.0f, 0.0f };
    Player* target_ = nullptr;
    BulletManager* bulletMgr_ = nullptr;

    DifficultyLevel difficulty_ = DifficultyLevel::Normal;
    float hp_ = 30000.0f;
    float maxHp_ = 30000.0f;
    float hurtTimer_ = 0.0f;

    // 速度与时间参数
    float speedMultiplier_ = 1.0f;  // 全局速度倍率 (影响动作快慢)
    float introDuration_ = 3.5f;    // 入场等待时间
    bool  introAnimPlayed_ = false; // 入场动画标记
    float moveSpeed_ = 80.0f;       // 基础移动速度 (影响平A晃动速度)
    float rushSpeed_ = 1000.0f;     // 冲撞速度

    HIKARI::POST::PostEffect* fxIce_ = nullptr;
    HIKARI::POST::PostChain  bossChain_;

    HIKARI::POST::PostEffect* fxFire_ = nullptr;
    float fireProgress_ = 0.0f;

    float iceProgress_ = 0.0f; // 0.0 = 正常, 1.0 = 全冰
    bool  isIceMode_ = false;

    // 阶段控制
    BossState state_ = BossState::Intro;
    GlobalPhase currentGlobalPhase_ = GlobalPhase::Normal;
    float stateTimer_ = 0.0f;
    bool phaseTriggered_[4] = { false };
    PhaseChangeCallback onPhaseChange_ = nullptr;

    // 形态控制
    std::vector<ModeDef> modes_;        // 形态库
    int activeModeIndex_ = -1;          // 当前变身形态

    // 技能执行
    std::deque<SkillNode> executionQueue_;
    bool waitingForCombo_ = false;
    float comboTimer_ = 0.0f;
    std::string lastSkillName_;

    // 动作控制
    ActionPattern pattern_ = ActionPattern::Idle;
    float patternTime_ = 0.0f;

    // 移动/缓动
    Vector2 easeStartPos_, easeTargetPos_;
    float easeTime_ = 0.0f, easeDuration_ = 0.0f;
    ActionPattern easeNextPattern_;

    // 普通攻击变量
    float normalMoveTime_ = 0.0f;
    float shootTimer_ = 0.0f;
    float normalDuration_ = 5.0f; // 平A持续多久后进变身

    // 技能专用变量
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
};
