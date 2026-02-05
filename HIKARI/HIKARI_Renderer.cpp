#include "HIKARI_Renderer.h"
#include <cmath>
#include "HIKARI_DxRenderer.h"
#include "HIKARI_DxTexture.h"
// ===== 内部ツール =====
namespace {

    using namespace HIKARI;
    using namespace HIKARI::RENDERER;

    static unsigned int gDefaultColor = 0xFFFFFFFF; // RGBA

    static int gWhiteTexHandle = -1;
    static const char* kDefaultWhiteTexPath = "./NoviceResources/white1x1.png";
    static std::string gWhiteTexPath = kDefaultWhiteTexPath;

    static inline int EnsureWhiteTexture() {
        if (gWhiteTexHandle < 0) {
            // 用 DX 纹理管理器载入 1x1 白贴图
            gWhiteTexHandle = HIKARI::DXTEX::DxTextureManager::LoadTexture(
                "Renderer_White1x1",
                gWhiteTexPath
            );
        }
        return gWhiteTexHandle;
    }


    static inline Vector2 TransformPoint(const Vector2& p, const Matrix3x3& m) {
        Vector2 o{};
        o.x = p.x * m.m[0][0] + p.y * m.m[1][0] + m.m[2][0];
        o.y = p.x * m.m[0][1] + p.y * m.m[1][1] + m.m[2][1];
        return o;
    }

    static inline Matrix3x3 ComposeWorldThenMaybeView(const Matrix3x3& world, CameraMode cam) {
        if (cam == CameraMode::Ignore) return world;
        const Matrix3x3& view = HIKARI::CAMERA::GetViewMatrix();
        return world * view;
    }

    static void DrawSpriteDxInternal(
        int dxHandle,
        const HIKARI::Transform2D& t,
        float width, float height,
        const HIKARI::RENDERER::SpriteUV& uv,
        HIKARI::RENDERER::CameraMode cam,
        unsigned int rgba)
    {
        if (dxHandle < 0) {
            return;
        }


        Matrix3x3 world = t.ToWorld(width, height);
        Matrix3x3 m = ComposeWorldThenMaybeView(world, cam);

        Vector2 pLT{ 0.0f, 0.0f };
        Vector2 pRT{ width, 0.0f };
        Vector2 pLB{ 0.0f, height };
        Vector2 pRB{ width, height };

        Vector2 lt = TransformPoint(pLT, m);
        Vector2 rt = TransformPoint(pRT, m);
        Vector2 lb = TransformPoint(pLB, m);
        Vector2 rb = TransformPoint(pRB, m);

        unsigned int c = rgba;

        HIKARI::DX::DxRenderer::DrawMeshQuad(
            // 顶点 0
            lt.x, lt.y, uv.u0, uv.v0,
            // 顶点 1
            rt.x, rt.y, uv.u1, uv.v0,
            // 顶点 2
            lb.x, lb.y, uv.u0, uv.v1,
            // 顶点 3
            rb.x, rb.y, uv.u1, uv.v1,
            dxHandle,
            c
        );
    }



} // anonymous

namespace HIKARI {
    namespace RENDERER {


        void SetDefaultColor(unsigned int rgba) { gDefaultColor = rgba; }
        unsigned int GetDefaultColor() { return gDefaultColor; }

        void SetBlendMode(BlendMode mode)
        {
            HIKARI::DX::DxRenderer::SetBlendMode(static_cast<HIKARI::DX::BlendMode>(mode));
        }

        void SetWhiteTexturePath(const char* pathRGBA1x1) {
            if (pathRGBA1x1 && pathRGBA1x1[0] != '\0') {
                gWhiteTexPath = pathRGBA1x1;
                gWhiteTexHandle = -1; // 次回利用時に再ロードさせる
            }
        }

        // ---- 基本的な線描画 ----
        void DrawLine(Vector2 p0, Vector2 p1, CameraMode cam, unsigned int rgba) {
            Matrix3x3 world = Matrix3x3::MakeIdentity();
            Matrix3x3 m = ComposeWorldThenMaybeView(world, cam);

            Vector2 a = TransformPoint(p0, m);
            Vector2 b = TransformPoint(p1, m);

            unsigned int c = (rgba == 0xFFFFFFFF ? gDefaultColor : rgba); 
            HIKARI::DX::DxRenderer::DrawLine(static_cast<int>(a.x), static_cast<int>(a.y),
                static_cast<int>(b.x), static_cast<int>(b.y), c);
        }

