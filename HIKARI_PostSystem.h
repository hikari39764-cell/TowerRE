#pragma once
#include <vector>
#include <stack> 
#include "HIKARI_RenderTarget2D.h"
#include "HIKARI_PostQuadDrawer.h"
#include "HIKARI_PostCommon.h"
#include "HIKARI_PostChain.h"

namespace HIKARI {
    namespace POST {

        class PostEffect;

        class PostSystem
        {
        public:
            static void Initialize();
            static void Shutdown();

            static void UpdateCommonParams(float deltaTime);
            static void SetIntensity(float intensity);
            static void SetCombo(float combo);

            static void ClearEffects();

            static void AddEffect(PostEffect* effect);


            static void BeginSceneCapture();

            static void EndSceneCaptureAndPresent();



            static void BeginLayer(PostChain& chain, float r = 0, float g = 0, float b = 0, float a = 0);

            static void EndLayer();

        private:
            static void EnsureSceneRTSize();

        private:
            static bool initialized_;


            static RenderTarget2D sceneRT_;

            static PostChain globalChain_;

            static QuadDrawer quad_;
            static CommonParams commonParams_;
            static float elapsedTime_;


            struct LayerInfo {
                RenderTarget2D* rt;  
                PostChain* chain; 
            };
            static std::stack<LayerInfo> rtStack_;
        };

    } // POST
} // HIKARI