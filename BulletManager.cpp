#include "BulletManager.h"
#include "GameConfig.h" // 假设包含窗口大小等配置
#include "HIKARI.h" // 引入纹理系统
#include <algorithm>
#include <cmath>

using namespace HIKARI;
using namespace HIKARI::PARTICLE;

// 定义路径常量 (完全复刻参考代码的路径)
namespace {
    const std::string kPathNormal = "./images/PlayerBullets/bulletNormal.png";
    const std::string kPathNormal2 = "./images/PlayerBullets/bullet.png";
    const std::string kPathPlus = "./images/PlayerBullets/bulletPlus.png";
    const std::string kPathFinal = "./images/PlayerBullets/bulletFinal.png";
    const std::string kPathBeam = "./images/PlayerBullets/beam.png";
    const std::string kPathBeamPlus = "./images/PlayerBullets/beamPlus.png";
    const std::string kPathBeamFinal = "./images/PlayerBullets/beamFinal.png";
    const std::string kPathEnemyNormal = "./images/EnemyBullets/EBullet.png";
    const std::string kPathEnemyIce = "./images/EnemyBullets/EBulletIce.png";
    const std::string kPathEnemyFire = "./images/EnemyBullets/EBulletFire.png";
    const std::string kPathEnemyBeam = "./images/EnemyBullets/Beam.png";
    const std::string kPathEnemyBeamIce = "./images/EnemyBullets/BeamIce.png";
#if 0
    const std::string kPathEnemyFinal = "./images/EnemyBullets/EBulletFinal.png";
    const std::string kPathEnemyBeamFinal = "./images/EnemyBullets/beamFinal.png";
#endif
}

BulletManager::~BulletManager() {
    ClearAll();
    // 清理特效内存
    if (bulletGlowEffect_) {
        delete bulletGlowEffect_;
        bulletGlowEffect_ = nullptr;
    }
}

void BulletManager::Init() {
    using namespace HIKARI::RENDERER;
    using namespace HIKARI::PARTICLE::PRESET;

    HIKARI::TEXTURE::Register("tex_p_normal", kPathNormal);
    HIKARI::TEXTURE::Register("tex_p_normal2", kPathNormal2);
    HIKARI::TEXTURE::Register("tex_p_plus", kPathPlus);
    HIKARI::TEXTURE::Register("tex_p_final", kPathFinal);
    HIKARI::TEXTURE::Register("tex_p_beam", kPathBeam);
    HIKARI::TEXTURE::Register("tex_p_beam_plus", kPathBeamPlus);
    HIKARI::TEXTURE::Register("tex_p_beam_final", kPathBeamFinal);
    HIKARI::TEXTURE::Register("tex_e_normal", kPathEnemyNormal);
    HIKARI::TEXTURE::Register("tex_e_ice", kPathEnemyIce);
    HIKARI::TEXTURE::Register("tex_e_fire", kPathEnemyFire);
    HIKARI::TEXTURE::Register("tex_e_beam", kPathEnemyBeam);
    HIKARI::TEXTURE::Register("tex_e_beam_ice", kPathEnemyBeamIce);
#if 0
    HIKARI::TEXTURE::Register("tex_e_final", kPathEnemyFinal);
    HIKARI::TEXTURE::Register("tex_e_beam_final", kPathEnemyBeamFinal);
#endif

    {
        EffectPrototype proto;

        proto.config = MakeTrailConfig(nullptr, 32, 0.2f, 0.5f, 60.0f, 0x00FFFF80, 0x00000000);
        proto.config.sizeMin = 8.0f; proto.config.sizeMax = 26.0f;
        proto.config.speedMax = 1200.0f;
        proto.config.speedMin = 600.0f;
        proto.drawFunc = MakeSpeedLineDrawer(CameraMode::Inherit);
        proto.spawnFunc = MakeTrailSpawn();
        fx_.RegisterEffect("fx_trail_cyan", proto);
    }
    {
        EffectPrototype proto;
        proto.config = MakeTrailConfig(nullptr, 32, 0.1f, 0.3f, 60.0f, 0xB020D0FF, 0x500A0018);
        proto.config.sizeMin = 16.0f; proto.config.sizeMax = 27.0f;
        proto.drawFunc = MakeCrossStarDrawer(true,CameraMode::Inherit);
        proto.spawnFunc = MakeTrailSpawn();
        fx_.RegisterEffect("fx_trail_gold", proto);
    }

    {
        EffectPrototype proto;
        proto.config = MakeRadialBurstConfig({ 0,0 }, 12, 0.2f, 0.4f, 50.0f, 150.0f, 0xFFFFFFFF, 0x00FFFFFF);
        proto.drawFunc = MakeCrossStarDrawer(true, CameraMode::Inherit);
        proto.spawnFunc = MakeRadialBurstSpawn();
        fx_.RegisterEffect("fx_hit_spark", proto);
    }

    InitCommonStyles();
    InitPostEffects();
}

