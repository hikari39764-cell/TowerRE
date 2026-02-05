#include "HIKARI_Camera.h"
#include "HIKARI_Input.h"
#include <cmath>
#include <string>

namespace HIKARI {
    namespace CAMERA {

        // ====== 内部状態 ======
        static State     gState{};
        static Vector2   gScreenCenter{ 0.0f, 0.0f };
        static int       gScreenW = 1280, gScreenH = 720;
        static Matrix3x3 gView = Matrix3x3::MakeIdentity();

        static bool      gDebugControlEnabled = true;
        static std::string gDebugLayerName = "DebugCamera";
        struct DebugActionsConfig {
            std::string dragButton = "CameraDrag";
            std::string dragAxisX = "CameraDragX";
            std::string dragAxisY = "CameraDragY";
            std::string zoomAxis = "CameraZoom";
        } gDebugActions;
        static float     gDebugZoomStep = 0.02f;
        static float     gDebugZoomMin = 0.05f;
        static float     gDebugZoomMax = 12.0f;

        // ====== シェイク ======
        static float   gShakeTime = 0.0f;   // 経過時間
        static float   gShakeDur = 0.0f;    // 総継続時間
        // 振幅
        static float   gAmpX = 0.0f, gAmpY = 0.0f, gAmpRot = 0.0f;
        // 周波数(Hz)
        static float   gFreqX = 0.0f, gFreqY = 0.0f, gFreqRot = 0.0f;
        // 位相
        static float   gPhaseX = 0.0f, gPhaseY = 0.0f, gPhaseRot = 0.0f;
        // 減衰カーブ
        static EaseFn  gEnvelope = nullptr;

        // ====== 追従 / 制約 ======
        static const Vector2* gFollowTarget = nullptr; // ワールド座標のポインタ
        static Vector2 gDeadzoneHalf = { 160.0f, 90.0f }; // 既定のデッドゾーン半サイズ（320x180 px）
        static float   gFollowStiffness = 0.18f;
        static float   gMaxFollowSpeed = 0.0f; // 0 = 上限なし
        static bool    gHasBounds = false;
        static RectF   gWorldBounds{ 0,0,0,0 };

