#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include "Bullet.h"
#include <list> 
#include "HIKARI.h"

class BulletManager {
public:
    ~BulletManager(); 

    void Init();
    void ClearAll();
    void RegisterStyle(const std::string& name, const BulletStyle& style);
    Bullet& Spawn(const Bullet& initData, const std::string& styleName);
    void Update(float dt);
    void Draw();
    std::list<Bullet>& GetBullets() { return bullets_; }
    HIKARI::PARTICLE::ParticleSystem& GetFxSystem() { return fx_; }

private:
    std::list<Bullet> bullets_;

    std::unordered_map<std::string, BulletStyle> styles_;
    HIKARI::PARTICLE::ParticleSystem fx_;

    HIKARI::POST::PostChain bulletChain_;
    HIKARI::POST::PostEffect* bulletGlowEffect_ = nullptr;

    void InitCommonStyles();
    void KillBullet(Bullet& b);
    void InitPostEffects(); 
};