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
	spineAnimation_.Load("./Animation/titleTower.atlas", "./Animation/title.json");
	spineAnimation_2.Load("./Animation/titleTower.atlas", "./Animation/title.json");
    HIKARI::TEXTURE::Register("titleXbg", "./images/Ui/uiBgTitle", "titleScene");
    HIKARI::TEXTURE::LoadGroup("titleScene");
    spineAnimation_.transform = { 640.0f,320.0f };
    spineAnimation_2.transform = { 640.0f,620.0f };
}

void Scene_Title::OnEnter()
{
	spineAnimation_.SetAnimation("titleIn", true);
	spineAnimation_2.SetAnimation("startIn", true);
    requested_ = false;
}

void Scene_Title::OnExit()
{
}

void Scene_Title::Update(float dt)
{
    bg_.Update(dt);
    spineAnimation_2.Update(kDt);
    spineAnimation_.Update(kDt);
    if (!requested_ && HIKARI::HINPUT::IsPressed("Space")) {
        requested_ = true;
        if (mgr_ != nullptr) {
			spineAnimation_.SetAnimation("titleOut", false);
			spineAnimation_2.SetAnimation("startOut", false);
            mgr_->RequestChange(SceneId::Game, std::make_unique<FadeTransition>(0.25f, 0.25f));
        }
    }
}

void Scene_Title::Draw()
{
    bg_.Draw();
    spineAnimation_.Draw();
    spineAnimation_2.Draw();
    HIKARI::Transform2D t;
    t.position = { 1280.0f,0.0f };
    HIKARI::RENDERER::DrawSprite("titleXbg", t, 498.0f, 1000.0f);
}
