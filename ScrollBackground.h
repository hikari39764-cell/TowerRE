#pragma once
#include <string>

namespace HIKARI { namespace POST { class PostEffect; class PostChain; } }

class ScrollBackground {
public:
    enum class Stage {
        Normal,
        Ice,
        Fire,
        Final,
    };

public:
    // 只加载资源
    void LoadOnce();

    // 仅重置滚动值
    void ResetScroll();

    // 直接切阶段（无转场）
    void SetStage(Stage s);

    // 开始转场（从当前阶段 -> s）
    // Normal->Ice : 冰冻刷下
    // Ice->Fire   : 火焰刷下
    // Fire->Final : 暂时不做特效
    void BeginTransition(Stage s, float durationSec = 1.2f);

    bool IsTransitioning() const { return transitioning_; }
    Stage GetStage() const { return stage_; }

    void Update(float dt);
    void Draw();


    void UpdateFinal(float /*dt*/) {}
    void DrawFinal() {}

private:
    // ===== scroll =====
    float bgOffsetY_ = 0.0f;
    float fgOffsetY_ = 0.0f;

    std::string bgName_ = "stage_bg_normal";
    std::string fgName_ = "stage_fg_normal";

    Stage stage_ = Stage::Normal;

    // ===== transition =====
    bool  transitioning_ = false;
    Stage fromStage_ = Stage::Normal;
    Stage toStage_ = Stage::Normal;

    float transT_ = 0.0f;
    float transDur_ = 1.2f;
    bool postInited_ = false;
    HIKARI::POST::PostEffect* fxIceWipe_ = nullptr;
    HIKARI::POST::PostEffect* fxFireWipe_ = nullptr;
    HIKARI::POST::PostChain* chainIce_ = nullptr;
    HIKARI::POST::PostChain* chainFire_ = nullptr;

private:
    void EnsurePostInited_();
    void RegisterStageTextures_();

    void DrawStage_(Stage s);

    void DrawTransitionOverlay_();
};
