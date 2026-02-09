#include "GameHUD.h"

using namespace HIKARI;
using namespace HIKARI::RENDERER;

void GameHUD::Init() {
    RegisterAllTextures();
    textY_ = -700.0f;
    isGameOverAnim_ = false;
    isGameClearAnim_ = false;
}

void GameHUD::RegisterAllTextures() {
    using namespace HIKARI::TEXTURE;

    // 背景
    Register(kBgNormal, "./images/Ui/uiBg.png", "ui");
    Register(kBgIce, "./images/Ui/uiBg_ice.png", "ui");
    Register(kBgFire, "./images/Ui/uiBg_fire.png", "ui");
    Register(kBgFinal, "./images/Ui/uiBg_final.png", "ui");

    // Boss HP
    Register(kBossHpBar, "./images/Ui/bossHp.png", "ui");
    Register(kBossHpFg, "./images/Ui/bossHpFg.png", "ui");
    Register(kBossHpBg, "./images/Ui/bossHpBg.png", "ui");
    Register(kBossName, "./images/Ui/bossName.png", "ui");

    // Player HP
    Register(kPlHpBg, "./images/Ui/newHpBg.png", "ui");
    Register(kPlHpBar, "./images/Ui/newHp.png", "ui");     // iconHandle[1][0]
    Register(kPlHpFg, "./images/Ui/newHpFg.png", "ui");   // iconHandle[1][1]
    Register(kPlHpIcon, "./images/Ui/newHp_icon.png", "ui");// iconHandle[1][2]

    // Player MP
    Register(kPlMpBg, "./images/Ui/newMp.png", "ui");      // iconHandle[2][0]
    Register(kPlMpFg, "./images/Ui/newMpFg.png", "ui");    // iconHandle[2][1]
    Register(kPlMpIcon, "./images/Ui/newMp_icon.png", "ui"); // iconHandle[2][2]
    Register(kPlMpGem, "./images/Ui/mp_02.png", "ui");      // iconHandle[2][3]

    // Bars
    Register(kPlMpBarSmall, "./images/Ui/mp_01.png", "ui"); // iconHandle[5][2]
    Register(kPlMpFrameSmall, "./images/Ui/mp_03.png", "ui"); // iconHandle[5][3]

    // Icons Left
    Register(kIconBase0, "./images/Ui/ui_0.png", "ui");
    Register(kIconAtk, "./images/Ui/attackLevel.png", "ui");
    Register(kIconSpd, "./images/Ui/speedLevel.png", "ui");
    Register(kIconLevel, "./images/Ui/levelMark.png", "ui");

    // Icons Right
    Register(kIconBase1, "./images/Ui/ui_1.png", "ui");
    Register(kIconDash, "./images/Ui/dash.png", "ui");
    Register(kIconHeal, "./images/Ui/heal.png", "ui");
    Register(kIconEnd, "./images/Ui/ending.png", "ui"); // iconHandle[3][3]

    // Keys
    Register(kKeyJ, "./images/Ui/key_0.png", "ui");
    Register(kKeyK, "./images/Ui/key_1.png", "ui");
    Register(kKeyJ_G, "./images/Ui/key_2.png", "ui");
    Register(kKeyK_G, "./images/Ui/key_3.png", "ui");
    Register(kKeyI, "./images/Ui/key_4.png", "ui");
    Register(kKeyU, "./images/Ui/key_5.png", "ui");
    Register(kKeyCommon, "./images/Ui/key_6.png", "ui");

    // Result
    Register(kGameOver, "./images/Ui/gameOver.png", "ui"); // iconHandle[1][3]
    Register(kEndingInf, "./images/Ui/endingInferno.png", "ui");

    TEXTURE::LoadGroup("ui");
}

void GameHUD::Update(float dt, const Player& player, const Boss& boss) {
    bool pDead = player.IsDead();
    bool bDead = boss.GetHp() <= 0;

    if (pDead || bDead) {
        // 文字下落动画
        if (textY_ < 100.0f) {
            textY_ += 200.0f * dt; // 调整下落速度
        }
    }
}

