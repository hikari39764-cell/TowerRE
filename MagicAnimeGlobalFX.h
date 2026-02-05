#pragma once
#include <DirectXMath.h>
#include "HIKARI_PostEffect.h"

class MagicAnimeGlobalFX {
public:
    void InitOnce();
    void SetEnabled(bool e) { enabled_ = e; }
    void UpdateParams(); 

private:
    bool inited_ = false;
    bool enabled_ = true;
    HIKARI::POST::PostEffect fx_;
};
