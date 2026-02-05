#pragma once
#include <Novice.h>
#include <string>
#include <vector>  
#include "Matrix3x3.h"
#include "HIKARI_Transform2D.h"
#include "HIKARI_Camera.h"
#include "HIKARI_Texture.h"
namespace HIKARI {
    namespace RENDERER {


        struct SpriteSheetInfo {
            int frameWidth = 0; // 1コマの幅（px）
            int frameHeight = 0; // 1コマの高さ（px）
            int columns = 1; // 横方向のコマ数
        };

        // スプライト用の UV 範囲（正規化 0.0～1.0）
        struct SpriteUV {
            float u0 = 0.0f, v0 = 0.0f; // 左上
            float u1 = 1.0f, v1 = 1.0f; // 右下
        };

        // フレームアニメ + MeshQuad 変形用
        struct SpriteFrameDeform {
            // この UV は「フレーム内」でのローカル UV（0.0～1.0）
            // 例: (0,0)-(1,1) = フレーム全体
            SpriteUV uv{};

            // メッシュの4頂点に対するローカル座標オフセット
            Vector2 offsetLT{ 0.0f, 0.0f };
            Vector2 offsetRT{ 0.0f, 0.0f };
            Vector2 offsetLB{ 0.0f, 0.0f };
            Vector2 offsetRB{ 0.0f, 0.0f };
        };

        // この描画呼び出しがカメラの影響を受けるかどうか
        enum class CameraMode { Inherit, Ignore };

        enum class FillMode { Fill, Wireframe };

        enum class BlendMode {
            StraightAlpha,
            PremultipliedAlpha,
            Additive,
            Multiply,
            Screen,
            Opaque
        };

        // グローバル既定のティントカラー
        void SetDefaultColor(unsigned int rgba);
        unsigned int GetDefaultColor();

        void SetBlendMode(BlendMode mode);

        // オプション：内部で使う 1x1 白テクスチャ（RGBA）のパスを設定
        // 既定値: "./NoviceResources/white1x1.png"
        void SetWhiteTexturePath(const char* pathRGBA1x1);

        // ---- 線分 & ボックス ----
        void DrawLine(
            Vector2 p0, Vector2 p1,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF
        );

        void DrawQuad(
            const HIKARI::Transform2D& t,
            float width, float height,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF
        );

        void DrawBox(
            const HIKARI::Transform2D& t,
            float width, float height,
            FillMode mode = FillMode::Wireframe,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF
        );

        // 画三角形（局部坐标系：以 Transform2D 的原点为局部原点）
        void DrawTriangle(
            const HIKARI::Transform2D& t,
            Vector2 p0, Vector2 p1, Vector2 p2,
            FillMode mode = FillMode::Wireframe,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF);

        // 画椭圆（局部原点为圆心；radiusX/radiusY 为局部半径）
        void DrawEllipse(
            const HIKARI::Transform2D& t,
            float radiusX, float radiusY,
            FillMode mode = FillMode::Wireframe,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF);

        // ---- スプライト（名前指定）----
        void DrawSprite(
            const std::string& textureName,
            const HIKARI::Transform2D& t,
            float width, float height,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF // RGBA
        );

        // UV 指定版
        void DrawSprite(
            const std::string& textureName,
            const HIKARI::Transform2D& t,
            float width, float height,
            const SpriteUV& uv,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF
        );


        // スプライトの一部分を描画
        void DrawSpriteRect(
            const std::string& textureName,
            int srcX, int srcY, int srcW, int srcH,
            const HIKARI::Transform2D& t,
            float dstW, float dstH,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF
        );

        // ---- スプライト（ハンドル指定）----
        void DrawSpriteHandle(
            int textureHandle,
            const HIKARI::Transform2D& t,
            float width, float height,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF
        );

        void DrawSpriteRectHandle(
            int textureHandle,
            int srcX, int srcY, int srcW, int srcH,
            const HIKARI::Transform2D& t,
            float dstW, float dstH,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF
        );

        void DrawSpriteRectHandleVertices(
            int textureHandle,
            int srcX, int srcY, int srcW, int srcH,
            const Vector2& lt, const Vector2& rt,
            const Vector2& lb, const Vector2& rb,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF
        );

        // ==== 簡易デフォーム用のグリッド ====
        struct DeformGrid {
            int cols = 0;    // グリッドの列数（横）
            int rows = 0;    // 行数（縦）

            // 各格点の座標（World Space）
            // 要素数 = cols * rows
            std::vector<Vector2> positions;

            // 各格点に対応する UV（0〜1）
            // 要素数 = cols * rows
            std::vector<Vector2> uvs;
        };

        // DXテクスチャハンドルから描画
        void DrawDeformGridHandle(
            int dxHandle,
            const DeformGrid& grid,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF
        );

        // テクスチャ名から描画（内部で GetDxHandle）
        void DrawDeformGrid(
            const std::string& textureName,
            const DeformGrid& grid,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF
        );



        // ---- SPINE ----


        void DrawMeshQuadHandleUV_Vertices(
            int textureHandle,
            const Vector2& lt, const Vector2& rt,
            const Vector2& lb, const Vector2& rb,
            float u_lt, float v_lt,
            float u_rt, float v_rt,
            float u_lb, float v_lb,
            float u_rb, float v_rb,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF);


        void DrawMeshQuadHandleUV_Local(
            int textureHandle,
            const HIKARI::Transform2D& t,
            const Vector2& ltLocal, const Vector2& rtLocal,
            const Vector2& lbLocal, const Vector2& rbLocal,
            float u_lt, float v_lt,
            float u_rt, float v_rt,
            float u_lb, float v_lb,
            float u_rb, float v_rb,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF);

        // ---- ANIMATION ----
        void DrawSpriteFrame(
            const std::string& textureName,
            const SpriteSheetInfo& sheet,
            int frameIndex,
            const HIKARI::Transform2D& t,
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF);


        // フレーム + MeshQuad 変形版
        void DrawSpriteFrameEx(
            const std::string& textureName,
            const SpriteSheetInfo& sheet,
            int frameIndex,
            const HIKARI::Transform2D& t,
            const SpriteFrameDeform& deform = SpriteFrameDeform{},
            CameraMode cam = CameraMode::Inherit,
            unsigned int rgba = 0xFFFFFFFF);





    } // namespace RENDERER
} // namespace HIKARI
