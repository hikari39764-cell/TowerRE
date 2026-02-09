#include "MagicAnimeGlobalFX.h"
#include "HIKARI_PostSystem.h"
using namespace DirectX;

void MagicAnimeGlobalFX::InitOnce()
{
    if (inited_) { return; }

    if (!fx_.LoadPixelShader(L"./shaders/PS_MagicAnimeGlobal.hlsl")) {
        return;
    }

    // 加到全局主链
    HIKARI::POST::PostSystem::AddEffect(&fx_);
    inited_ = true;
}

void MagicAnimeGlobalFX::UpdateParams()
{
    if (!inited_) { return; }

    // user0: jitterPx, waveAmpPx, waveFreq, gradeInt
    fx_.SetUser(0, XMFLOAT4(
        enabled_ ? 1.0f : 0.0f,   // jitterPx
        1.0f,                     // waveAmpPx
        1.8f,                     // waveFreq
        0.75f                     // gradeInt
    ));

    // user1: hueAmt, grainInt, glowInt, vignette
    fx_.SetUser(1, XMFLOAT4(
        0.05f,   // hueAmt
        0.02f,   // grainInt
        0.45f,   // glowInt
        0.10f    // vignette
    ));
    // user2: edgeStrength, edgeThreshold, edgeSoftness, edgeDarken
    fx_.SetUser(2, XMFLOAT4(
        0.85f,  // edgeStrength：越大线越明显
        0.12f,  // edgeThreshold：越大越不容易出线
        0.06f,  // edgeSoftness：越小越硬朗
        0.65f   // edgeDarken：线条压暗程度
    ));

    // user3: edgeTint(rgb), edgeTintMix
    fx_.SetUser(3, XMFLOAT4(
        0.18f, 0.05f, 0.25f, // 偏紫的“墨色”
        0.55f                // tint 混合强度
    ));
}