        // ====== ユーティリティ ======
        static inline float Clamp(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
        static inline float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
        static inline float Hash01(float x) { float s = std::sin(x * 12.9898f) * 43758.5453f; return s - std::floor(s); }

        static inline Vector2 TransformPoint(const Vector2& p, const Matrix3x3& m) {
            return { p.x * m.m[0][0] + p.y * m.m[1][0] + m.m[2][0],
                     p.x * m.m[0][1] + p.y * m.m[1][1] + m.m[2][1] };
        }
        static Matrix3x3 InverseAffine(const Matrix3x3& m) {
            float a = m.m[0][0], c = m.m[1][0], tx = m.m[2][0];
            float b = m.m[0][1], d = m.m[1][1], ty = m.m[2][1];
            float det = a * d - b * c;
            if (std::fabs(det) < 1e-6f) return Matrix3x3::MakeIdentity();
            float invDet = 1.0f / det;
            Matrix3x3 r{};
            r.m[0][0] = d * invDet; r.m[0][1] = -b * invDet; r.m[0][2] = 0.0f;
            r.m[1][0] = -c * invDet; r.m[1][1] = a * invDet; r.m[1][2] = 0.0f;
            r.m[2][0] = (c * ty - d * tx) * invDet;
            r.m[2][1] = (b * tx - a * ty) * invDet;
            r.m[2][2] = 1.0f;
            return r;
        }
        static inline Vector2 ViewportHalfWorld() {
            // ズーム倍率に応じて変化する可視半径（ワールドサイズ）
            return { (gScreenW * 0.5f) / gState.scale.x, (gScreenH * 0.5f) / gState.scale.y };
        }

        static void ApplyDebugControl(float dt) {
            (void)dt;
            if (!gDebugControlEnabled) return;
            if (!HINPUT::IsLayerActive(gDebugLayerName)) return;

            const bool dragging = HINPUT::IsDown(gDebugActions.dragButton);
            float dragX = dragging ? HINPUT::GetAxis(gDebugActions.dragAxisX) : 0.0f;
            float dragY = dragging ? HINPUT::GetAxis(gDebugActions.dragAxisY) : 0.0f;
            if (std::fabs(dragX) > 0.0001f || std::fabs(dragY) > 0.0001f) {
                gState.position.x -= dragX / gState.scale.x;
                gState.position.y -= dragY / gState.scale.y;
            }

            float zoomInput = HINPUT::GetAxis(gDebugActions.zoomAxis);
            if (std::fabs(zoomInput) > 0.0001f) {
                float factor = 1.0f + zoomInput * gDebugZoomStep;
                if (factor < 0.1f) factor = 0.1f;
                gState.scale.x = Clamp(gState.scale.x * factor, gDebugZoomMin, gDebugZoomMax);
                gState.scale.y = Clamp(gState.scale.y * factor, gDebugZoomMin, gDebugZoomMax);
            }
        }

        // ====== 基本 Set/Get ======
        void SetScreenCenter(const Vector2& screenCenter) { gScreenCenter = screenCenter; }
        void SetPosition(const Vector2& pos) { gState.position = pos; }
        void SetScale(const Vector2& s) { gState.scale = s; }
        void SetRotation(float rad) { gState.rotation = rad; }
        void SetPivot(const Vector2& p) { gState.pivot = p; }

        // ====== 拡張 ======
        void SetScreenSize(int w, int h) {
            if (w <= 0 || h <= 0) return;
            gScreenW = w; gScreenH = h;
            gScreenCenter = { w * 0.5f, h * 0.5f };
            gState.pivot = gScreenCenter;
        }

        // ====== シェイク ======
        void ShakeEx(const ShakeParams& p) {
            gShakeTime = 0.0f;
            gShakeDur = (p.durationSec < 0.0f ? 0.0f : p.durationSec);

            gAmpX = p.ampX;   gAmpY = p.ampY;   gAmpRot = p.ampRot;
            gFreqX = p.freqX;  gFreqY = p.freqY;  gFreqRot = p.freqRot;

            // 初期位相のランダム化（同一パターンの回避）
            float seed = gShakeTime + gShakeDur + gAmpX * 0.37f + gAmpY * 0.73f + gAmpRot * 1.11f;
            gPhaseX = Hash01(seed + 1.23f) * 6.2831853f;
            gPhaseY = Hash01(seed + 4.56f) * 6.2831853f;
            gPhaseRot = Hash01(seed + 7.89f) * 6.2831853f;

            gEnvelope = p.envelope; // nullptr を許可
        }
        void Shake(float amplitude, float durationSec) {
            ShakeParams p;
            p.ampX = amplitude;
            p.ampY = amplitude * 0.8f;
            p.ampRot = amplitude * 0.0025f;
            p.freqX = 12.0f; p.freqY = 9.0f; p.freqRot = 7.0f;
            p.durationSec = durationSec;
            p.envelope = nullptr;
            ShakeEx(p);
        }
        void StopShake() {
            gShakeTime = 0.0f; gShakeDur = 0.0f;
            gAmpX = gAmpY = gAmpRot = 0.0f;
            gFreqX = gFreqY = gFreqRot = 0.0f;
        }

        // ====== 追従 / 制約 ======
        void SetFollowTarget(const Vector2* targetWorldPosPtr) { gFollowTarget = targetWorldPosPtr; }
        void SetDeadzoneHalfSize(const Vector2& halfSizePx) { gDeadzoneHalf = halfSizePx; }
        void SetFollowStiffness(float s01) {
            if (s01 < 0.0f) s01 = 0.0f; if (s01 > 1.0f) s01 = 1.0f; gFollowStiffness = s01;
        }
        void SetMaxFollowSpeed(float pxPerSec) { gMaxFollowSpeed = (pxPerSec < 0.0f ? 0.0f : pxPerSec); }
        void SetBoundsWorld(const RectF& wr) { gWorldBounds = wr; gHasBounds = (wr.width > 0.0f && wr.height > 0.0f); }

        // ====== 更新 ======
        void Update(float dt) {
            if (dt < 0.0f) dt = 0.0f;

            // (0) シェイクの計算
            Vector2 shakeOffset{ 0.0f, 0.0f };
            float   shakeAngle = 0.0f;

            if (gShakeDur > 0.0f && (gAmpX > 0.0f || gAmpY > 0.0f || gAmpRot > 0.0f)) {
                gShakeTime += dt;
                float u = (gShakeDur > 0.0f) ? (gShakeTime / gShakeDur) : 1.0f;
                u = Clamp01(u);

                // 残り時間に応じたエンベロープ（減衰）を適用
                float env = (gEnvelope ? gEnvelope(1.0f - u) : (1.0f - u));

                // 位相更新
                const float TAU = 6.28318530718f;
                gPhaseX += TAU * gFreqX * dt;
                gPhaseY += TAU * gFreqY * dt;
                gPhaseRot += TAU * gFreqRot * dt;

                // 正弦波合成
                shakeOffset.x = gAmpX * env * std::sin(gPhaseX);
                shakeOffset.y = gAmpY * env * std::cos(gPhaseY);
                shakeAngle = gAmpRot * env * std::sin(gPhaseRot);

                if (u >= 1.0f) { StopShake(); }
            }

            // (1) 追従処理
            if (gFollowTarget) {
                Vector2 cam = gState.position;

                // 相対変位（ワールド）
                Vector2 delta{ gFollowTarget->x - cam.x, gFollowTarget->y - cam.y };

                // デッドゾーンをワールドスケールに換算（ズーム依存）
                Vector2 halfWorld{ gDeadzoneHalf.x / gState.scale.x, gDeadzoneHalf.y / gState.scale.y };

                Vector2 move{};
                if (std::fabs(delta.x) > halfWorld.x) move.x = delta.x - (delta.x > 0 ? halfWorld.x : -halfWorld.x);
                if (std::fabs(delta.y) > halfWorld.y) move.y = delta.y - (delta.y > 0 ? halfWorld.y : -halfWorld.y);

                // スムーズ追従
                cam.x += move.x * gFollowStiffness;
                cam.y += move.y * gFollowStiffness;

                // 最大速度制限
                if (gMaxFollowSpeed > 0.0f) {
                    Vector2 diff{ cam.x - gState.position.x, cam.y - gState.position.y };
                    float len2 = diff.x * diff.x + diff.y * diff.y;
                    float maxStep = gMaxFollowSpeed * dt;
                    if (len2 > maxStep * maxStep) {
                        float len = std::sqrt(len2);
                        cam.x = gState.position.x + diff.x * (maxStep / len);
                        cam.y = gState.position.y + diff.y * (maxStep / len);
                    }
                }

                // ビューポート半径に基づく境界制限
                if (gHasBounds) {
                    Vector2 vh = ViewportHalfWorld();
                    float minX = gWorldBounds.x + vh.x;
                    float maxX = gWorldBounds.x + gWorldBounds.width - vh.x;
                    float minY = gWorldBounds.y + vh.y;
                    float maxY = gWorldBounds.y + gWorldBounds.height - vh.y;
                    if (minX <= maxX) cam.x = Clamp(cam.x, minX, maxX);
                    if (minY <= maxY) cam.y = Clamp(cam.y, minY, maxY);
                }

                gState.position = cam;
            }

            // (1.5) デバッグ入力による手動パン / ズーム
            ApplyDebugControl(dt);

            // (2) ビュー行列の合成
            Matrix3x3 mScreen = Matrix3x3::MakeTranslate(gScreenCenter.x, gScreenCenter.y);
            Matrix3x3 mShake = Matrix3x3::MakeTranslate(shakeOffset.x, shakeOffset.y);
            Matrix3x3 mPivot = Matrix3x3::MakeTranslate(gState.pivot.x, gState.pivot.y);
            Matrix3x3 mNegPivot = Matrix3x3::MakeTranslate(-gState.pivot.x, -gState.pivot.y);
            Matrix3x3 mR = Matrix3x3::MakeRotate(gState.rotation + shakeAngle);
            Matrix3x3 mS = Matrix3x3::MakeScale(gState.scale.x, gState.scale.y);
            Matrix3x3 mNegPos = Matrix3x3::MakeTranslate(-gState.position.x, -gState.position.y);

            gView = mScreen * mShake * mPivot * mR * mS * mNegPivot * mNegPos;
        }

        // ====== 取得 ======
        const Matrix3x3& GetViewMatrix() { return gView; }
        State            GetState() { return gState; }

        Vector2 GetPosition()
        {
            return gState.position;
        }

        int GetScreenWidth() {
            return gScreenW;
        }
        int GetScreenHeight() {
            return gScreenH;
        }

        void EnableDebugControl(bool enable) { gDebugControlEnabled = enable; }
        void SetDebugLayer(const std::string& layerName) { gDebugLayerName = layerName; }
        void SetDebugZoomStep(float step) { gDebugZoomStep = step; }
        void SetDebugZoomLimits(float minZoom, float maxZoom) {
            gDebugZoomMin = (minZoom < 0.0001f) ? 0.0001f : minZoom;
            gDebugZoomMax = (maxZoom < gDebugZoomMin) ? gDebugZoomMin : maxZoom;
        }
        void SetDebugActions(const std::string& dragButton, const std::string& dragAxisX, const std::string& dragAxisY, const std::string& zoomAxis) {
            if (!dragButton.empty()) gDebugActions.dragButton = dragButton;
            if (!dragAxisX.empty()) gDebugActions.dragAxisX = dragAxisX;
            if (!dragAxisY.empty()) gDebugActions.dragAxisY = dragAxisY;
            if (!zoomAxis.empty()) gDebugActions.zoomAxis = zoomAxis;
        }

        // ====== 座標変換 ======
        Vector2 WorldToScreen(const Vector2& w) { return TransformPoint(w, gView); }
        Vector2 ScreenToWorld(const Vector2& s) { return TransformPoint(s, InverseAffine(gView)); }

    } // namespace CAMERA
} // namespace HIKARI