        // ---- Quad----
        void DrawQuad(const HIKARI::Transform2D& t,
            float width, float height,
            CameraMode cam,
            unsigned int rgba) {
            DrawBox(t, width, height, FillMode::Fill, cam, rgba);
        }

        // ---- Box ----
        void DrawBox(const HIKARI::Transform2D& t,
            float width, float height,
            FillMode mode,
            CameraMode cam,
            unsigned int rgba) {
            if (mode == FillMode::Fill) {
                const int h = EnsureWhiteTexture();
                if (h >= 0) {
                    DrawSpriteRectHandle(
                        h, 0, 0, 1, 1,
                        t, width, height,
                        cam,
                        rgba // RGBA
                    );
                    return;
                }
                // 白画像が読めなかった場合、ワイヤーフレームへフォールバック
                mode = FillMode::Wireframe;
            }

            // Wireframe：4 本の線で描画
            Matrix3x3 world = t.ToWorld(width, height);
            Matrix3x3 m = ComposeWorldThenMaybeView(world, cam);

            Vector2 pLT{ 0.0f, 0.0f };
            Vector2 pRT{ width, 0.0f };
            Vector2 pLB{ 0.0f, height };
            Vector2 pRB{ width, height };

            Vector2 lt = TransformPoint(pLT, m);
            Vector2 rt = TransformPoint(pRT, m);
            Vector2 lb = TransformPoint(pLB, m);
            Vector2 rb = TransformPoint(pRB, m);

            unsigned int c = (rgba == 0xFFFFFFFF ? gDefaultColor : rgba);

            HIKARI::DX::DxRenderer::DrawLine(lt.x, lt.y, rt.x, rt.y, c);
            HIKARI::DX::DxRenderer::DrawLine(rt.x, rt.y, rb.x, rb.y, c);
            HIKARI::DX::DxRenderer::DrawLine(rb.x, rb.y, lb.x, lb.y, c);
            HIKARI::DX::DxRenderer::DrawLine(lb.x, lb.y, lt.x, lt.y, c);

        }

        void DrawTriangle(
            const HIKARI::Transform2D& t,
            Vector2 p0, Vector2 p1, Vector2 p2,
            FillMode mode, CameraMode cam, unsigned int rgba)
        {
            Matrix3x3 world = t.ToWorld(1.0f, 1.0f);
            Matrix3x3 m = ComposeWorldThenMaybeView(world, cam);

            Vector2 a = TransformPoint(p0, m);
            Vector2 b = TransformPoint(p1, m);
            Vector2 c = TransformPoint(p2, m);

            unsigned int col = (rgba == 0xFFFFFFFF ? gDefaultColor : rgba);

            // === Fill 模式：用 1x1 白贴图 + 退化四边形画一个实心三角形 ===
            if (mode == FillMode::Fill) {
                int whiteHandle = EnsureWhiteTexture();
                if (whiteHandle >= 0) {

                    const float u0 = 0.0f, v0 = 0.0f;
                    const float u1 = 1.0f, v1 = 1.0f;

                    HIKARI::DX::DxRenderer::DrawMeshQuad(
                        // v0 : a
                        a.x, a.y, u0, v0,
                        // v1 : b
                        b.x, b.y, u1, v0,
                        // v2 : c
                        c.x, c.y, u0, v1,
                        // v3 : c (退化)
                        c.x, c.y, u1, v1,
                        whiteHandle,
                        col
                    );
                } else {
                    mode = FillMode::Wireframe;
                }
            }

            // === Wireframe：三个边 ===
            if (mode == FillMode::Wireframe) {
                HIKARI::DX::DxRenderer::DrawLine(a.x, a.y, b.x, b.y, col);
                HIKARI::DX::DxRenderer::DrawLine(b.x, b.y, c.x, c.y, col);
                HIKARI::DX::DxRenderer::DrawLine(c.x, c.y, a.x, a.y, col);
            }
        }



