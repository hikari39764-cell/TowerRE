#pragma once

#include <memory>
#include <unordered_map>

#include "SceneId.h"
#include "ITransition.h"

class IScene;
class ITransition;

class SceneManager
{
public:
    SceneManager() = default;
    ~SceneManager();

    void Register(SceneId id, std::unique_ptr<IScene> scene);
    void Boot(SceneId first);
    void RequestChange(SceneId next, std::unique_ptr<ITransition> transition = nullptr);
    void Update(float dt);
    void Draw();

private:
    void SwapTo(SceneId id);

private:
    std::unordered_map<SceneId, std::unique_ptr<IScene>, SceneIdHash> scenes_;
    std::unordered_map<SceneId, bool, SceneIdHash> created_;

    IScene* current_ = nullptr;
    SceneId currentId_{};

    SceneId pendingId_{};
    bool hasPending_ = false;

    std::unique_ptr<ITransition> transition_;
};