void BulletManager::InitPostEffects() {

    bulletGlowEffect_ = new HIKARI::POST::PostEffect();


    if (!bulletGlowEffect_->LoadPixelShader(L"./shaders/BulletGlow.hlsl")) {
    }

    DirectX::XMFLOAT4 params[1];


    params[0].x = 3.0f;


    params[0].y = 8.0f;

    params[0].z = 0.5f;



    bulletGlowEffect_->SetUser(0, params[0]);
    bulletChain_.Add(bulletGlowEffect_);

    // 4. 将特效加入链中
    bulletChain_.Add(bulletGlowEffect_);
}

void BulletManager::InitCommonStyles() {
    // 定义子弹样式 (映射参考代码中的各个类)

    // 对应 PlayerBullet_Normal
    BulletStyle sNormal;
    sNormal.name = "normal";
    sNormal.textureName = "tex_p_normal"; // 绑定贴图
    sNormal.flyFxName = "fx_trail_cyan";  // 辅助粒子
    sNormal.hitFxName = "fx_hit_spark";
    sNormal.baseRadius = 8.0f;
    RegisterStyle("normal", sNormal);

    // 对应 PlayerBulletNormal2 (Icon: bullet.png)
    BulletStyle sNormal2;
    sNormal2.name = "normal2";
    sNormal2.textureName = "tex_p_normal2";
    sNormal2.flyFxName = "fx_trail_cyan";
    sNormal2.hitFxName = "fx_hit_spark";
    sNormal2.baseRadius = 8.0f;
    RegisterStyle("normal2", sNormal2);

    // 对应 PlayerBulletPlus
    BulletStyle sPlus;
    sPlus.name = "plus";
    sPlus.textureName = "tex_p_plus";
    sPlus.flyFxName = "fx_trail_gold";
    sPlus.hitFxName = "fx_hit_spark";
    sPlus.baseRadius = 8.0f;
    RegisterStyle("plus", sPlus);

    // 对应 PlayerBulletFinal
    BulletStyle sFinal;
    sFinal.name = "final";
    sFinal.textureName = "tex_p_final";
    sFinal.flyFxName = "fx_trail_gold";
    sFinal.hitFxName = "fx_hit_spark";
    sFinal.baseRadius = 8.0f;
    RegisterStyle("final", sFinal);


    BulletStyle sBeam;
    sBeam.name = "beam";
    sBeam.textureName = "tex_p_beam";
    sBeam.flyFxName = "fx_trail_cyan"; 
    sBeam.hitFxName = "fx_hit_spark";
    sBeam.baseRadius = 8.0f;
    RegisterStyle("beam", sBeam);


    BulletStyle sBeamPlus;
    sBeamPlus.name = "beam_plus";
    sBeamPlus.textureName = "tex_p_beam_plus";
    sBeamPlus.flyFxName = "fx_trail_gold";
    sBeamPlus.hitFxName = "fx_hit_spark";
    sBeamPlus.baseRadius = 8.0f;
    RegisterStyle("beam_plus", sBeamPlus);

    // 对应 PlayerBulletBeamFinal
    BulletStyle sBeamFinal;
    sBeamFinal.name = "beam_final";
    sBeamFinal.textureName = "tex_p_beam_final";
    sBeamFinal.flyFxName = "fx_trail_gold";
    sBeamFinal.hitFxName = "fx_hit_spark";
    sBeamFinal.baseRadius = 8.0f;
    RegisterStyle("beam_final", sBeamFinal);

    BulletStyle eNormal;
    eNormal.name = "e_normal";
    eNormal.textureName = "tex_e_normal";
    eNormal.flyFxName = "fx_trail_cyan";
    eNormal.hitFxName = "fx_hit_spark";
    eNormal.baseRadius = 8.0f;
    RegisterStyle("e_normal", eNormal);

    BulletStyle eIce;
    eIce.name = "e_ice";
    eIce.textureName = "tex_e_ice";
    eIce.flyFxName = "fx_trail_cyan";
    eIce.hitFxName = "fx_hit_spark";
    eIce.baseRadius = 8.0f;
    RegisterStyle("e_ice", eIce);

    BulletStyle eFire;
    eFire.name = "e_fire";
    eFire.textureName = "tex_e_fire";
    eFire.flyFxName = "fx_trail_gold";
    eFire.hitFxName = "fx_hit_spark";
    eFire.baseRadius = 8.0f;
    RegisterStyle("e_fire", eFire);

    BulletStyle eBeam;
    eBeam.name = "e_beam";
    eBeam.textureName = "tex_e_beam";
    eBeam.flyFxName = "fx_trail_cyan";
    eBeam.hitFxName = "fx_hit_spark";
    eBeam.baseRadius = 8.0f;
    RegisterStyle("e_beam", eBeam);

    BulletStyle eBeamIce;
    eBeamIce.name = "e_beam_ice";
    eBeamIce.textureName = "tex_e_beam_ice";
    eBeamIce.flyFxName = "fx_trail_cyan";
    eBeamIce.hitFxName = "fx_hit_spark";
    eBeamIce.baseRadius = 8.0f;
    RegisterStyle("e_beam_ice", eBeamIce);
#if 0
    BulletStyle eFinal;
    eFinal.name = "e_final";
    eFinal.textureName = "tex_e_final";
    eFinal.flyFxName = "fx_trail_gold";
    eFinal.hitFxName = "fx_hit_spark";
    eFinal.baseRadius = 8.0f;
    RegisterStyle("e_final", eFinal);
#endif
}

