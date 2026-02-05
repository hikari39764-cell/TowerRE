Texture2D gTex : register(t0);
SamplerState gSamp : register(s0);

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer CommonParams : register(b0)
{
    float time; // Shader 内部时间
    float deltaTime;
    float combo;
    float intensity;

    float resolutionX;
    float resolutionY;
    float pad0;
    float pad1;

    // User Data from C++
    // [0]: CenterXY, RingRadius, Thick
    // [1]: ManaCount, OrbitRadius, DotSize, OrbitSpeed
    // [2]: BaseTint, AuraIntensity
    // [3]: Enabled, SpawnFx, GlobalTime(Sync用), Unused
    // [4]..[9]: DeathFX (StartX, StartY, Progress(0~1), Type+Seed)
    float4 user[16];
};

static const float PI = 3.14159265;
static const float TAU = 6.28318530;

// -------------------------------------------------------------
// 工具函数
// -------------------------------------------------------------
float2 Rot(float2 v, float a)
{
    float s = sin(a), c = cos(a);
    return float2(v.x * c - v.y * s, v.x * s + v.y * c);
}
float Hash11(float p)
{
    p = frac(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return frac(p);
}
float Hash21(float2 p)
{
    float3 p3 = frac(float3(p.x, p.y, p.x) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}
float Noise21(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float a = Hash21(i);
    float b = Hash21(i + float2(1, 0));
    float c = Hash21(i + float2(0, 1));
    float d = Hash21(i + float2(1, 1));
    float2 u = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
}
float FBM(float2 p)
{
    float v = 0.0;
    float a = 0.5;
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        v += a * Noise21(p);
        p *= 2.02;
        a *= 0.5;
    }
    return v;
}
float Soft(float x)
{
    x = saturate(x);
    return x * x * (3.0 - 2.0 * x);
}
float RingMask(float d, float r, float thick)
{
    return Soft(1.0 - abs(d - r) / thick);
}
float RuneSeg(float ang, float segCount, float jitter)
{
    float u = (ang + PI) / TAU;
    float idx = floor(u * segCount);
    float f = frac(u * segCount);
    float rnd = Hash11(idx + jitter);
    float core = Soft(1.0 - abs(f - 0.5) * 2.0);
    float gate = step(0.25, rnd);
    return core * gate * (0.4 + 0.9 * rnd);
}
float TrailFalloff(float2 dp, float2 tangent, float trailLen, float width)
{
    float t = dot(dp, tangent);
    float n = dot(dp, float2(-tangent.y, tangent.x));
    float back = saturate((-t) / max(trailLen, 1.0));
    float along = exp(-back * 2.2);
    float w = exp(-abs(n) / max(width, 0.5));
    float head = exp(-abs(t) / max(trailLen, 1.0));
    return along * w * head;
}
float3 Palette(float t)
{
    float3 c1 = float3(1.00, 0.08, 0.35);
    float3 c2 = float3(0.55, 0.12, 1.00);
    float3 c3 = float3(1.00, 0.35, 0.95);
    float u = sin(t) * 0.5 + 0.5;
    float v = sin(t * 0.7 + 1.4) * 0.5 + 0.5;
    return lerp(lerp(c1, c2, u), c3, v * 0.55);
}

// -------------------------------------------------------------
// Main Shader
// -------------------------------------------------------------
float4 main(PS_IN i) : SV_TARGET
{
    if (user[3].x < 0.5)
        return float4(0, 0, 0, 0);

    // --- 参数读取 ---
    float2 centerPx = user[0].xy;
    float ringR = user[0].z;
    float thick = max(user[0].w, 1.0);

    int manaCount = (int) user[1].x;
    float orbitR = user[1].y;
    float dotSize = max(user[1].z, 1.0);
    float orbitSpd = user[1].w;

    float3 baseTint = user[2].xyz;
    float auraInt = user[2].w;

    float addFx = saturate(user[3].y);
    float gTime = user[3].z;

    float2 px = float2(i.uv.x * resolutionX, i.uv.y * resolutionY);
    float2 p = px - centerPx;
    float d = length(p) + 1e-4;
    float ang = atan2(p.y, p.x);

    // ========================================================
    // 1. 背景圆环与符文
    // ========================================================
    float2 pn = p / max(ringR, 1.0);
    pn = Rot(pn, time * 0.35);
    float n1 = FBM(pn * 3.5 + float2(time * 0.6, -time * 0.45));
    float n2 = FBM(pn * 7.2 + float2(-time * 0.9, time * 0.25));
    float energy = lerp(n1, n2, 0.55);

    float thin = clamp(thick * 0.25, 1.0, 2.6);
    float thinGlow = thin * 2.35;
    
    // 内部挖空遮罩 (背景层被挖空，特效层不被挖空)
    float innerMask = smoothstep(ringR * 0.60, ringR * 0.70, d);

    float star = 1.0 + 0.045 * sin(ang * 10.0 + time * 1.15 + energy * 1.6);
    float breathe = 1.0 + 0.015 * sin(time * 4.2 + ang * 2.0 + energy * 1.6);

    float r0 = ringR * breathe * star;
    float ring0 = RingMask(d, r0, thin);
    float ring0Glow = RingMask(d, r0, thinGlow) * 0.58;

    float r1 = ringR * (1.18 + 0.014 * sin(time * 3.1 + energy * 5.0));
    float ring1 = RingMask(d, r1, thin * 0.78);
    float ring1Glow = RingMask(d, r1, thinGlow * 0.95) * 0.46;

    float rune = RuneSeg(ang + time * 0.85, 22.0, 13.7) * ring0;
    float rune2 = RuneSeg(ang - time * 1.05, 33.0, 7.3) * ring1;

    float3 cA = Palette(ang * 1.8 + time * 1.1);
    float3 cB = Palette(ang * 2.3 - time * 0.9 + 1.7);

    float3 ringCol = 0;
    ringCol += cA * ring0 * 0.95;
    ringCol += cA * ring0Glow * 0.72;
    ringCol += cB * ring1 * 0.80;
    ringCol += cB * ring1Glow * 0.62;
    ringCol += (cB * 1.28 + float3(0.10, 0.03, 0.18)) * rune * 0.62;
    ringCol += (cA * 1.12) * rune2 * 0.52;

    float ringFogN = FBM(pn * 8.0 + float2(time * 1.2, -time * 1.0));
    float ringFog = exp(-abs(d - ringR) / max(ringR * 0.10, 6.0)) * (0.18 + 0.22 * ringFogN);
    ringCol += (cA * 0.45 + cB * 0.55 + float3(0.10, 0.02, 0.18)) * ringFog;

    // 装饰粒子 Spark / Glitter
    float spark = 0.0;
    float nearRing = saturate(ring0Glow + ring1Glow);
    float2 sp = (p / max(ringR, 1.0)) * 12.0;
    float sN = Noise21(sp + float2(time * 1.9, -time * 1.4));
    float tw = sin(time * 11.5 + sN * 12.0) * 0.5 + 0.5;
    spark = step(0.93, sN) * tw * nearRing;
    float3 sparkCol = (float3(1.0, 0.65, 0.98) + cB) * spark * 0.78;

    float2 pp = p / max(ringR, 1.0);
    float glitter = 0.0;
    float g0 = Noise21(pp * 48.0 + float2(time * 4.5, -time * 3.8));
    float g1 = Noise21(pp * 76.0 + float2(-time * 5.4, time * 4.2));
    float g = max(g0, g1);
    float gGate = step(0.965, g);
    float gTw = sin(time * (18.0 + 10.0 * energy) + g * 15.0) * 0.5 + 0.5;
    glitter = gGate * gTw * (0.25 + 0.75 * nearRing);
    ringCol += (float3(1.0, 0.85, 1.0) + cA) * glitter * 0.25;

    // ========================================================
    // 2. Spawn / Summon 效果 (增加魔力时)
    // ========================================================
    float spawnGlobal = addFx;
    float spawnTg = 1.0 - spawnGlobal;
    float gatherG = Soft(saturate(spawnTg / 0.35));
    float condG = Soft(saturate((spawnTg - 0.35) / 0.40));
    float summonGate = spawnGlobal;

    float3 summonCol = 0;
    if (summonGate > 0.001)
    {
        float rS0 = ringR * (1.35 + 0.08 * sin(time * 2.1 + energy * 2.0));
        float rS1 = ringR * (1.55 + 0.10 * sin(time * 1.7 - energy * 2.2));
        float wS0 = max(1.0, thin * (1.10 + 0.80 * gatherG));
        float wS1 = max(1.0, thin * (0.95 + 0.70 * gatherG));
        float s0 = RingMask(d, rS0, wS0);
        float s1 = RingMask(d, rS1, wS1);
        float seg0 = RuneSeg(ang + time * (2.6 + 0.8 * energy), 48.0, 21.7);
        float seg1 = RuneSeg(ang - time * (2.2 + 0.7 * energy), 72.0, 9.3);
        float pulseS = 0.70 + 0.30 * sin(time * 9.0 + energy * 5.0);
        float sig0 = s0 * seg0 * (0.55 + 0.75 * pulseS);
        float sig1 = s1 * seg1 * (0.50 + 0.65 * pulseS);
        float haloS = exp(-abs(d - rS0) / max(ringR * 0.08, 5.0)) * (0.35 + 0.35 * pulseS);
        float rays = pow(saturate(sin(ang * (18.0 + 6.0 * energy) + time * (10.0 + 4.0 * energy)) * 0.5 + 0.5), 10.0);
        rays *= exp(-abs(d - rS0) / max(ringR * 0.14, 8.0));
        float swirlN = FBM(pp * 6.0 + float2(time * 1.8, -time * 1.6));
        float swirl = smoothstep(0.45, 0.92, swirlN) * exp(-abs(d - rS1) / max(ringR * 0.18, 10.0));
        float fadeS = (1.0 - condG) * summonGate;
        float3 sCol = (cA * 0.55 + cB * 0.85 + float3(0.18, 0.03, 0.22));
        summonCol += sCol * (sig0 * 1.10 + sig1 * 0.85 + haloS * 0.55 + rays * 0.80 + swirl * 0.65) * fadeS;
    }

    // ========================================================
    // 3. 活跃魔力点 (Living Dots)
    // ========================================================
    float3 dotsCol = 0;
    float dotsA = 0.0;

    if (manaCount > 0)
    {
        // 基础角度使用 gTime (C++时间)，保证位置同步
        float baseA = gTime * orbitSpd;

        [unroll]
        for (int k = 0; k < 12; ++k)
        {
            if (k >= manaCount)
                break;

            float id = (float) k;
            float rnd = Hash11(id * 17.13 + 3.7);

            // Spawn 动画逻辑
            float newId = max((float) manaCount - 1.0, 0.0);
            float isNew = 1.0 - step(0.6, abs(id - newId));
            float spawn = isNew * addFx;
            float spawnT = 1.0 - spawn;
            float condenseT = Soft(saturate((spawnT - 0.35) / 0.40));
            float lockT = Soft(saturate((spawnT - 0.75) / 0.25));
            float fadeOut = exp(-spawnT * 0.9);
            float pulse = 0.82 + 0.32 * sin(time * (3.6 + 1.6 * rnd) + id * 2.1 + energy * 1.6);

            // 位置计算
            float a = baseA
                    + TAU * id / (float) manaCount
                    + 0.10 * sin(time * (1.9 + 1.0 * rnd) + id * 2.0 + energy * 2.2);

            float2 dir = float2(cos(a), sin(a));
            float2 tangent = float2(-dir.y, dir.x);
            float wob = 0.10 * sin(time * (4.2 + 1.8 * rnd) + id * 1.7 + energy * 2.0);

            float spawnR = lerp(orbitR * 1.55, orbitR, condenseT);
            float2 dotPos = dir * spawnR + tangent * (dotSize * 0.55 * wob * lockT);
            float2 dp = p - dotPos;
            float dd = length(dp);

            // 尺寸与颜色
            float sizeBase = dotSize * lerp(0.92, 1.32, pulse);
            float sizeNow = sizeBase * lerp(0.55, 1.00, condenseT);
            float localAng = atan2(dp.y, dp.x);
            float gradT = Soft(saturate(dd / max(sizeNow * 2.6, 0.001)));
            float3 colCoreW = float3(1.0, 0.96, 1.0);
            float3 colMid = float3(1.0, 0.55, 0.95);
            float3 colEdge = float3(1.0, 0.18, 0.45);
            float3 colGrad = lerp(lerp(colCoreW, colMid, gradT), colEdge, gradT * 0.55);

            // 核心绘制
            float spin = time * (1.9 + 1.35 * rnd) + id * 1.7 + energy * 0.8;
            float2 q = Rot(dp, spin);
            float coreGeom = Soft(1.0 - (abs(q.x) + abs(q.y)) / max(sizeNow, 1.0));
            float blob = exp(-dd / max(sizeNow * lerp(2.6, 1.0, condenseT), 1.0));
            float core = lerp(blob, coreGeom, condenseT);
            float highlight = saturate(exp(-dd / max(sizeNow * 0.50, 1.0)));

            // 十字
            float lineW = max(0.9, sizeNow * 0.15);
            float cross = exp(-abs(q.x) / lineW) * exp(-abs(q.y) / (lineW * 3.0))
                        + exp(-abs(q.y) / lineW) * exp(-abs(q.x) / (lineW * 3.0));
            cross = saturate(cross) * condenseT;

            // 装饰
            float halo = RingMask(dd, sizeNow * (1.55 + 0.20 * sin(time * (2.0 + rnd) + id)), max(1.0, sizeNow * 0.18));
            float petals = pow(saturate(sin(localAng * 3.0 + time * 2.8) * 0.5 + 0.5), 10.0) * exp(-dd / (sizeNow * 1.9)) * condenseT;
            float trail = TrailFalloff(dp, tangent, sizeNow * 4.8, sizeNow * 0.78) * lockT;

            // 颜色叠加
            float3 cDot = 0;
            cDot += colCoreW * (core * 1.1 + highlight * 1.0 + cross * 0.9);
            cDot += colGrad * (halo + petals + trail);
            
            // Spawn Flash
            float pop1 = RingMask(dd, sizeNow * lerp(1.15, 5.0, condenseT), sizeNow * 0.2) * exp(-condenseT * 1.6) * 0.70;
            cDot += (colMid + float3(0.15, 0.02, 0.18)) * pop1 * spawn;

            dotsCol += cDot;
            dotsA += core * 0.58 + highlight * 0.45 + trail * 0.2 + pop1 * 0.24;
        }
    }

  // ========================================================
    // 4. 事件特效 (Event FX) - 优化版：降低过曝，提升辨识度
    // ========================================================
    float3 fxCol = 0;
    float3 impactCol = 0;

    [unroll]
    for (int j = 4; j < 10; ++j)
    {
        float4 data = user[j];
        float t = data.z; // Progress 0~1
        if (t < 0.0 || t >= 1.0)
            continue;

        // 解包
        float rawType = floor(data.w);
        float seed = frac(data.w) * 1000.0;

        float2 startPos = data.xy - centerPx;
        
        // --- 颜色配置 (关键修改：核心不再是纯白，而是带颜色的高亮) ---
        float3 colCore = float3(1, 1, 1);
        float3 colTrail = float3(0.5, 0.5, 0.5); // 拖尾用深色

        if (rawType == 1.0)
        { // [红: 攻击] - 偏橙红，高饱和
            colCore = float3(1.0, 0.6, 0.6); // 核心微红
            colTrail = float3(1.0, 0.1, 0.05); // 拖尾深红
        }
        else if (rawType == 2.0)
        { // [蓝: 速度] - 电光蓝
            colCore = float3(0.6, 0.8, 1.0); // 核心微蓝
            colTrail = float3(0.0, 0.5, 1.0); // 拖尾深蓝
        }
        else if (rawType == 3.0)
        { // [紫: 闪避] - 深紫
            colCore = float3(0.9, 0.7, 1.0); // 核心微紫
            colTrail = float3(0.7, 0.0, 1.0); // 拖尾深紫
        }
        else if (rawType == 4.0)
        { // [绿: 回复] - 翡翠绿
            colCore = float3(0.7, 1.0, 0.7); // 核心微绿
            colTrail = float3(0.0, 1.0, 0.3); // 拖尾深绿
        }

        // --- Stage 1: Burst (飞出粒子) ---
        float burstT = saturate(t / 0.20);
        if (burstT > 0.0 && burstT < 1.0)
        {
            float fade = 1.0 - burstT;
            for (int pId = 0; pId < 3; pId++)
            { // 粒子数适中
                float pRnd = Hash11(seed + pId * 0.13);
                float pAng = pRnd * TAU;
                float2 dir = float2(cos(pAng), sin(pAng));
                float flyDist = 35.0 * burstT;
                float2 pPos = startPos + dir * flyDist;
                float dPart = length(p - pPos);
                
                // 亮度降低：从 *8.0 降为 *3.0，颜色更实
                float spark = exp(-dPart / 1.5) * fade * 3.0;
                fxCol += spark * colTrail; // 使用深色拖尾色，避免发白
            }
        }

        // --- Stage 2: Stream (充能轨迹) ---
        float suckT = smoothstep(0.1, 0.85, t);
        if (suckT > 0.0 && suckT < 1.0)
        {
            float2 side = float2(-startPos.y, startPos.x) * 0.6;
            if (frac(seed) > 0.5)
                side *= -1.0;
            float2 cp = startPos * 0.5 + side;
            
            float2 currPos = lerp(lerp(startPos, cp, suckT), lerp(cp, float2(0, 0), suckT), suckT);
            
            // 头部：亮度从 *6.0 降为 *3.0
            float dHead = length(p - currPos);
            float headBlob = exp(-dHead / 3.0);
            fxCol += headBlob * colCore * 3.0;

            // 拖尾：强调颜色本身
            int trailCount = 5;
            for (int tr = 1; tr <= trailCount; tr++)
            {
                float lag = suckT - 0.03 * tr;
                if (lag > 0.0)
                {
                    float2 lagPos = lerp(lerp(startPos, cp, lag), lerp(cp, float2(0, 0), lag), lag);
                    float noiseWiggle = (Hash11(tr * 1.1 + time * 10.0) - 0.5) * 3.0;
                    float dTrail = length(p - lagPos + noiseWiggle);
                    
                    // 拖尾更细，颜色更深
                    float trailBase = exp(-dTrail / (1.2 + tr * 0.4));
                    fxCol += trailBase * colTrail * (0.8 / tr); // 移除 sparkIntensity 造成的过度闪烁
                }
            }
        }

        // --- Stage 3: Impact (中心爆发) ---
        float impactT = (t - 0.75) / 0.25;
        if (impactT > 0.0)
        {
            impactT = saturate(impactT);
            float fade = 1.0 - impactT;
            float fadeSmooth = smoothstep(1.0, 0.0, impactT);
            
            // 1. 中心一闪 (亮度降低，防止盖住玩家)
            float centerFlash = exp(-d / (ringR * 0.5)) * fadeSmooth;
            impactCol += centerFlash * colCore * 1.5;

            // 2. 冲击波环 (更清晰的线条，而非模糊的光晕)
            float waveR = ringR * 1.2 * pow(impactT, 0.6);
            float waveW = 8.0 * fade; // 变细一点
            float wave = RingMask(d, waveR, waveW);
            impactCol += wave * colTrail * 1.5 * fade;

            // 3. 类型专属 (强调形状清晰度)
            if (rawType == 1.0)
            {
                // [红: 攻击] 尖刺更锐利
                float spikes = pow(saturate(sin(ang * 12.0 + seed) * sin(ang * 7.0 - time * 10.0)), 10.0); // 指数提高，变尖
                float spikeRegion = RingMask(d, ringR * impactT * 1.2, 40.0);
                impactCol += spikes * spikeRegion * colTrail * 2.5 * fade;
            }
            else if (rawType == 2.0)
            {
                // [蓝: 速度] 线条感
                float speedLine = pow(saturate(sin(ang * 30.0 + impactT * 40.0)), 20.0); // 很细的线
                float lineRegion = RingMask(d, ringR * impactT * 1.4, 30.0);
                impactCol += speedLine * lineRegion * colCore * 2.0 * fade;
            }
            else if (rawType == 3.0)
            {
                // [紫: 闪避] 残影
                float echoR = ringR * (1.2 - impactT * 0.4);
                float echo = RingMask(d, echoR, 3.0); // 很细的环
                impactCol += echo * colTrail * 2.0 * fade;
            }
            else if (rawType == 4.0)
            {
                // [绿: 回复] +++ 符号
                for (int hp = 0; hp < 3; hp++)
                {
                    float hRnd = Hash11(seed + hp * 1.23);
                    float2 hOff = float2((hRnd - 0.5) * 70.0, -20.0 - 60.0 * impactT - hRnd * 30.0);
                    float2 dp = p - hOff;
                    float w = 2.0;
                    float l = 10.0;
                    float cross = exp(-abs(dp.x) / w) * exp(-abs(dp.y) / l) + exp(-abs(dp.y) / w) * exp(-abs(dp.x) / l);
                    // 绿色这里可以稍微亮一点，因为绿色本身容易暗
                    impactCol += saturate(cross) * colTrail * 3.0 * fade;
                }
            }
        }
    }


    // ========================================================
    // 5. 最终合成
    // ========================================================
    float3 finalCol = 0;
    
    // 背景层
    float3 bgLayer = ringCol + sparkCol + summonCol + dotsCol;
    finalCol += bgLayer * innerMask;

    // 特效层
    finalCol += fxCol;
    finalCol += impactCol;

    // 整体色调处理 (保持原样)
    finalCol *= auraInt;
    finalCol *= lerp(float3(1, 1, 1), baseTint, 0.30);

    // Alpha 计算 (优化混合)
    float alphaOut = 0.0;
    alphaOut += ring0 * 0.22 + ring0Glow * 0.18 + ring1 * 0.16;
    alphaOut += saturate(dotsA);
    alphaOut += saturate(length(summonCol) * 0.28) * summonGate;
    alphaOut *= innerMask;
    
    // 特效层 Alpha：不再简单相加，避免 Alpha 过大导致混合异常
    // 使用 max 或者 saturate 限制
    float fxAlpha = saturate(length(fxCol) * 0.8 + length(impactCol) * 0.8);
    alphaOut = max(alphaOut, fxAlpha); // 取最大值，保证清晰度

    return float4(finalCol, saturate(alphaOut));
}