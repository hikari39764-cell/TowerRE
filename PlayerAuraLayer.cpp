#include "PlayerAuraLayer.h"
#include "HIKARI_PostSystem.h"
#include <algorithm> 

using namespace DirectX;

static float RandomFloat(float min, float max) {
    float r = (float)rand() / (float)RAND_MAX;
    return min + r * (max - min);
}

void PlayerAuraLayer::InitOnce()
{
    if (inited_) { return; }
    // 请确保 Shader 文件名一致
    if (!auraPS_.LoadPixelShader(L"./shaders/PS_PlayerAuraLocal.hlsl")) {
        return;
    }
    auraChain_.Clear();
    auraChain_.Add(&auraPS_);
    inited_ = true;
}

// 主动触发特效
void PlayerAuraLayer::TriggerCostFx(const Vector2& playerPosPx, float playerRadiusPx, int manaCountBeforeCost, AuraFxType type)
{
    // 计算之前那个魔力点的位置
    int deadIndex = manaCountBeforeCost - 1;
    if (deadIndex < 0) deadIndex = 0;

    float orbitR = playerRadiusPx * 2.8f;
    float orbitSpd = 2.3f;
    float baseA = globalTime_ * orbitSpd;
    float TAU = 6.28318530f;

    // 算出魔力点飞出前的理论位置
    float angle = baseA + TAU * (float)deadIndex / (float)(std::max(1, manaCountBeforeCost));
    angle += 0.1f * sin(globalTime_ * 1.9f + (float)deadIndex);

    float dx = cos(angle) * orbitR;
    float dy = sin(angle) * orbitR;

    AuraDeathFx fx;
    fx.startPos = XMFLOAT2(playerPosPx.x + dx, playerPosPx.y + dy);
    fx.timer = 0.0f;
    fx.duration = 0.8f; // 特效持续时间
    fx.seed = RandomFloat(0.0f, 99.0f); // 种子限制在 0~99
    fx.type = type;

    deathEffects_.push_back(fx);
}

void PlayerAuraLayer::UpdateParams(const Vector2& playerPosPx, float playerRadiusPx, int manaCount, float dt)
{
    if (!inited_) { return; }
    if (dt <= 0.0f) { dt = 1.0f / 60.0f; }

    globalTime_ += dt;

    // 检测魔力增加 (变多时闪一下)
    if (manaCount > lastManaCount_) {
        addFx_ = 1.0f;
    }
    // 注意：这里删除了 manaCount < lastManaCount_ 的自动检测
    // 因为现在由 Player 类调用 TriggerCostFx 来指定颜色

    lastManaCount_ = manaCount;

    addFx_ -= dt * 1.25f;
    addFx_ = std::max(0.0f, std::min(addFx_, 1.0f));

    // 更新所有特效时间
    for (auto& fx : deathEffects_) {
        fx.timer += dt;
    }

    // 移除播放完的特效
    deathEffects_.erase(std::remove_if(deathEffects_.begin(), deathEffects_.end(),
        [](const AuraDeathFx& fx) { return fx.timer >= fx.duration; }),
        deathEffects_.end());

    // --- 填充 Shader 参数 ---
    auraPS_.SetUser(0, XMFLOAT4(playerPosPx.x, playerPosPx.y, playerRadiusPx * 2.2f, 10.0f));
    auraPS_.SetUser(1, XMFLOAT4((float)manaCount, playerRadiusPx * 2.8f, 7.0f, 2.3f));
    auraPS_.SetUser(2, XMFLOAT4(0.95f, 0.12f, 1.00f, 1.35f));
    auraPS_.SetUser(3, XMFLOAT4(enabled_ ? 1.0f : 0.0f, addFx_, globalTime_, 0.0f));

    // 填充特效数据到 user[4] ~ user[9]
    for (int i = 0; i < 6; ++i) {
        if (i < deathEffects_.size()) {
            const auto& fx = deathEffects_[i];
            float progress = fx.timer / fx.duration;

            // [核心技巧] 将 类型(int) 和 种子(float) 压缩进一个 float w
            // 比如 type=4, seed=50.5 -> val = 4.0 + 0.0505
            // 种子除以 1000 以确保它只是小数部分
            float packedTypeSeed = (float)fx.type + (fx.seed * 0.001f);

            auraPS_.SetUser(4 + i, XMFLOAT4(fx.startPos.x, fx.startPos.y, progress, packedTypeSeed));
        } else {
            // 无效槽位，z = -1
            auraPS_.SetUser(4 + i, XMFLOAT4(0, 0, -1.0f, 0));
        }
    }
}

void PlayerAuraLayer::DrawLayer()
{
    if (!inited_ || !enabled_) { return; }
    HIKARI::POST::PostSystem::BeginLayer(auraChain_, 0, 0, 0, 0);
    HIKARI::POST::PostSystem::EndLayer();
}