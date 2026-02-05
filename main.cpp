#include <Novice.h>
#include "HIKARI/HIKARI.h"
#include "ScrollBackground.h"
#include "Scene_Title.h"
#include "Scene_Game.h"
#include "MagicAnimeGlobalFX.h"
const char kWindowTitle[] = "HIKARI_Ver1.2";

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

    // 下層サービスの起動（入力/レンダラー/相机等）
    HIKARI::SERVICES::BootstrapConfig servicesCfg{};
    HIKARI::SERVICES::Initialize(kWindowTitle, servicesCfg);

    // デフォルトのメイン入力レイヤー
    HIKARI::HINPUT::SwitchLayer("Debug");
    static ScrollBackground gBg;
    gBg.LoadOnce();
    gBg.ResetScroll(); 
    gBg.SetStage(ScrollBackground::Stage::Normal);
    // パーティクルラボの初期化（デバッグ用途、サービスレイヤーの上に載せる）
    HIKARI::LAB::ParticleLab particleLab;
    static bool isParticleLab = false;
    particleLab.Init();

    MagicAnimeGlobalFX gMagicFX;
    gMagicFX.InitOnce();
    // ウィンドウの×ボタンが押されるまでループ
    while (Novice::ProcessMessage() == 0) {
        // フレームの開始（サービス層：入力更新/相机更新 等）
        HIKARI::SERVICES::BeginFrame(servicesCfg);
        gMagicFX.UpdateParams();
        static bool sceneInited = false;
        static Scene_Title sTitle(gBg);
        static Scene_Game  sGame(gBg);
        static IScene* current = nullptr;

        if (HIKARI::HINPUT::IsPressed("X"))
        {
            gBg.BeginTransition(ScrollBackground::Stage::Ice);   // 测 Normal -> Ice（冰冻刷下）
        } else if (HIKARI::HINPUT::IsPressed("B")){
            gBg.BeginTransition(ScrollBackground::Stage::Fire);

        } else if (HIKARI::HINPUT::IsPressed("C")) {
            gBg.SetStage(ScrollBackground::Stage::Normal);
        }

        auto SwitchScene = [&](SceneId id) {
            if (id == SceneId::Title) { current = &sTitle; } else { current = &sGame; }
            current->Init();
            };

        if (!sceneInited) {
            SwitchScene(SceneId::Title);
            sceneInited = true;
        }

        current->Update(kDt);

        SceneId next{};
        if (current->WantsNext(next)) {
            SwitchScene(next);
        }

        current->Draw();

        if (HIKARI::HINPUT::IsPressed("OpenParticleLab")) { isParticleLab = !isParticleLab; }
        if (isParticleLab) { particleLab.Update(kDt); particleLab.Draw(); }

        ///
        /// ↑パーティクルラボここまで
        ///

        // フレームの終了（サービス層）
        HIKARI::SERVICES::EndFrame();

        // ESCキーが押されたらループを抜ける
        if (HIKARI::HINPUT::IsPressed("CloseProgram")) {
            break;
        }
    }

    // 下層サービスの終了処理
    HIKARI::SERVICES::FinalizeAll();
    return 0;
}
