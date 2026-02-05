#include "HIKARI_PostSystem.h"
#include "HIKARI_PostEffect.h"
#include <base/DirectXCommon.h>
#include <cassert>

namespace HIKARI {
    namespace POST {

        bool PostSystem::initialized_ = false;
        RenderTarget2D PostSystem::sceneRT_{};
        QuadDrawer PostSystem::quad_{};
        PostChain PostSystem::globalChain_{}; 
        CommonParams PostSystem::commonParams_{};
        float PostSystem::elapsedTime_ = 0.0f;

        // 栈
        std::stack<PostSystem::LayerInfo> PostSystem::rtStack_{};

        void PostSystem::Initialize()
        {
            if (initialized_) return;
            quad_.Init();
            initialized_ = true;
        }

        void PostSystem::Shutdown()
        {
            if (!initialized_) return;
            globalChain_.Finalize();
            sceneRT_.Finalize();
            quad_.Finalize();

            // 清空栈
            while (!rtStack_.empty()) rtStack_.pop();

            initialized_ = false;
        }

        void PostSystem::UpdateCommonParams(float deltaTime)
        {
            if (!initialized_) {
                Initialize();
            }

            auto* dx = KamataEngine::DirectXCommon::GetInstance();
            commonParams_.resolutionX = static_cast<float>(dx->GetBackBufferWidth());
            commonParams_.resolutionY = static_cast<float>(dx->GetBackBufferHeight());

            commonParams_.deltaTime = deltaTime;
            elapsedTime_ += deltaTime;
            commonParams_.time = elapsedTime_;
        }

        void PostSystem::SetIntensity(float intensity)
        {
            commonParams_.intensity = intensity;
        }

        void PostSystem::SetCombo(float combo)
        {
            commonParams_.combo = combo;
        }

        void PostSystem::ClearEffects()
        {
            globalChain_.Clear();
        }

        void PostSystem::AddEffect(PostEffect* effect)
        {
            globalChain_.Add(effect);
        }

        void PostSystem::EnsureSceneRTSize()
        {
            auto* dx = KamataEngine::DirectXCommon::GetInstance();
            int w = dx->GetBackBufferWidth();
            int h = dx->GetBackBufferHeight();
            if (w <= 0 || h <= 0) return;

            if (!sceneRT_.GetResource() || sceneRT_.GetWidth() != w || sceneRT_.GetHeight() != h) {
                sceneRT_.Init(w, h);
            }
        }

        void PostSystem::BeginSceneCapture()
        {
            if (!initialized_) Initialize();

            EnsureSceneRTSize();
            UpdateCommonParams(0.0f); 

            while (!rtStack_.empty()) rtStack_.pop();


            rtStack_.push({ &sceneRT_, nullptr });


            sceneRT_.BeginCapture(0.0f, 0.0f, 0.0f, 1.0f);
        }

        void PostSystem::EndSceneCaptureAndPresent()
        {
            if (!initialized_) return;


            if (rtStack_.empty()) return;


            RenderTarget2D* currentRT = rtStack_.top().rt;
            currentRT->EndCapture();
            rtStack_.pop(); 


            RenderTarget2D* finalRT = currentRT;
            if (globalChain_.HasAny()) {
                finalRT = globalChain_.Execute(*currentRT, quad_, commonParams_);
            }

            auto* dx = KamataEngine::DirectXCommon::GetInstance();
            dx->SetRenderTargets(true); 

            quad_.DrawFullscreen(finalRT->GetSrvHeap(), finalRT->GetSrvGpu());
        }


        void PostSystem::BeginLayer(PostChain& chain, float r, float g, float b, float a)
        {
            if (!initialized_) Initialize();

            if (rtStack_.empty()) {

                BeginSceneCapture();
            }


            RenderTarget2D* prevRT = rtStack_.top().rt;
            int w = prevRT->GetWidth();
            int h = prevRT->GetHeight();

            chain.PrepareBuffers(w, h);
            RenderTarget2D* layerRT = chain.GetPing();


            prevRT->EndCapture();


            layerRT->BeginCapture(r, g, b, a);

            rtStack_.push({ layerRT, &chain });
        }

        void PostSystem::EndLayer()
        {
            if (rtStack_.size() <= 1) {

                return;
            }


            LayerInfo currentLayer = rtStack_.top();
            rtStack_.pop(); 

            currentLayer.rt->EndCapture();

            RenderTarget2D* processedRT = currentLayer.rt;
            if (currentLayer.chain && currentLayer.chain->HasAny()) {
                processedRT = currentLayer.chain->Execute(*currentLayer.rt, quad_, commonParams_);
            }

            LayerInfo prevLayer = rtStack_.top();

            prevLayer.rt->Rebind();

            quad_.DrawBlended(processedRT->GetSrvHeap(), processedRT->GetSrvGpu());
        }

    } // POST
} // HIKARI
        