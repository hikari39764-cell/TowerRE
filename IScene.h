#pragma once

#include "SceneId.h"

class SceneManager;

class IScene
{
public:
    virtual ~IScene() = default;

    virtual void OnCreate() = 0;
    virtual void OnEnter() = 0;
    virtual void OnExit() = 0;

    virtual void Update(float dt) = 0;
    virtual void Draw() = 0;

    void SetSceneManager(SceneManager* mgr) { mgr_ = mgr; }

protected:
    SceneManager* mgr_ = nullptr;
};
