#pragma once
struct Bullet;

struct IBulletBehavior {
    virtual ~IBulletBehavior() {}
    virtual void OnSpawn(Bullet& b) {}
    virtual void Update(Bullet& b, float dt) = 0;
};