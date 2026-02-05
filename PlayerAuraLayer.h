#pragma once
#include <DirectXMath.h>
#include <vector>
#include "HIKARI.h"


enum class AuraFxType : int {
    None = 0,
    UpgradeShot = 1,  // 红色：强化攻击
    UpgradeSpeed = 2, // 蓝色：强化速度
    Dash = 3,         // 紫色：冲刺
    Heal = 4          // 绿色：回复
};

struct AuraDeathFx {
    DirectX::XMFLOAT2 startPos;
    float timer;
    float duration;
    float seed;
    AuraFxType type; // 记录类型
};

class PlayerAuraLayer {
public:
    void InitOnce();
    void SetEnabled(bool enabled) { enabled_ = enabled; }

    // 2. 普通更新：只负责位置和魔力点增加的检测
    void UpdateParams(const Vector2& playerPosPx, float playerRadiusPx, int manaCount, float dt = (1.0f / 60.0f));

    // 3. 新增：当 Player 消耗魔力时手动调用此函数
    void TriggerCostFx(const Vector2& playerPosPx, float playerRadiusPx, int manaCountBeforeCost, AuraFxType type);

    void DrawLayer();

private:
    bool inited_ = false;
    bool enabled_ = true;

    int   lastManaCount_ = 0;
    float addFx_ = 0.0f;     // 魔力增加时的闪光
    float globalTime_ = 0.0f;

    std::vector<AuraDeathFx> deathEffects_;

    HIKARI::POST::PostEffect auraPS_;
    HIKARI::POST::PostChain  auraChain_;
};