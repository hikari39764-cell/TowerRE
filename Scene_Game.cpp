#define NOMINMAX 
#include <algorithm> 
#include <cmath> 
#include "Scene_Game.h"
#include "HIKARI.h"
#include "ScrollBackground.h"


bool CheckCollision(const Vector2& pos1, float r1, const RectF& rect2) {

    float closestX = (std::max<float>)(rect2.x, (std::min<float>)(pos1.x, rect2.x + rect2.width));
    float closestY = (std::max<float>)(rect2.y, (std::min<float>)(pos1.y, rect2.y + rect2.height));

    float distanceX = pos1.x - closestX;
    float distanceY = pos1.y - closestY;

    return (distanceX * distanceX + distanceY * distanceY) < (r1 * r1);
}
Scene_Game::Scene_Game(ScrollBackground& bg)
    : bg_(bg)
{
}

void Scene_Game::Init()
{
    hud_.Init();
    gAura.InitOnce();
    bullets_.ClearAll();
    bullets_.Init();
    boss_.Init(&player_, &bullets_);
    boss_.SetPhaseChangeCallback([this](GlobalPhase phase) {
        if (phase == GlobalPhase::Ice) {
            bg_.BeginTransition(ScrollBackground::Stage::Ice);
        } else if (phase == GlobalPhase::Fire) {
            bg_.BeginTransition(ScrollBackground::Stage::Fire);
        } else if (phase == GlobalPhase::Final) {
            // Final 阶段可能直接切，或者你做个 Final 转场
            bg_.SetStage(ScrollBackground::Stage::Final);
        }
        });
    player_.Init();
}

void Scene_Game::Update(float dt)
{
    bg_.Update(dt);

    // 更新玩家
    player_.Update(dt, bullets_);

    // 触发光环特效
    if (player_.isUseMana[0]) gAura.TriggerCostFx(player_.Pos(), player_.Radius(), player_.Mana() + 1, AuraFxType::UpgradeShot);
    if (player_.isUseMana[1]) gAura.TriggerCostFx(player_.Pos(), player_.Radius(), player_.Mana() + 1, AuraFxType::UpgradeSpeed);
    if (player_.isUseMana[2]) gAura.TriggerCostFx(player_.Pos(), player_.Radius(), player_.Mana() + 1, AuraFxType::Heal);
    if (player_.isUseMana[3]) gAura.TriggerCostFx(player_.Pos(), player_.Radius(), player_.Mana() + 1, AuraFxType::Dash);
    gAura.UpdateParams(player_.Pos(), player_.Radius(), player_.Mana(), dt);

    // 更新 Boss
    boss_.Update(dt);

    // 更新子弹
    bullets_.Update(dt);
    hud_.Update(dt, player_, boss_);
    // 碰撞检测逻辑
    RectF bossRect = boss_.GetAABB();
    Vector2 playerPos = player_.Pos();
    float playerRadius = player_.Radius() * 0.5f;

    // 1. 遍历所有子弹
    for (auto& b : bullets_.GetBullets()) {
        if (!b.alive) continue;

        // 情况 A: 玩家子弹打 Boss
        if (!b.isEnemy) {
            if (b.pos.x > bossRect.x && b.pos.x < bossRect.x + bossRect.width &&
                b.pos.y > bossRect.y && b.pos.y < bossRect.y + bossRect.height)
            {

                boss_.TakeDamage(10.0f);
                b.alive = false;

                // (可选) 这里可以补充一个击中特效，比如:
                // bullets_.GetFxSystem().PlayOneShot("fx_hit_spark", b.pos, 1);
            }
        }

        else {
            float distSq = (b.pos.x - playerPos.x) * (b.pos.x - playerPos.x) +
                (b.pos.y - playerPos.y) * (b.pos.y - playerPos.y);
            float hitR = playerRadius + b.radius;

            if (distSq < hitR * hitR) {
                // ★ 修改点 1：子弹击中玩家
                // 需求：不击飞，只晃动
                // 传 nullptr 代表没有击飞源，力为 0.0f
                player_.OnHit(nullptr, 0.0f);
                b.alive = false;
            }
        }
    }

    // 2. 情况 C: Boss 本体撞玩家
    if (CheckCollision(playerPos, playerRadius, bossRect)) {
        // ★ 修改点 2：Boss 本体撞击玩家
        // 需求：被击飞一段

        // 获取 Boss 的位置作为“攻击者位置”
        Vector2 bossPos = boss_.GetPos();

        // 传入 Boss 位置，并给予一个击飞力度 (例如 800.0f，你可以根据手感调整)
        player_.OnHit(&bossPos, 800.0f);
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
bool Scene_Game::WantsNext(SceneId& outNext)
{
    (void)outNext;
    return false;
}
