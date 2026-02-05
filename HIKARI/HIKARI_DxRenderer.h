#pragma once
#include <cstddef>
#include <cstdint>
#include <wrl.h>
#include <d3d12.h>
#include "HIKARI_Utility.h"
namespace HIKARI {
    namespace DX {

        enum class BlendMode {
            StraightAlpha,
            PremultipliedAlpha,
            Additive,
            Multiply,
            Screen,
            Opaque,
            Count
        };

        struct MeshVertex {
            float px, py; 
            float u, v; 
            float r, g, b, a; 
        };

        class DxRenderer {
        public:
            static void Init();
            static void Finalize();

            static void BeginFrame();
            static void EndFrame();

            static void SetBlendMode(BlendMode mode);

            static void DrawSprite(
                int texHandle,
                float x, float y,
                float w, float h,
                uint32_t color = 0xFFFFFFFF);

            static void DrawSpriteUV(
                int texHandle,
                float x, float y,
                float w, float h,
                float u0, float v0,
                float u1, float v1,
                uint32_t color = 0xFFFFFFFF);

            static void DrawMesh(
                const MeshVertex* verts,
                int vertexCount,
                D3D_PRIMITIVE_TOPOLOGY topology,
                int texHandle,
                uint32_t color = 0xFFFFFFFF
            );

            static void DrawMeshQuad(
                float x0, float y0, float u0, float v0,
                float x1, float y1, float u1, float v1,
                float x2, float y2, float u2, float v2,
                float x3, float y3, float u3, float v3,
                int texHandle,
                uint32_t color = 0xFFFFFFFF);

            static void DrawLine(
                float x0, float y0,
                float x1, float y1,
                uint32_t color = 0xFFFFFFFF);

        private:

            DxRenderer() = default;
            ~DxRenderer() = default;

            DxRenderer(const DxRenderer&) = delete;
            DxRenderer& operator=(const DxRenderer&) = delete;

            static Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSig_;
            static Microsoft::WRL::ComPtr<ID3D12PipelineState> colorPsos_[static_cast<size_t>(BlendMode::Count)];
            static Microsoft::WRL::ComPtr<ID3D12PipelineState> psoLine_;

            static BlendMode currentBlendMode_;
        };

    } // namespace DX
} // namespace HIKARI
