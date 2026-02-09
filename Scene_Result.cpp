#include "Scene_Result.h"

#include <memory>

#include "FadeTransition.h"
#include "HIKARI/HIKARI.h"
#include "SceneManager.h"

void Scene_Result::OnCreate()
{
    // no-op
}

void Scene_Result::OnEnter()
{
    requested_ = false;
}

void Scene_Result::OnExit()
{
}

void Scene_Result::Update(float dt)
{
    (void)dt;
    if (!requested_ && HIKARI::HINPUT::IsPressed("Space")) {
        requested_ = true;
        if (mgr_ != nullptr) {
            mgr_->RequestChange(SceneId::Title, std::make_unique<FadeTransition>(0.25f, 0.25f));
        }
    }
}

void Scene_Result::Draw()
{

    Novice::ScreenPrintf(320, 300, "SPACE TO RESTART");
}