        void DrawEllipse(
            const HIKARI::Transform2D& t,
            float radiusX, float radiusY,
            FillMode mode, CameraMode cam, unsigned int rgba)
        {
            const int kSegments = 64;

            float width = radiusX * 2.0f;
            float height = radiusY * 2.0f;

            Matrix3x3 world = t.ToWorld(width, height);
            Matrix3x3 m = ComposeWorldThenMaybeView(world, cam);

            std::vector<Vector2> pts;
            pts.reserve(kSegments + 1);


            float cx = width * 0.5f;
            float cy = height * 0.5f;

            for (int i = 0; i <= kSegments; ++i) {
                float theta = (float)i / (float)kSegments * 2.0f * 3.14159265f;

                float localX = cx + radiusX * std::cos(theta);
                float localY = cy + radiusY * std::sin(theta);

                Vector2 pLocal{ localX, localY };
                pts.push_back(TransformPoint(pLocal, m));
            }

            unsigned int col = (rgba == 0xFFFFFFFF ? gDefaultColor : rgba);

            // === 线框：用线段连接周围的点 ===
            for (int i = 0; i < kSegments; ++i) {
                const Vector2& a = pts[i];
                const Vector2& b = pts[i + 1];
                HIKARI::DX::DxRenderer::DrawLine(a.x, a.y, b.x, b.y, col);
            }

            // === Fill：用三角扇形填充整个椭圆 ===
            if (mode == FillMode::Fill) {
                int whiteHandle = EnsureWhiteTexture();
                if (whiteHandle < 0) {

                    return;
                }
                Vector2 centerLocal{ cx, cy };
                Vector2 centerWorld = TransformPoint(centerLocal, m);

                const float u0 = 0.0f, v0 = 0.0f;
                const float u1 = 1.0f, v1 = 1.0f;

                for (int i = 0; i < kSegments; ++i) {
                    const Vector2& pA = centerWorld;
                    const Vector2& pB = pts[i];
                    const Vector2& pC = pts[i + 1];


                    HIKARI::DX::DxRenderer::DrawMeshQuad(
                        pA.x, pA.y, u0, v0,
                        pB.x, pB.y, u1, v0,
                        pC.x, pC.y, u0, v1,
                        pC.x, pC.y, u1, v1,
                        whiteHandle,
                        col
                    );
                }
            }
        }





        // ---- スプライト：名前指定 ----
        void DrawSprite(const std::string& textureName,
            const HIKARI::Transform2D& t,
            float width, float height,
            CameraMode cam,
            unsigned int rgba)
        {
            // DX テクスチャハンドルを取得
            int dxHandle = HIKARI::TEXTURE::GetDxHandle(textureName);
            if (dxHandle < 0) {
                return;
            }

            // デフォルト UV（0～1 フル領域）
            SpriteUV uv;
            uv.u0 = 0.0f;
            uv.v0 = 0.0f;
            uv.u1 = 1.0f;
            uv.v1 = 1.0f;
            DrawSpriteDxInternal(dxHandle, t, width, height, uv, cam, rgba);
        }

        // ---- スプライト：名前指定 + UV ----
        void DrawSprite(const std::string& textureName,
            const HIKARI::Transform2D& t,
            float width, float height,
            const SpriteUV& uv,
            CameraMode cam,
            unsigned int rgba)
        {
            int dxHandle = HIKARI::TEXTURE::GetDxHandle(textureName);
            if (dxHandle < 0) {
                return;
            }

            DrawSpriteDxInternal(dxHandle, t, width, height, uv, cam, rgba);
        }

