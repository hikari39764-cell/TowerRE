#include "Scene_Title.h"

#include <memory>

#include "FadeTransition.h"
#include "HIKARI/HIKARI.h"
#include "SceneManager.h"
#include "ScrollBackground.h"

Scene_Title::Scene_Title(ScrollBackground& bg)
    : bg_(bg)
{
}

void Scene_Title::OnCreate()
{
    // no-op
}

void Scene_Title::OnEnter()
{
    requested_ = false;
}

void Scene_Title::OnExit()
{
}

void Scene_Title::Update(float dt)
{
    bg_.Update(dt);

    if (!requested_ && HIKARI::HINPUT::IsPressed("Space")) {
        requested_ = true;
        if (mgr_ != nullptr) {
            mgr_->RequestChange(SceneId::Game, std::make_unique<FadeTransition>(0.25f, 0.25f));
        }
    }
}

void Scene_Title::Draw()
{
    bg_.Draw();
}
