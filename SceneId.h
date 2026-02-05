#pragma once
#include <cstddef>

enum class SceneId
{
    Title,
    Game,

    Login,
    Register,
    Load,
    ControlsGuide,
    Tutorial,
    World,
    MatchSetup,
    OceanShip,
    Result,
    Pause,
    Ranking,
    ReplayList,
    ReplayPause,
    ReplayPlayback,
    SkinSelect,
};

struct SceneIdHash
{
    size_t operator()(SceneId id) const noexcept { return static_cast<size_t>(id); }
};
