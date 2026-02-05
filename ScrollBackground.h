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
    // 只加载资源（不会重置滚动）
    void LoadOnce();

    // 仅重置滚动值（想“从头开始”一局时才调用）
    void ResetScroll();

    // 直接切阶段（无转场）
    void SetStage(Stage s);

    // 开始转场（从当前阶段 -> s）
    // Normal->Ice : 冰冻刷下
    // Ice->Fire   : 火焰刷下
    // Fire->Final : 暂时不做特效（这里会直接切；你说 final 转场先不做）
    void BeginTransition(Stage s, float durationSec = 1.2f);

    bool IsTransitioning() const { return transitioning_; }
    Stage GetStage() const { return stage_; }

    void Update(float dt);
    void Draw();

    // Final 阶段：你后面要自己做背景动画，这里先留空钩子
    void UpdateFinal(float /*dt*/) {}
    void DrawFinal() {}

private:
    // ===== scroll =====
    float bgOffsetY_ = 0.0f;
    float fgOffsetY_ = 0.0f;

    // ===== stage texture keys =====
    // 注意：这里不再只用 stage_bg / stage_fg 两个 key，
    // 而是为每个阶段注册独立 key，切换只换 key，offset 不动。
    std::string bgName_ = "stage_bg_normal";
    std::string fgName_ = "stage_fg_normal";

    Stage stage_ = Stage::Normal;

    // ===== transition =====
    bool  transitioning_ = false;
    Stage fromStage_ = Stage::Normal;
    Stage toStage_ = Stage::Normal;

    float transT_ = 0.0f;
    float transDur_ = 1.2f;

    // 只在转场期间用：把“新阶段背景”画到一张局部 layer 上，并用 shader 做蒙版刷下效果再叠回去
    bool postInited_ = false;
    HIKARI::POST::PostEffect* fxIceWipe_ = nullptr;
    HIKARI::POST::PostEffect* fxFireWipe_ = nullptr;
    HIKARI::POST::PostChain* chainIce_ = nullptr;
    HIKARI::POST::PostChain* chainFire_ = nullptr;

private:
    void EnsurePostInited_();
    void RegisterStageTextures_();

    // 画一个阶段的 BG/FG（final 特殊处理）
    void DrawStage_(Stage s);

    // 用对应转场 shader 把“toStage”叠上去
    void DrawTransitionOverlay_();
};
