#pragma once
#include "BulletManager.h"
#include "BulletBehaviors_Basic.h"
#include <cmath>
#include <vector>

namespace BulletPatterns {

    static const float PI = 3.14159265f;
    static const float HALF_PI = PI / 2.0f;

    inline Vector2 AngleToVel(float refAngle, float speed) {
        return Vector2{
            -std::cos(refAngle) * speed,
            -std::sin(refAngle) * speed
        };
    }

    inline void SpawnOne(BulletManager& mgr, Vector2 pos, float refAngle, float speed, int damage, const std::string& style) {
        Bullet b;
        b.pos = pos;
        b.damage = damage;
        b.isEnemy = false;
        b.ttl = 4.0f;
        b.vel = AngleToVel(refAngle, speed);

        static BulletLinear linear; 
        b.behavior = &linear;

        mgr.Spawn(b, style);
    }


    inline void PlayerShoot(BulletManager& mgr, int bulletType, float x, float y) {

        switch (bulletType) {
            // --- 基础直线弹类型 ---
        case 0: {
            SpawnOne(mgr, { x, y }, HALF_PI, 1020.0f, 4, "normal");
            break;
        }
        case 1: {
            SpawnOne(mgr, { x, y }, HALF_PI, 600.0f, 4, "normal");
            break;
        }
        case 2: {
            SpawnOne(mgr, { x + 8.0f , y }, HALF_PI, 480.0f, 4, "normal");
            SpawnOne(mgr, { x - 8.0f, y }, HALF_PI, 480.0f, 4, "normal");
            break;
        }
        case 3: {
            SpawnOne(mgr, { x - 8.0f, y }, HALF_PI, 600.0f, 4, "normal");
            SpawnOne(mgr, { x + 8.0f, y }, HALF_PI, 600.0f, 4, "normal");
            break;
        }
        case 4: {

            SpawnOne(mgr, { x, y }, HALF_PI, 750.5f, 4, "normal");
            SpawnOne(mgr, { x - 8.0f, y }, HALF_PI - 0.08f, 480.0f, 4, "normal");
            SpawnOne(mgr, { x + 8.0f, y }, HALF_PI + 0.08f, 480.0f, 4, "normal");
            break;
        }

              // --- 激光起始 ---
        case 5: {
            SpawnOne(mgr, { x, y }, HALF_PI, 900.0f, 16, "beam");
            break;
        }
        case 6: {
            SpawnOne(mgr, { x, y }, HALF_PI, 1350.0f, 16, "beam");
            break;
        }
        case 7: {
            SpawnOne(mgr, { x, y }, HALF_PI, 1110.0f, 10, "beam");

            SpawnOne(mgr, { x - 8.0f, y }, HALF_PI - 0.08f, 480.0f, 3, "normal2");

            SpawnOne(mgr, { x + 8.0f, y }, HALF_PI + 0.08f, 480.0f, 3, "normal2");
            break;
        }
        case 8: {
            SpawnOne(mgr, { x, y }, HALF_PI, 1170.0f, 12, "beam_plus");
            SpawnOne(mgr, { x  - 8.0f, y }, HALF_PI - 0.08f, 600.0f, 3, "plus");
            SpawnOne(mgr, { x + 8.0f, y }, HALF_PI + 0.08f, 600.0f, 3, "plus");
            break;
        }
        case 9: {

            SpawnOne(mgr, { x, y }, HALF_PI, 1230.0f, 12, "beam_plus");

            SpawnOne(mgr, { x + 16, y }, HALF_PI - 0.2f, 780.0f, 10, "beam_plus");

            SpawnOne(mgr, { x - 16, y }, HALF_PI + 0.2f, 780.0f, 10, "beam_plus");
            break;
        }
        case 10: {

            SpawnOne(mgr, { x, y }, HALF_PI, 1380.0f, 13, "beam_final");
            SpawnOne(mgr, { x + 16, y }, HALF_PI - 0.2f, 780.0f, 11, "beam_final");
            SpawnOne(mgr, { x - 16, y }, HALF_PI + 0.2f, 780.0f, 11, "beam_final");
            break;
        }
        case 11: {

            SpawnOne(mgr, { x, y }, HALF_PI, 1500.0f, 13, "beam_final");
            SpawnOne(mgr, { x + 16, y }, HALF_PI - 0.2f, 900.0f, 11, "beam_final");
            SpawnOne(mgr, { x - 16, y }, HALF_PI + 0.2f, 900.0f, 11, "beam_final");

            SpawnOne(mgr, { x + 8, y }, HALF_PI, 510.0f, 2, "final");
            SpawnOne(mgr, { x - 24, y }, HALF_PI, 510.0f, 2, "final");

            break;
        }
        case 12: {
            // The Ultimate Pattern
            // 3 Lasers
            // Center
            SpawnOne(mgr, { x, y }, HALF_PI, 1560.0f, 14, "beam_final");
            // Left Laser
            SpawnOne(mgr, { x - 15, y }, HALF_PI + 0.18f, 1200.0f, 11, "beam_final");
            // Right Laser
            SpawnOne(mgr, { x + 15, y }, HALF_PI - 0.18f, 1200.0f, 11, "beam_final");

            // 4 Scatter Final Bullets
            // s1
            SpawnOne(mgr, { x - 20, y }, HALF_PI - 0.07f, 600.0f, 2, "final");
            // s2
            SpawnOne(mgr, { x + 20, y }, HALF_PI + 0.07f, 600.0f, 2, "final");
            // s3
            SpawnOne(mgr, { x - 30, y }, HALF_PI - 0.18f, 540.0f, 2, "final");
            // s4
            SpawnOne(mgr, { x + 30, y }, HALF_PI + 0.18f, 540.0f, 2, "final");
            break;
        }
        default:
            break;
        }
    }
}