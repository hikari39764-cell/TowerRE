#pragma once
#include "HIKARI/HIKARI_Utility.h"
#include "HIKARI/HIKARI_Transform2D.h"
#include "BulletBehavior.h"

namespace HIKARI { namespace PARTICLE { class Emitter; } }

struct BulletStyle {
    std::string name;
    std::string textureName; 
    std::string flyFxName;
    std::string spawnFxName;
    std::string hitFxName;
    float baseRadius = 8.0f;
};

struct Bullet {
    Vector2 pos{};
    Vector2 vel{};
    Vector2 acc{}; 

    float radius = 8.0f;
    int damage = 1;

    bool alive = true;
    bool isEnemy = false;

    float life = 0.0f;
    float ttl = 5.0f;

    IBulletBehavior* behavior = nullptr;

    HIKARI::PARTICLE::Emitter* activeEmitter = nullptr;
    HIKARI::Transform2D visualsTransform{};
    std::string onDeathFxName;
    std::string styleName; 
};