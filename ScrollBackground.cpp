#include "ScrollBackground.h"
#include "GameConfig.h"
#include "HIKARI/HIKARI.h"

#include "HIKARI_PostSystem.h"
#include "HIKARI_PostEffect.h"
#include "HIKARI_PostChain.h"

using namespace HIKARI;

namespace {
    // 资源路径（按你给的）
    const char* kBgNormal = "./images/stage/normal.png";
    const char* kFgNormal = "./images/stage/normal_fg.png";

    const char* kBgIce = "./images/stage/ice.png";
    const char* kFgIce = "./images/stage/ice_fg.png";

    const char* kBgFire = "./images/stage/fire.png";
    const char* kFgFire = "./images/stage/fire_fg.png";

    // final 你没给素材名，这里先留空。你后面自己在 DrawFinal() 里做动画。
}

void ScrollBackground::LoadOnce()
{
    static bool loaded = false;
    if (loaded) { return; }

    RegisterStageTextures_();
    TEXTURE::LoadGroup("stage"); // 统一 group

    loaded = true;
}

void ScrollBackground::RegisterStageTextures_()
{
    // 每个阶段独立 key（这样切换只换 key，不需要重注册）
    TEXTURE::Register("stage_bg_normal", kBgNormal, "stage");
    TEXTURE::Register("stage_fg_normal", kFgNormal, "stage");

    TEXTURE::Register("stage_bg_ice", kBgIce, "stage");
    TEXTURE::Register("stage_fg_ice", kFgIce, "stage");

    TEXTURE::Register("stage_bg_fire", kBgFire, "stage");
    TEXTURE::Register("stage_fg_fire", kFgFire, "stage");

    // final 暂时不注册（你要自己做动画）
}

void ScrollBackground::ResetScroll()
{
    bgOffsetY_ = 0.0f;
    fgOffsetY_ = 0.0f;
}

void ScrollBackground::SetStage(Stage s)
{
    transitioning_ = false;
    stage_ = s;

    if (stage_ == Stage::Normal) {
        bgName_ = "stage_bg_normal";
        fgName_ = "stage_fg_normal";
    } else if (stage_ == Stage::Ice) {
        bgName_ = "stage_bg_ice";
        fgName_ = "stage_fg_ice";
    } else if (stage_ == Stage::Fire) {
        bgName_ = "stage_bg_fire";
        fgName_ = "stage_fg_fire";
    } else { // Final
        // final 不滚动/不画前景，由 DrawFinal() 决定怎么画
        bgName_.clear();
        fgName_.clear();
    }
}

void ScrollBackground::BeginTransition(Stage s, float durationSec)
{
    // final 的转场你说暂时不做：这里直接切即可（你后面要做再加）
    if (s == Stage::Final) {
        SetStage(Stage::Final);
        return;
    }

    // 只实现：Normal->Ice, Ice->Fire
    // 其他组合先直接切（避免你测试时卡住）
    if (!((stage_ == Stage::Normal && s == Stage::Ice) ||
        (stage_ == Stage::Ice && s == Stage::Fire))) {
        SetStage(s);
        return;
    }

    EnsurePostInited_();

    transitioning_ = true;
    fromStage_ = stage_;
    toStage_ = s;

    transT_ = 0.0f;
    transDur_ = (durationSec <= 0.01f) ? 0.01f : durationSec;
}

void ScrollBackground::EnsurePostInited_()
{
    if (postInited_) { return; }
    postInited_ = true;

    // 这里用 new 是为了避免你没包含完整定义导致 stack 构造问题；你也可以改成成员对象
    fxIceWipe_ = new POST::PostEffect();
    fxFireWipe_ = new POST::PostEffect();
    chainIce_ = new POST::PostChain();
    chainFire_ = new POST::PostChain();

    // shader 路径
    // 你把下面两个 hlsl 放到 ./shaders/ 下（我下面给完整文件）
    fxIceWipe_->LoadPixelShader(L"./shaders/PS_StageWipe_Ice.hlsl");
    fxFireWipe_->LoadPixelShader(L"./shaders/PS_StageWipe_Fire.hlsl");

    chainIce_->Clear();
    chainIce_->Add(fxIceWipe_);

    chainFire_->Clear();
    chainFire_->Add(fxFireWipe_);
}