        void DrawSpriteRect(const std::string& textureName,
            int srcX, int srcY, int srcW, int srcH,
            const HIKARI::Transform2D& t,
            float dstW, float dstH,
            CameraMode cam,
            unsigned int rgba)
        {
            int dxHandle = HIKARI::TEXTURE::GetDxHandle(textureName);
            if (dxHandle < 0) {
                return;
            }

            // 取得テクスチャ全体サイズ
            UINT texW = 0, texH = 0;
            HIKARI::DXTEX::DxTextureManager::GetTextureSize(dxHandle, texW, texH);
            if (texW == 0 || texH == 0) {
                return;
            }

            // src のピクセル範囲 → 正規化 UV
            SpriteUV uv{};
            uv.u0 = static_cast<float>(srcX) / static_cast<float>(texW);
            uv.v0 = static_cast<float>(srcY) / static_cast<float>(texH);
            uv.u1 = static_cast<float>(srcX + srcW) / static_cast<float>(texW);
            uv.v1 = static_cast<float>(srcY + srcH) / static_cast<float>(texH);

            DrawSpriteDxInternal(dxHandle, t, dstW, dstH, uv, cam, rgba);
        }

        // ---- スプライト：ハンドル指定 ----
        void DrawSpriteHandle(int textureHandle,
            const HIKARI::Transform2D& t,
            float width, float height,
            CameraMode cam,
            unsigned int rgba) {
            if (textureHandle < 0) return;

            Matrix3x3 world = t.ToWorld(width, height);
            Matrix3x3 m = ComposeWorldThenMaybeView(world, cam);

            Vector2 pLT{ 0.0f, 0.0f };
            Vector2 pRT{ width, 0.0f };
            Vector2 pLB{ 0.0f, height };
            Vector2 pRB{ width, height };

            Vector2 lt = TransformPoint(pLT, m);
            Vector2 rt = TransformPoint(pRT, m);
            Vector2 lb = TransformPoint(pLB, m);
            Vector2 rb = TransformPoint(pRB, m);

            unsigned int c = (rgba == 0xFFFFFFFF ? gDefaultColor : rgba);

            int srcW = static_cast<int>(width);
            int srcH = static_cast<int>(height);
            SpriteUV uv{};
            DrawSpriteDxInternal(textureHandle, t, width, height, uv, cam, rgba);
        }

        // ---- スプライト：ハンドル指定 ----
        void DrawSpriteRectHandle(int textureHandle,
            int srcX, int srcY, int srcW, int srcH,
            const HIKARI::Transform2D& t,
            float dstW, float dstH,
            CameraMode cam,
            unsigned int rgba) {
            if (textureHandle < 0) {
                return;
            }

            UINT texW = 0;
            UINT texH = 0;
            HIKARI::DXTEX::DxTextureManager::GetTextureSize(textureHandle, texW, texH);
            if (texW == 0 || texH == 0) {
                return;
            }

            float u0 = static_cast<float>(srcX) / static_cast<float>(texW);
            float v0 = static_cast<float>(srcY) / static_cast<float>(texH);
            float u1 = static_cast<float>(srcX + srcW) / static_cast<float>(texW);
            float v1 = static_cast<float>(srcY + srcH) / static_cast<float>(texH);

            Matrix3x3 world = t.ToWorld(dstW, dstH);
            Matrix3x3 m = ComposeWorldThenMaybeView(world, cam);

            Vector2 pLT{ 0.0f, 0.0f };
            Vector2 pRT{ dstW, 0.0f };
            Vector2 pLB{ 0.0f, dstH };
            Vector2 pRB{ dstW, dstH };

            Vector2 lt = TransformPoint(pLT, m);
            Vector2 rt = TransformPoint(pRT, m);
            Vector2 lb = TransformPoint(pLB, m);
            Vector2 rb = TransformPoint(pRB, m);

            unsigned int c = (rgba == 0xFFFFFFFF ? gDefaultColor : rgba); // RGBA

            HIKARI::DX::DxRenderer::DrawMeshQuad(
                lt.x, lt.y, u0, v0,
                rt.x, rt.y, u1, v0,
                lb.x, lb.y, u0, v1,
                rb.x, rb.y, u1, v1,
                textureHandle,
                c
            );
        }


