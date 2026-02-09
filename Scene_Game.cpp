#define NOMINMAX

#include <algorithm>
#include <cmath>

#include "Scene_Game.h"

#include "HIKARI.h"
#include "FadeTransition.h"
#include "ScrollBackground.h"
#include "SceneManager.h"
static bool CheckCollision(const Vector2& pos1, float r1, const RectF& rect2)
{
    float closestX = (std::max<float>)(rect2.x, (std::min<float>)(pos1.x, rect2.x + rect2.width));
    float closestY = (std::max<float>)(rect2.y, (std::min<float>)(pos1.y, rect2.y + rect2.height));

    float dx = pos1.x - closestX;
    float dy = pos1.y - closestY;

    return (dx * dx + dy * dy) < (r1 * r1);
}

Scene_Game::Scene_Game(ScrollBackground& bg)
    : bg_(bg)
{
}

void Scene_Game::OnCreate()
{
}

void Scene_Game::OnEnter()
{
    hud_.Init();
    gAura.InitOnce();
    bullets_.ClearAll();
    bullets_.Init();
    requestedResult_ = false;

    boss_.Init(&player_, &bullets_);
    boss_.SetPhaseChangeCallback([this](GlobalPhase phase) {
        if (phase == GlobalPhase::Ice) {
            bg_.BeginTransition(ScrollBackground::Stage::Ice);
        } else if (phase == GlobalPhase::Fire) {
            bg_.BeginTransition(ScrollBackground::Stage::Fire);
        }
#if 0
        else if (phase == GlobalPhase::Final) {
            bg_.SetStage(ScrollBackground::Stage::Final);
        }
#endif
    });

    player_.Init();
}

void Scene_Game::OnExit()
{
}

void Scene_Game::Update(float dt)
{
    bg_.Update(dt);

    player_.Update(dt, bullets_);

    if (player_.isUseMana[0]) gAura.TriggerCostFx(player_.Pos(), player_.Radius(), player_.Mana() + 1, AuraFxType::UpgradeShot);
    if (player_.isUseMana[1]) gAura.TriggerCostFx(player_.Pos(), player_.Radius(), player_.Mana() + 1, AuraFxType::UpgradeSpeed);
    if (player_.isUseMana[2]) gAura.TriggerCostFx(player_.Pos(), player_.Radius(), player_.Mana() + 1, AuraFxType::Heal);
    if (player_.isUseMana[3]) gAura.TriggerCostFx(player_.Pos(), player_.Radius(), player_.Mana() + 1, AuraFxType::Dash);
    gAura.UpdateParams(player_.Pos(), player_.Radius(), player_.Mana(), dt);

    boss_.Update(dt);
    bullets_.Update(dt);

    hud_.Update(dt, player_, boss_);

    RectF bossRect = boss_.GetAABB();
    Vector2 playerPos = player_.Pos();
    float playerRadius = player_.Radius() * 0.5f;

    for (auto& b : bullets_.GetBullets()) {
        if (!b.alive) {
            continue;
        }

        if (!b.isEnemy) {
            if (b.pos.x > bossRect.x && b.pos.x < bossRect.x + bossRect.width &&
                b.pos.y > bossRect.y && b.pos.y < bossRect.y + bossRect.height)
            {
                boss_.TakeDamage(10.0f);
                b.alive = false;
            }
        } else {
            float dx = (b.pos.x - playerPos.x);
            float dy = (b.pos.y - playerPos.y);
            float distSq = dx * dx + dy * dy;
            float hitR = playerRadius + b.radius;

            if (distSq < hitR * hitR) {
                player_.OnHit(nullptr, 0.0f);
                b.alive = false;
            }
        }
    }

    if (CheckCollision(playerPos, playerRadius, bossRect)) {
        Vector2 bossPos = boss_.GetPos();
        player_.OnHit(&bossPos, 800.0f);
    }

    if (!requestedResult_ && boss_.IsDead()) {
        requestedResult_ = true;
        bullets_.ClearAll();
        if (mgr_ != nullptr) {
            mgr_->RequestChange(SceneId::Result, std::make_unique<FadeTransition>(0.25f, 0.25f));
        }
    }
}

void Scene_Game::Draw()
{
    bg_.Draw();
    bullets_.Draw();
    boss_.Draw();
    bullets_.GetFxSystem().Draw();
    gAura.DrawLayer();
    player_.Draw();
    hud_.Draw(player_, boss_);
}