void ScrollBackground::Update(float dt)
{
    // Final：不滚动
    if (stage_ == Stage::Final) {
        UpdateFinal(dt);
        return;
    }

    // 继续滚动（滚动值在阶段切换时不重置 -> “继承滚动值”）
    bgOffsetY_ += GAMECFG::kScrollSpeedBg * dt;
    fgOffsetY_ += GAMECFG::kScrollSpeedFg * dt;

    if (bgOffsetY_ >= (float)GAMECFG::kStageBgH) { bgOffsetY_ -= (float)GAMECFG::kStageBgH; }
    if (fgOffsetY_ >= (float)GAMECFG::kStageFgH) { fgOffsetY_ -= (float)GAMECFG::kStageFgH; }

    if (transitioning_) {
        transT_ += dt;
        if (transT_ >= transDur_) {
            // 转场结束：正式切到新阶段（offset 不变）
            transitioning_ = false;
            SetStage(toStage_);
        }
    }
}

void ScrollBackground::DrawStage_(Stage s)
{
    if (s == Stage::Final) {
        // final：你要求不画前景，而且要留空函数给你填动画
        DrawFinal();
        return;
    }

    std::string bgKey;
    std::string fgKey;

    if (s == Stage::Normal) {
        bgKey = "stage_bg_normal";
        fgKey = "stage_fg_normal";
    } else if (s == Stage::Ice) {
        bgKey = "stage_bg_ice";
        fgKey = "stage_fg_ice";
    } else { // Fire
        bgKey = "stage_bg_fire";
        fgKey = "stage_fg_fire";
    }

    // 背景：2112 循环滚动（用你原逻辑）:contentReference[oaicite:2]{index=2}
    int y0 = -(int)bgOffsetY_;
    {
        Transform2D t{};
        t.pivotPx = { 0,0 };
        t.position = { 0.0f, (float)y0 };
        RENDERER::DrawSprite(bgKey, t, (float)GAMECFG::kGameW, (float)GAMECFG::kStageBgH);

        t.position = { 0.0f, (float)(y0 + GAMECFG::kStageBgH) };
        RENDERER::DrawSprite(bgKey, t, (float)GAMECFG::kGameW, (float)GAMECFG::kStageBgH);
    }

    // 前景：你目前是固定画一次（y=0）:contentReference[oaicite:3]{index=3}
    {
        Transform2D t{};
        t.pivotPx = { 0,0 };
        t.position = { 0.0f, 0.0f };
        RENDERER::DrawSprite(fgKey, t, (float)GAMECFG::kGameW, (float)GAMECFG::kStageFgH);
    }
}

void ScrollBackground::DrawTransitionOverlay_()
{
    if (!transitioning_) { return; }

    float p = transT_ / transDur_;
    if (p < 0.0f) { p = 0.0f; }
    if (p > 1.0f) { p = 1.0f; }

    // 选择 shader（只做 Normal->Ice, Ice->Fire）
    POST::PostChain* chain = nullptr;
    POST::PostEffect* fx = nullptr;

    if (fromStage_ == Stage::Normal && toStage_ == Stage::Ice) {
        chain = chainIce_;
        fx = fxIceWipe_;
    } else if (fromStage_ == Stage::Ice && toStage_ == Stage::Fire) {
        chain = chainFire_;
        fx = fxFireWipe_;
    } else {
        return;
    }


    fx->SetUser(0, DirectX::XMFLOAT4(
        p,
        60.0f,  // 边缘宽度（像素）
        0.9f,   // 边缘噪声起伏
        1.0f    // 特效强度
    ));

    fx->SetUser(1, DirectX::XMFLOAT4(0, 0, 0, 0));


    POST::PostSystem::BeginLayer(*chain, 0, 0, 0, 0);

    DrawStage_(toStage_);

    POST::PostSystem::EndLayer();
}

void ScrollBackground::Draw()
{
    // 先画旧/当前阶段
    DrawStage_(stage_);

    // 如果在转场：叠加新阶段 layer（只影响 1280*1000 的游戏区；UI你在右侧自己画，不会被这里覆盖）
    if (transitioning_) {
        DrawTransitionOverlay_();
    }
}