     // =========================================================
     // DeformGrid 描画（DXハンドル版）
     // =========================================================
        void DrawDeformGridHandle(
            int dxHandle,
            const DeformGrid& grid,
            CameraMode cam,
            unsigned int rgba)
        {
            if (dxHandle < 0) {
                return;
            }
            if (grid.cols < 2 || grid.rows < 2) {
                return;
            }

            int vertexCount = grid.cols * grid.rows;
            if ((int)grid.positions.size() != vertexCount) {
                return;
            }

            bool hasUV = ((int)grid.uvs.size() == vertexCount);

            unsigned int c = (rgba == 0xFFFFFFFF ? gDefaultColor : rgba);

            Matrix3x3 view = ComposeWorldThenMaybeView(Matrix3x3::MakeIdentity(), cam);
            bool applyView = (cam != CameraMode::Ignore);
            std::vector<Vector2> transformed;
            const std::vector<Vector2>* positionsPtr = &grid.positions;

            if (applyView) {
                transformed.resize(vertexCount);
                for (int i = 0; i < vertexCount; ++i) {
                    transformed[i] = TransformPoint(grid.positions[i], view);
                }
                positionsPtr = &transformed;
            }

            // グリッドの各小四角形を TRIANGLESTRIP で描画
            for (int y = 0; y < grid.rows - 1; ++y) {
                for (int x = 0; x < grid.cols - 1; ++x) {
                    int i0 = y * grid.cols + x;
                    int i1 = y * grid.cols + (x + 1);
                    int i2 = (y + 1) * grid.cols + x;
                    int i3 = (y + 1) * grid.cols + (x + 1);

                    HIKARI::DX::MeshVertex verts[4]{};

                    // 位置
                    verts[0].px = (*positionsPtr)[i0].x;
                    verts[0].py = (*positionsPtr)[i0].y;
                    verts[1].px = (*positionsPtr)[i1].x;
                    verts[1].py = (*positionsPtr)[i1].y;
                    verts[2].px = (*positionsPtr)[i2].x;
                    verts[2].py = (*positionsPtr)[i2].y;
                    verts[3].px = (*positionsPtr)[i3].x;
                    verts[3].py = (*positionsPtr)[i3].y;

                    // UV（もし与えられていなければ、0〜1の格子で自動生成）
                    if (hasUV) {
                        verts[0].u = grid.uvs[i0].x;
                        verts[0].v = grid.uvs[i0].y;
                        verts[1].u = grid.uvs[i1].x;
                        verts[1].v = grid.uvs[i1].y;
                        verts[2].u = grid.uvs[i2].x;
                        verts[2].v = grid.uvs[i2].y;
                        verts[3].u = grid.uvs[i3].x;
                        verts[3].v = grid.uvs[i3].y;
                    } else {
                        // x / (cols-1), y / (rows-1) のシンプルな正規化 UV
                        float u0 = (float)x / (float)(grid.cols - 1);
                        float v0 = (float)y / (float)(grid.rows - 1);
                        float u1 = (float)(x + 1) / (float)(grid.cols - 1);
                        float v1 = (float)(y + 1) / (float)(grid.rows - 1);

                        verts[0].u = u0; verts[0].v = v0;
                        verts[1].u = u1; verts[1].v = v0;
                        verts[2].u = u0; verts[2].v = v1;
                        verts[3].u = u1; verts[3].v = v1;
                    }

                    // 頂点色は DrawMesh 側で rgba に統一されるのでここでは 0 で OK
                    // 描画
                    HIKARI::DX::DxRenderer::DrawMesh(
                        verts,
                        4,
                        D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,
                        dxHandle,
                        c
                    );
                }
            }
        }

        // =========================================================
        // DeformGrid 描画（テクスチャ名版）
        // =========================================================
        void DrawDeformGrid(
            const std::string& textureName,
            const DeformGrid& grid,
            CameraMode cam,
            unsigned int rgba)
        {
            int dxHandle = HIKARI::TEXTURE::GetDxHandle(textureName);
            if (dxHandle < 0) {
                return;
            }
            DrawDeformGridHandle(dxHandle, grid, cam, rgba);
        }



        // ---- SPINE ----