void GameHUD::DrawBar(const std::string& texName, float x, float y, float rate) {
    if (rate < 0.0f) rate = 0.0f;
    if (rate > 1.0f) rate = 1.0f;

    // 获取纹理原始尺寸
    int handle = TEXTURE::GetDxHandle(texName);
    UINT w = 0, h = 0;
    DXTEX::DxTextureManager::GetTextureSize(handle, w, h);

    if (w > 0) {
        Transform2D t;
        t.position = { x, y };
        t.scale = { rate, 1.0f }; // 只缩放 X 轴
        // 传递原始宽高，让内部去缩放
        RENDERER::DrawSprite(texName, t, (float)w, (float)h, CameraMode::Ignore);
    }
}

void GameHUD::Draw(const Player& player, const Boss& boss) {


    if (player.IsDead() || boss.GetHp() <= 0) {
        // 画背景 (Boss当前状态的背景)
        std::string bgKey = kBgNormal;
        if (boss.GetGlobalPhase() == GlobalPhase::Ice) bgKey = kBgIce;
        else if (boss.GetGlobalPhase() == GlobalPhase::Fire) bgKey = kBgFire;
        else if (boss.GetGlobalPhase() == GlobalPhase::Final) bgKey = kBgFinal;

        Transform2D tBg; tBg.position = { 1280.0f, 0.0f };
        RENDERER::DrawSprite(bgKey, tBg, 498.0f, 1000.0f, CameraMode::Ignore);

        // 画下落文字
        Transform2D tText; tText.position = { 1280.0f, textY_ };

        if (player.IsDead()) {
            RENDERER::DrawSprite(kGameOver, tText, 498.0f, 652.0f, CameraMode::Ignore); // iconHandle[1][3]
        } else {
            // Boss Dead (Clear)
            // 根据难度显示不同的通关图
            // 假设 Boss 没有 GetDifficulty()，你需要传进来或者怎么获取
            // 暂时假定 Normal 显示 kIconEnd, Challenge 显示 kEndingInf
            // 如果你把 difficulty 存到 Boss 里了，可以用 boss.GetDifficulty()
            // 这里用默认逻辑：
            RENDERER::DrawSprite(kIconEnd, tText, 498.0f, 652.0f, CameraMode::Ignore);
        }
        return; // 结算时不再绘制 HUD
    }


    std::string bgKey = kBgNormal;
    if (boss.GetGlobalPhase() == GlobalPhase::Ice) bgKey = kBgIce;
    else if (boss.GetGlobalPhase() == GlobalPhase::Fire) bgKey = kBgFire;
    else if (boss.GetGlobalPhase() == GlobalPhase::Final) bgKey = kBgFinal;

    Transform2D tBg; tBg.position = { 1280.0f, 0.0f };
    RENDERER::DrawSprite(bgKey, tBg, 498.0f, 1000.0f, CameraMode::Ignore);


    Transform2D tBoss; tBoss.position = { 1310.0f, 664.0f };
    RENDERER::DrawSprite(kBossHpBg, tBoss, 431.0f, 44.0f, CameraMode::Ignore);

 
    float bossRate = boss.GetHp() / boss.GetMaxHp();
    DrawBar(kBossHpBar, 1310.0f, 664.0f, bossRate);

  
    tBoss.position = { 1280.0f, 626.0f };
    RENDERER::DrawSprite(kBossHpFg, tBoss, 498.0f, 110.0f, CameraMode::Ignore);


    tBoss.position = { 1280.0f, 690.0f };
    RENDERER::DrawSprite(kBossName, tBoss, 498.0f, 64.0f, CameraMode::Ignore);


    Transform2D tIcon;


    tIcon.position = { 1310.0f, 300.0f };
    RENDERER::DrawSprite(kIconBase0, tIcon, 64.0f, 64.0f, CameraMode::Ignore);

    tIcon.position = { 1310.0f, 370.0f };
    RENDERER::DrawSprite(kIconAtk, tIcon, 64.0f, 64.0f, CameraMode::Ignore);

    tIcon.position = { 1310.0f, 440.0f };
    RENDERER::DrawSprite(kIconSpd, tIcon, 64.0f, 64.0f, CameraMode::Ignore);

    tIcon.position = { 1550.0f, 440.0f };
    RENDERER::DrawSprite(kIconDash, tIcon, 64.0f, 64.0f, CameraMode::Ignore);

    tIcon.position = { 1550.0f, 370.0f };
    RENDERER::DrawSprite(kIconHeal, tIcon, 64.0f, 64.0f, CameraMode::Ignore);

    tIcon.position = { 1370.0f, 295.0f };
    RENDERER::DrawSprite(kKeyU, tIcon, 64.0f, 64.0f, CameraMode::Ignore);
    tIcon.position = { 1410.0f, 295.0f };
    RENDERER::DrawSprite(kKeyI, tIcon, 64.0f, 64.0f, CameraMode::Ignore);

    int shotLv = player.GetShotLv();
    for (int j = 0; j < shotLv; j++) {
        tIcon.position = { 1380.0f + 20.0f * (j % 6), 375.0f + 20.0f * (j / 6) };
        RENDERER::DrawSprite(kIconLevel, tIcon, 21.0f, 20.0f, CameraMode::Ignore);
    }

    int spdLv = player.GetSpeedLv();
    for (int j = 0; j < spdLv; j++) {
        tIcon.position = { 1380.0f + 20.0f * (j % 6), 445.0f + 20.0f * (j / 6) };
        RENDERER::DrawSprite(kIconLevel, tIcon, 21.0f, 20.0f, CameraMode::Ignore);
    }


    tIcon.position = { 1550.0f, 300.0f };
    RENDERER::DrawSprite(kIconBase1, tIcon, 64.0f, 64.0f, CameraMode::Ignore);


    bool canDash = (player.GetDashCd() <= 0.0f);
    tIcon.position = { 1620.0f, 370.0f }; 

    RENDERER::DrawSprite(canDash ? kKeyJ : kKeyJ_G, tIcon, 64.0f, 64.0f, CameraMode::Ignore);


    bool canHeal = (player.GetHealCd() <= 0.0f);
    tIcon.position = { 1620.0f, 440.0f };
    RENDERER::DrawSprite(canHeal ? kKeyK : kKeyK_G, tIcon, 64.0f, 64.0f, CameraMode::Ignore);


    float dashRate = player.GetDashCd() / player.GetMaxDashCd();
    DrawBar(kPlMpBarSmall, 1600.0f, 420.0f, dashRate);
    tIcon.position = { 1600.0f, 420.0f };
    RENDERER::DrawSprite(kPlMpFrameSmall, tIcon, 110.0f, 3.0f, CameraMode::Ignore);

 
    float healRate = player.GetHealCd() / player.GetMaxHealCd();
    DrawBar(kPlMpBarSmall, 1600.0f, 490.0f, healRate);
    tIcon.position = { 1600.0f, 490.0f };
    RENDERER::DrawSprite(kPlMpFrameSmall, tIcon, 110.0f, 3.0f, CameraMode::Ignore);

 
    tIcon.position = { 1370.0f, 100.0f };
    RENDERER::DrawSprite(kPlHpBg, tIcon, 386.0f, 23.0f, CameraMode::Ignore);
 
    float plHpRate = player.GetHp() / player.GetMaxHp();
    DrawBar(kPlHpBar, 1370.0f, 100.0f, plHpRate);
   
    RENDERER::DrawSprite(kPlHpFg, tIcon, 386.0f, 23.0f, CameraMode::Ignore);
 
    tIcon.position = { 1300.0f, 80.0f };
    RENDERER::DrawSprite(kPlHpIcon, tIcon, 64.0f, 60.0f, CameraMode::Ignore);

    float mpChargeRate = player.GetManaTimer() / player.GetManaInterval();
    DrawBar(kPlMpBg, 1370.0f, 220.0f, mpChargeRate);

    tIcon.position = { 1370.0f, 220.0f };
    RENDERER::DrawSprite(kPlMpFg, tIcon, 379.0f, 5.0f, CameraMode::Ignore);


    tIcon.position = { 1300.0f, 160.0f };
    RENDERER::DrawSprite(kPlMpIcon, tIcon, 64.0f, 60.0f, CameraMode::Ignore);

   
    int mp = player.GetMana();
    for (int i = 0; i < mp; i++) {
        tIcon.position = { 1380.0f + 50.0f * (i % 6), 170.0f }; 
        RENDERER::DrawSprite(kPlMpGem, tIcon, 42, 40, CameraMode::Ignore);
    }

    tIcon.position = { 1280.0f, 0.0f };
    RENDERER::DrawSprite(kKeyCommon, tIcon, 0, 0, CameraMode::Ignore); 
}