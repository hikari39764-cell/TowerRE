#pragma once

class ITransition
{
public:
    virtual ~ITransition() = default;

    virtual void Start() = 0;
    virtual void Update(float dt) = 0;
    virtual void DrawOverlay() = 0;

    virtual bool IsFinished() const = 0;
    virtual bool ShouldSwapSceneNow() const = 0;
};
