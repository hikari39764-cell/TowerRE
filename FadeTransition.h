#pragma once

#include "ITransition.h"

class FadeTransition : public ITransition
{
public:
    FadeTransition(float fadeOutSec = 0.25f, float fadeInSec = 0.25f);

    void Start() override;
    void Update(float dt) override;
    void DrawOverlay() override;

    bool IsFinished() const override;
    bool ShouldSwapSceneNow() const override;

private:
    float fadeOut_ = 0.25f;
    float fadeIn_ = 0.25f;
    float t_ = 0.0f;
};
