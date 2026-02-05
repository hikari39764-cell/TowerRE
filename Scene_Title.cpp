#include "Scene_Title.h"
#include "ScrollBackground.h"
#include "HIKARI/HIKARI.h"

Scene_Title::Scene_Title(ScrollBackground& bg)
    : bg_(bg)
{
}

void Scene_Title::Init()
{
    wantNext_ = false;
}

void Scene_Title::Update(float dt)
{
    bg_.Update(dt);

    if (HIKARI::HINPUT::IsPressed("Space")) {
        wantNext_ = true;
    }
}

void Scene_Title::Draw()
{
    bg_.Draw();


}

bool Scene_Title::WantsNext(SceneId& outNext)
{
    if (wantNext_) {
        outNext = SceneId::Game;
        return true;
    }
    return false;
}
