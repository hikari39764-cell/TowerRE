#include "ScrollBackground.h"
#include "GameConfig.h"
#include "HIKARI/HIKARI.h"

#include "HIKARI_PostSystem.h"
#include "HIKARI_PostEffect.h"
#include "HIKARI_PostChain.h"

using namespace HIKARI;

namespace {
    // 资源路径
    const char* kBgNormal = "./images/stage/normal.png";
    const char* kFgNormal = "./images/stage/normal_fg.png";

    const char* kBgIce = "./images/stage/ice.png";
    const char* kFgIce = "./images/stage/ice_fg.png";

    const char* kBgFire = "./images/stage/fire.png";
    const char* kFgFire = "./images/stage/fire_fg.png";

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
    // 每个阶段独立 key
    TEXTURE::Register("stage_bg_normal", kBgNormal, "stage");
    TEXTURE::Register("stage_fg_normal", kFgNormal, "stage");

    TEXTURE::Register("stage_bg_ice", kBgIce, "stage");
    TEXTURE::Register("stage_fg_ice", kFgIce, "stage");

    TEXTURE::Register("stage_bg_fire", kBgFire, "stage");
    TEXTURE::Register("stage_fg_fire", kFgFire, "stage");

    // final 暂时不注册
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
    if (s == Stage::Final) {
        SetStage(Stage::Final);
        return;
    }

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

    fxIceWipe_ = new POST::PostEffect();
    fxFireWipe_ = new POST::PostEffect();
    chainIce_ = new POST::PostChain();
    chainFire_ = new POST::PostChain();

    fxIceWipe_->LoadPixelShader(L"./shaders/PS_StageWipe_Ice.hlsl");
    fxFireWipe_->LoadPixelShader(L"./shaders/PS_StageWipe_Fire.hlsl");

    chainIce_->Clear();
    chainIce_->Add(fxIceWipe_);

    chainFire_->Clear();
    chainFire_->Add(fxFireWipe_);
}

void ScrollBackground::Update(float dt)
{

    if (stage_ == Stage::Final) {
        UpdateFinal(dt);
        return;
    }

    bgOffsetY_ += GAMECFG::kScrollSpeedBg * dt;
    fgOffsetY_ += GAMECFG::kScrollSpeedFg * dt;

    if (bgOffsetY_ >= (float)GAMECFG::kStageBgH) { bgOffsetY_ -= (float)GAMECFG::kStageBgH; }
    if (fgOffsetY_ >= (float)GAMECFG::kStageFgH) { fgOffsetY_ -= (float)GAMECFG::kStageFgH; }

    if (transitioning_) {
        transT_ += dt;
        if (transT_ >= transDur_) {

            transitioning_ = false;
            SetStage(toStage_);
        }
    }
}

void ScrollBackground::DrawStage_(Stage s)
{
    if (s == Stage::Final) {
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

    int y0 = -(int)bgOffsetY_;
    {
        Transform2D t{};
        t.pivotPx = { 0,0 };
        t.position = { 0.0f, (float)y0 };
        RENDERER::DrawSprite(bgKey, t, (float)GAMECFG::kGameW, (float)GAMECFG::kStageBgH,HIKARI::RENDERER::CameraMode::Ignore);

        t.position = { 0.0f, (float)(y0 + GAMECFG::kStageBgH) };
        RENDERER::DrawSprite(bgKey, t, (float)GAMECFG::kGameW, (float)GAMECFG::kStageBgH, HIKARI::RENDERER::CameraMode::Ignore);
    }

    {
        Transform2D t{};
        t.pivotPx = { 0,0 };
        t.position = { 0.0f, 0.0f };
        RENDERER::DrawSprite(fgKey, t, (float)GAMECFG::kGameW, (float)GAMECFG::kStageFgH, HIKARI::RENDERER::CameraMode::Ignore);
    }
}

void ScrollBackground::DrawTransitionOverlay_()
{
    if (!transitioning_) { return; }

    float p = transT_ / transDur_;
    if (p < 0.0f) { p = 0.0f; }
    if (p > 1.0f) { p = 1.0f; }

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
        60.0f,  // 边缘宽度
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

    if (transitioning_) {
        DrawTransitionOverlay_();
    }
}
