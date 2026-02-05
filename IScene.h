#pragma once

enum class SceneId { Title, Game };

struct IScene {
    virtual ~IScene() {}
    virtual void Init() = 0;
    virtual void Update(float dt) = 0;
    virtual void Draw() = 0;
    virtual bool WantsNext(SceneId& outNext) = 0;
};