        void DrawSpriteRectHandleVertices(
            int textureHandle,
            int srcX, int srcY, int srcW, int srcH,
            const Vector2& ltIn, const Vector2& rtIn,
            const Vector2& lbIn, const Vector2& rbIn,
            CameraMode cam,
            unsigned int rgba
        ) {
            if (textureHandle < 0) {
                return;
            }

            UINT texW = 0;
            UINT texH = 0;
            HIKARI::DXTEX::DxTextureManager::GetTextureSize(textureHandle, texW, texH);
            if (texW == 0 || texH == 0) {
                return;
            }

            float u0 = static_cast<float>(srcX) / static_cast<float>(texW);
            float v0 = static_cast<float>(srcY) / static_cast<float>(texH);
            float u1 = static_cast<float>(srcX + srcW) / static_cast<float>(texW);
            float v1 = static_cast<float>(srcY + srcH) / static_cast<float>(texH);

            Matrix3x3 world = Matrix3x3::MakeIdentity();
            Matrix3x3 m = ComposeWorldThenMaybeView(world, cam);

            Vector2 lt = TransformPoint(ltIn, m);
            Vector2 rt = TransformPoint(rtIn, m);
            Vector2 lb = TransformPoint(lbIn, m);
            Vector2 rb = TransformPoint(rbIn, m);

            unsigned int c = (rgba == 0xFFFFFFFF ? GetDefaultColor() : rgba);

            HIKARI::DX::DxRenderer::DrawMeshQuad(
                lt.x, lt.y, u0, v0,
                rt.x, rt.y, u1, v0,
                lb.x, lb.y, u0, v1,
                rb.x, rb.y, u1, v1,
                textureHandle,
                c
            );
        }



        void DrawMeshQuadHandleUV_Vertices(
            int textureHandle,
            const Vector2& lt, const Vector2& rt,
            const Vector2& lb, const Vector2& rb,
            float u_lt, float v_lt,
            float u_rt, float v_rt,
            float u_lb, float v_lb,
            float u_rb, float v_rb,
            CameraMode cam,
            unsigned int rgba)
        {
            if (textureHandle < 0) { return; }

            Matrix3x3 world = Matrix3x3::MakeIdentity();
            Matrix3x3 m = ComposeWorldThenMaybeView(world, cam);

            Vector2 lt2 = TransformPoint(lt, m);
            Vector2 rt2 = TransformPoint(rt, m);
            Vector2 lb2 = TransformPoint(lb, m);
            Vector2 rb2 = TransformPoint(rb, m);

            unsigned int c = rgba;

            HIKARI::DX::DxRenderer::DrawMeshQuad(
                lt2.x, lt2.y, u_lt, v_lt,
                rt2.x, rt2.y, u_rt, v_rt,
                lb2.x, lb2.y, u_lb, v_lb,
                rb2.x, rb2.y, u_rb, v_rb,
                textureHandle,
                c
            );
        }

        void DrawMeshQuadHandleUV_Local(
            int textureHandle,
            const HIKARI::Transform2D& t,
            const Vector2& ltLocal, const Vector2& rtLocal,
            const Vector2& lbLocal, const Vector2& rbLocal,
            float u_lt, float v_lt,
            float u_rt, float v_rt,
            float u_lb, float v_lb,
            float u_rb, float v_rb,
            CameraMode cam,
            unsigned int rgba)
        {
            if (textureHandle < 0) { return; }

            Matrix3x3 world = t.ToWorld(1.0f, 1.0f);
            Matrix3x3 m = ComposeWorldThenMaybeView(world, cam);

            Vector2 lt2 = TransformPoint(ltLocal, m);
            Vector2 rt2 = TransformPoint(rtLocal, m);
            Vector2 lb2 = TransformPoint(lbLocal, m);
            Vector2 rb2 = TransformPoint(rbLocal, m);

            unsigned int c = (rgba == 0xFFFFFFFF ? GetDefaultColor() : rgba);

            HIKARI::DX::DxRenderer::DrawMeshQuad(
                lt2.x, lt2.y, u_lt, v_lt,
                rt2.x, rt2.y, u_rt, v_rt,
                lb2.x, lb2.y, u_lb, v_lb,
                rb2.x, rb2.y, u_rb, v_rb,
                textureHandle,
                c
            );
        }

