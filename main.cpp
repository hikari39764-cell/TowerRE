#include <memory>

#include <Novice.h>

#include "HIKARI/HIKARI.h"
#include "MagicAnimeGlobalFX.h"
#include "SceneManager.h"
#include "Scene_Game.h"
#include "Scene_Result.h"
#include "Scene_Title.h"
#include "ScrollBackground.h"

const char kWindowTitle[] = "HIKARI_Ver1.2";

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    HIKARI::SERVICES::BootstrapConfig servicesCfg{};
    HIKARI::SERVICES::Initialize(kWindowTitle, servicesCfg);

    HIKARI::HINPUT::SwitchLayer("Debug");

    static ScrollBackground gBg;
    gBg.LoadOnce();
    gBg.ResetScroll();
    gBg.SetStage(ScrollBackground::Stage::Normal);

    HIKARI::LAB::ParticleLab particleLab;
    static bool isParticleLab = false;
    particleLab.Init();

    MagicAnimeGlobalFX gMagicFX;
    gMagicFX.InitOnce();

    SceneManager mgr;
    mgr.Register(SceneId::Title, std::make_unique<Scene_Title>(gBg));
    mgr.Register(SceneId::Game, std::make_unique<Scene_Game>(gBg));
    mgr.Register(SceneId::Result, std::make_unique<Scene_Result>());
    mgr.Boot(SceneId::Title);

    while (Novice::ProcessMessage() == 0) {
        HIKARI::SERVICES::BeginFrame(servicesCfg);
        gMagicFX.UpdateParams();

        if (HIKARI::HINPUT::IsPressed("X")) {
            gBg.BeginTransition(ScrollBackground::Stage::Ice);
        } else if (HIKARI::HINPUT::IsPressed("B")) {
            gBg.BeginTransition(ScrollBackground::Stage::Fire);
        } else if (HIKARI::HINPUT::IsPressed("C")) {
            gBg.SetStage(ScrollBackground::Stage::Normal);
        }

        mgr.Update(kDt);
        mgr.Draw();

        if (HIKARI::HINPUT::IsPressed("OpenParticleLab")) {
            isParticleLab = !isParticleLab;
        }
        if (isParticleLab) {
            particleLab.Update(kDt);
            particleLab.Draw();
        }

        HIKARI::SERVICES::EndFrame();

        if (HIKARI::HINPUT::IsPressed("CloseProgram")) {
            break;
        }
    }

    HIKARI::SERVICES::FinalizeAll();
    return 0;
}
