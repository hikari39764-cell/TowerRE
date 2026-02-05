#pragma once
#include <cmath>
#include "BulletBehavior.h"
#include "Bullet.h"


struct BulletLinear : public IBulletBehavior {
    void Update(Bullet& b, float dt) override {
        b.pos.x += b.vel.x * dt;
        b.pos.y += b.vel.y * dt;
    }
};

struct BulletAccel : public IBulletBehavior {
    void Update(Bullet& b, float dt) override {
        b.vel.x += b.acc.x * dt;
        b.vel.y += b.acc.y * dt;
        b.pos.x += b.vel.x * dt;
        b.pos.y += b.vel.y * dt;
    }
};

struct BulletSineWave : public IBulletBehavior {
    float baseSpeed = 300.0f;
    float amp = 120.0f;  
    float freq = 4.0f; 

    void Update(Bullet& b, float dt) override {
        b.pos.y += baseSpeed * dt;
        b.pos.x += std::sin(b.life * freq) * amp * dt;
    }
};
