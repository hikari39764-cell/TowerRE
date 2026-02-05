#include "FadeTransition.h"

#include <Novice.h>
#include <cmath>

#include "HIKARI/HIKARI_Utility.h"

static float Clamp01(float v)
{
    return (v < 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);
}

FadeTransition::FadeTransition(float fadeOutSec, float fadeInSec)
    : fadeOut_(fadeOutSec), fadeIn_(fadeInSec)
{
}

void FadeTransition::Start()
{
    t_ = 0.0f;
}

void FadeTransition::Update(float dt)
{
    t_ += dt;
}

bool FadeTransition::ShouldSwapSceneNow() const
{
    if (fadeOut_ <= 0.0f) {
        return true;
    }
    return t_ >= fadeOut_;
}

bool FadeTransition::IsFinished() const
{
    const float total = fadeOut_ + fadeIn_;
    if (total <= 0.0f) {
        return true;
    }
    return t_ >= total;
}

void FadeTransition::DrawOverlay()
{
    float a = 1.0f;

    if (fadeOut_ > 0.0f && t_ < fadeOut_) {
        a = Clamp01(t_ / fadeOut_);
    } else if (fadeIn_ > 0.0f) {
        const float u = (t_ - fadeOut_) / fadeIn_;
        a = 1.0f - Clamp01(u);
    } else {
        a = 1.0f;
    }

    const int alpha = static_cast<int>(std::round(a * 255.0f));
    const unsigned int col = PackRGBA(0, 0, 0, alpha);
    Novice::DrawBox(0, 0, kScreenW, kScreenH, 0.0f, col, kFillModeSolid);
}
