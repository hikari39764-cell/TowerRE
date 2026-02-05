#pragma once
#include "HIKARI.h"
#include "Player.h"
#include "Boss.h"
#include <string>

class GameHUD {
public:
    void Init();
    void Update(float dt, const Player& player, const Boss& boss);
    void Draw(const Player& player, const Boss& boss);

private:
    void RegisterAllTextures();
    void DrawBar(const std::string& texName, float x, float y, float rate); // 辅助函数

private:

    float textY_ = -700.0f;
    bool  isGameOverAnim_ = false;
    bool  isGameClearAnim_ = false;


    const std::string kBgNormal = "ui_bg_normal";
    const std::string kBgIce = "ui_bg_ice";
    const std::string kBgFire = "ui_bg_fire";
    const std::string kBgFinal = "ui_bg_final";

    // Boss HP
    const std::string kBossHpBg = "ui_boss_hp_bg";
    const std::string kBossHpBar = "ui_boss_hp_bar";
    const std::string kBossHpFg = "ui_boss_hp_fg";
    const std::string kBossName = "ui_boss_name";

    // Player HP
    const std::string kPlHpBg = "ui_pl_hp_bg";
    const std::string kPlHpBar = "ui_pl_hp_bar";
    const std::string kPlHpFg = "ui_pl_hp_fg";
    const std::string kPlHpIcon = "ui_pl_hp_icon";

    // Player MP
    const std::string kPlMpBg = "ui_pl_mp_bg";   // 原 iconHandle[2][0]
    const std::string kPlMpFg = "ui_pl_mp_fg";   // 原 iconHandle[2][1]
    const std::string kPlMpIcon = "ui_pl_mp_icon"; // 原 iconHandle[2][2]
    const std::string kPlMpGem = "ui_pl_mp_gem";  // 原 mp_02.png (实际上是 iconHandle[2][3]?)
    const std::string kPlMpBarSmall = "ui_mp_bar_s"; // 原 mp_01.png (iconHandle[5][2])
    const std::string kPlMpFrameSmall = "ui_mp_frame_s"; // 原 mp_03.png (iconHandle[5][3])

    // Icons (Left)
    const std::string kIconBase0 = "ui_icon_base0"; // ui_0.png
    const std::string kIconAtk = "ui_icon_atk";
    const std::string kIconSpd = "ui_icon_spd";
    const std::string kIconLevel = "ui_icon_lvl";   // levelMark.png

    // Icons (Right)
    const std::string kIconBase1 = "ui_icon_base1"; // ui_1.png
    const std::string kIconDash = "ui_icon_dash";
    const std::string kIconHeal = "ui_icon_heal";
    const std::string kIconEnd = "ui_icon_end";   // ending.png

    // Keys 
    const std::string kKeyJ = "ui_key_j"; // key_0
    const std::string kKeyK = "ui_key_k"; // key_1
    const std::string kKeyJ_G = "ui_key_j_gray"; // key_2
    const std::string kKeyK_G = "ui_key_k_gray"; // key_3
    const std::string kKeyI = "ui_key_i"; // key_4
    const std::string kKeyU = "ui_key_u"; // key_5
    const std::string kKeyCommon = "ui_key_common"; // key_6

    // Result
    const std::string kGameOver = "ui_game_over";
    const std::string kEndingInf = "ui_ending_inferno";
};