        // ---- ANIMATION ----
        void DrawSpriteFrame(
            const std::string& textureName,
            const SpriteSheetInfo& sheet,
            int frameIndex,
            const HIKARI::Transform2D& t,
            CameraMode cam,
            unsigned int rgba)
        {
            if (sheet.frameWidth <= 0 || sheet.frameHeight <= 0 || sheet.columns <= 0) {
                return;
            }

            if (frameIndex < 0) {
                return;
            }

            int col = frameIndex % sheet.columns;
            int row = frameIndex / sheet.columns;

            int srcX = col * sheet.frameWidth;
            int srcY = row * sheet.frameHeight;
            int srcW = sheet.frameWidth;
            int srcH = sheet.frameHeight;

            DrawSpriteRect(
                textureName,
                srcX, srcY, srcW, srcH,
                t,
                static_cast<float>(srcW),
                static_cast<float>(srcH),
                cam,
                rgba
            );
        }

        // フレーム + MeshQuad 変形版
        void DrawSpriteFrameEx(
            const std::string& textureName,
            const SpriteSheetInfo& sheet,
            int frameIndex,
            const HIKARI::Transform2D& t,
            const SpriteFrameDeform& deform,
            CameraMode cam,
            unsigned int rgba)
        {
            if (sheet.frameWidth <= 0 || sheet.frameHeight <= 0 || sheet.columns <= 0) {
                return;
            }
            if (frameIndex < 0) {
                return;
            }

            // 1) まずは通常のフレーム計算（どのコマを使うか）
            int col = frameIndex % sheet.columns;
            int row = frameIndex / sheet.columns;

            int srcX = col * sheet.frameWidth;
            int srcY = row * sheet.frameHeight;
            int srcW = sheet.frameWidth;
            int srcH = sheet.frameHeight;

            // 2) DX テクスチャハンドル＆サイズ取得
            int dxHandle = HIKARI::TEXTURE::GetDxHandle(textureName);
            if (dxHandle < 0) {
                return;
            }

            UINT texW = 0, texH = 0;
            HIKARI::DXTEX::DxTextureManager::GetTextureSize(dxHandle, texW, texH);
            if (texW == 0 || texH == 0) {
                return;
            }

            // 3) このフレームがテクスチャ全体のどの範囲か（ベース UV）
            float baseU0 = static_cast<float>(srcX) / static_cast<float>(texW);
            float baseV0 = static_cast<float>(srcY) / static_cast<float>(texH);
            float baseU1 = static_cast<float>(srcX + srcW) / static_cast<float>(texW);
            float baseV1 = static_cast<float>(srcY + srcH) / static_cast<float>(texH);

            // 4) deform.uv は「フレーム内」でのローカル UV（0～1）として解釈
            //    例: uv={0,0,1,1} → フレーム全体
            auto lerp = [](float a, float b, float t) {
                return a + (b - a) * t;
                };

            float u0 = lerp(baseU0, baseU1, deform.uv.u0);
            float v0 = lerp(baseV0, baseV1, deform.uv.v0);
            float u1 = lerp(baseU0, baseU1, deform.uv.u1);
            float v1 = lerp(baseV0, baseV1, deform.uv.v1);

            // 5) ローカル頂点（フレームサイズを基準にした矩形）＋オフセット
            float w = static_cast<float>(srcW);
            float h = static_cast<float>(srcH);

            Vector2 ltLocal{ 0.0f, 0.0f };
            Vector2 rtLocal{ w,    0.0f };
            Vector2 lbLocal{ 0.0f, h };
            Vector2 rbLocal{ w,    h };

            ltLocal.x += deform.offsetLT.x;
            ltLocal.y += deform.offsetLT.y;
            rtLocal.x += deform.offsetRT.x;
            rtLocal.y += deform.offsetRT.y;
            lbLocal.x += deform.offsetLB.x;
            lbLocal.y += deform.offsetLB.y;
            rbLocal.x += deform.offsetRB.x;
            rbLocal.y += deform.offsetRB.y;

            // 6) Transform2D + Camera を使って MeshQuad 描画
            unsigned int c = (rgba == 0xFFFFFFFF ? GetDefaultColor() : rgba);

            DrawMeshQuadHandleUV_Local(
                dxHandle,
                t,
                ltLocal, rtLocal, lbLocal, rbLocal,
                u0, v0,   // 左上
                u1, v0,   // 右上
                u0, v1,   // 左下
                u1, v1,   // 右下
                cam,
                c
            );
        }


    } // namespace RENDERER
} // namespace HIKARI
