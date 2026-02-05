#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "HIKARI_D3DBlobCompat.h"

namespace HIKARI {
    namespace POST {

        class QuadDrawer
        {
        public:
            QuadDrawer() = default;
            ~QuadDrawer() { Finalize(); }

            bool Init();
            void Finalize();

            // Copy 用：输入RTをそのまま backbuffer に貼る
            void DrawFullscreen(ID3D12DescriptorHeap* srvHeap, D3D12_GPU_DESCRIPTOR_HANDLE srvGpu);

            // PostChain / PostEffect 用
            void SetInputTexture(ID3D12DescriptorHeap* srvHeap, D3D12_GPU_DESCRIPTOR_HANDLE srvGpu);
            void SetPixelShader(ID3DBlob* psBlob);
            void SetConstantBuffer(D3D12_GPU_VIRTUAL_ADDRESS cbv0);
            void DrawFullscreen();
            void DrawBlended(ID3D12DescriptorHeap* srvHeap, D3D12_GPU_DESCRIPTOR_HANDLE srvGpu);
        private:
            bool CreateRootSignature();
            bool CreatePipeline(ID3DBlob* psBlob, Microsoft::WRL::ComPtr<ID3D12PipelineState>& outPso);
            bool CreateBlendPipeline();
        private:
            bool initialized_ = false;

            Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig_;

            // fullscreen shaders（内蔵）
            Microsoft::WRL::ComPtr<ID3DBlob> vsBlob_;
            Microsoft::WRL::ComPtr<ID3DBlob> psCopyBlob_;

            // PSO
            Microsoft::WRL::ComPtr<ID3D12PipelineState> psoCopy_;
            Microsoft::WRL::ComPtr<ID3D12PipelineState> psoPost_;
             Microsoft::WRL::ComPtr<ID3D12PipelineState> psoBlend_; 
            ID3DBlob* currentPostPS_ = nullptr;

            // current input
            ID3D12DescriptorHeap* currentSrvHeap_ = nullptr;
            D3D12_GPU_DESCRIPTOR_HANDLE currentSrvGpu_{};

            // b0 (CommonParams) from PostEffect
            D3D12_GPU_VIRTUAL_ADDRESS currentCBV0_ = 0;
        };

    } // POST
} // HIKARI