void BulletManager::RegisterStyle(const std::string& name, const BulletStyle& style) {
    styles_[name] = style;
}

void BulletManager::ClearAll() {
    for (auto& b : bullets_) {
        KillBullet(b);
    }
    bullets_.clear();
}

Bullet& BulletManager::Spawn(const Bullet& initData, const std::string& styleName) {
    bullets_.push_back(initData);
    Bullet& b = bullets_.back();

    b.visualsTransform.position = b.pos;
    b.visualsTransform.rotation = 0.0f;
    b.visualsTransform.scale = { 1.0f, 1.0f };

    auto it = styles_.find(styleName);
    if (it != styles_.end()) {
        const auto& style = it->second;

        b.styleName = styleName;
        if (b.radius <= 0.0f) b.radius = style.baseRadius;
        b.onDeathFxName = style.hitFxName;

        if (!style.spawnFxName.empty()) {
            fx_.PlayOneShot(style.spawnFxName, b.pos, 3);
        }
        if (!style.flyFxName.empty()) {
            b.activeEmitter = fx_.CreateEmitterFromPreset(style.flyFxName, &b.visualsTransform);
        }
    }

    if (b.behavior) {
        b.behavior->OnSpawn(b);
    }
    return b;
}

void BulletManager::Update(float dt) {
    for (auto it = bullets_.begin(); it != bullets_.end(); ) {
        Bullet& b = *it;

        if (!b.alive) {
            if (!b.onDeathFxName.empty()) {
                fx_.PlayOneShot(b.onDeathFxName, b.pos, 1);
            }
            KillBullet(b);
            it = bullets_.erase(it);
            continue;
        }

        b.life += dt;
        if (b.life >= b.ttl) b.alive = false;

        if (b.behavior) {
            b.behavior->Update(b, dt);
        } else {
            b.pos.x += b.vel.x * dt;
            b.pos.y += b.vel.y * dt;
        }

        if (b.pos.x < -100 || b.pos.x > 1380 || b.pos.y < -100 || b.pos.y > 1100) {
            b.alive = false;
            b.onDeathFxName.clear();
        }

        b.visualsTransform.position = b.pos;

        if (std::abs(b.vel.x) > 0.001f || std::abs(b.vel.y) > 0.001f) {

            float angle = std::atan2(b.vel.y, b.vel.x);

            b.visualsTransform.rotation = angle + (3.14159265f / 2.0f);
        }

        ++it;
    }
    fx_.Update(dt);
}

void BulletManager::KillBullet(Bullet& b) {
    if (b.activeEmitter) {
        fx_.DestroyEmitter(b.activeEmitter);
        b.activeEmitter = nullptr;
    }
}

void BulletManager::Draw() {
    

    HIKARI::POST::PostSystem::BeginLayer(bulletChain_, 0.0f, 0.0f, 0.0f, 0.0f);

    for (const auto& b : bullets_) {
        if (!b.alive) continue;

        auto it = styles_.find(b.styleName);
        if (it != styles_.end() && !it->second.textureName.empty()) {
            int handle = HIKARI::TEXTURE::GetDxHandle(it->second.textureName);
            int w = 32, h = 32;
            if (handle >= 0) {
                UINT tw = 0,th = 0;
                HIKARI::DXTEX::DxTextureManager::GetTextureSize(handle, tw, th);
                w = static_cast<int>(tw);
                h = static_cast<int>(th);
            }

            HIKARI::Transform2D t = b.visualsTransform;
            t.pivotPx = { w * 0.5f, h * 0.5f };

            HIKARI::RENDERER::DrawSprite(
                it->second.textureName,
                t,
                (float)w, (float)h,
                HIKARI::RENDERER::CameraMode::Inherit,
                0xFFFFFFFF
            );
        }
    }

    HIKARI::POST::PostSystem::EndLayer();


    HIKARI::POST::PostSystem::SetIntensity(1.0f);

}
