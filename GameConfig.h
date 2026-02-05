#pragma once

namespace GAMECFG {
    static const int kGameW = 1280;
    static const int kGameH = 1000;

    // 玩家

    static const float kPlayerRadius = 14.0f;
    static const float kPlayerBaseSpeed = 320.0f;
    static const float kPlayerSpeedStep = 60.0f;
    static const int   kPlayerMaxSpeedLv = 5;

    static const int   kPlayerMaxHp = 5;

    static const float kShotIntervalBase = 0.10f;
    static const int   kPlayerMaxShotLv = 5;

    // 魔力点
    static const float kManaInterval = 2.0f;
    static const int   kManaMax = 6;

    // 消耗
    static const int kCostUpgradeSpeed = 1; // I
    static const int kCostUpgradeShot = 1; // U
    static const int kCostHeal = 2; // K
    static const int kCostDash = 1; // J

    // CD
    static const float kDashCooldown = 0.60f;
    static const float kHealCooldown = 1.80f;

    // dash 参数
    static const float kDashDuration = 0.12f;
    static const float kDashSpeedMul = 3.0f;

    // 开场飞入
    static const float kIntroTime = 0.65f;

    static const char* kStageBgPath = "./images/stage/normal.png";     // 1280x2112
    static const char* kStageFgPath = "./images/stage/normal_fg.png";  // 1280x1000

    // 尺寸（用于循环滚动计算）
    static const int kStageBgH = 2112;
    static const int kStageFgH = 1000;

    // 滚动速度（像素/秒）
    static const float kScrollSpeedBg = 180.0f;
    static const float kScrollSpeedFg = 180.0f;
}
