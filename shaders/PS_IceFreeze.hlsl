Texture2D gTex : register(t0);
SamplerState gSamp : register(s0);

cbuffer CommonParams : register(b0)
{
    float time;
    float deltaTime;
    float combo;
    float intensity;
    float resX;
    float resY;
    float pad0;
    float pad1;
    float4 user[16]; 
};


float hash(float2 p)
{
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

float noise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);
    float res = lerp(lerp(hash(i + float2(0.0, 0.0)), hash(i + float2(1.0, 0.0)), f.x),
                     lerp(hash(i + float2(0.0, 1.0)), hash(i + float2(1.0, 1.0)), f.x), f.y);
    return res;
}

float fbm(float2 p)
{
    float total = 0.0;
    float amp = 0.5;
    for (int i = 0; i < 4; i++)
    {
        total += noise(p) * amp;
        p *= 2.0;
        amp *= 0.5;
    }
    return total;
}

float voronoi(float2 uv)
{
    float2 i = floor(uv);
    float2 f = frac(uv);
    float minDist = 1.0;
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            float2 neighbor = float2(float(x), float(y));
            float2 p = float2(hash(i + neighbor), hash(i + neighbor + 1.0));
            p = 0.5 + 0.5 * sin(time * 0.5 + 6.2831 * p);
            float2 diff = neighbor + p - f;
            float dist = length(diff);
            minDist = min(minDist, dist);
        }
    }
    return minDist;
}

float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float2 bossUV = user[1].xy;
    float progress = user[0].x;
    float4 rawCol = gTex.Sample(gSamp, uv);

    // 如果没有进度，直接返回原图
    if (progress <= 0.001 && intensity <= 0.0)
    {
        if (rawCol.a <= 0.01)
            discard;
        return rawCol;
    }

    // 计算距离中心的距离
    float aspect = resX / resY;
    float2 center = bossUV;
    float2 uvCorrected = uv;
    uvCorrected.x *= aspect;
    float2 centerCorrected = center;
    centerCorrected.x *= aspect;
    float distFromCenter = distance(uvCorrected, centerCorrected);

    float rangeMask = smoothstep(0.28, 0.12, distFromCenter);

    // 生成冰的折射
    float2 warp = float2(noise(uv * 10.0 + time * 0.5), noise(uv * 10.0 - time * 0.5));
    float2 iceUV = uv + (warp - 0.5) * 0.02 * progress;

    // 采样偏移颜色
    float4 refractedCol = gTex.Sample(gSamp, iceUV);

    // 寒气光环
    float4 auraCol = float4(0, 0, 0, 0);
    
    

    // 实体部分的冰冻处理
    float freezeNoise = voronoi(uv * 8.0);
    // 扩散动画也受距离限制
    float maskRadius = progress * 1.0;
    float freezeMask = smoothstep(maskRadius, maskRadius - 0.2, distFromCenter - freezeNoise * 0.1);
    if (progress >= 1.0)
        freezeMask = 1.0;

    // 颜色重映射
    float gray = dot(refractedCol.rgb, float3(0.299, 0.587, 0.114));
    float3 deepIce = float3(0.0, 0.2, 0.6); // 深蓝
    float3 midIce = float3(0.1, 0.7, 1.0); // 亮青
    float3 highlight = float3(0.9, 0.98, 1.0); // 白
    
    float3 iceColor = lerp(deepIce, midIce, smoothstep(0.0, 0.5, gray));
    iceColor = lerp(iceColor, highlight, smoothstep(0.5, 1.0, gray));
    
    // 加裂纹
    float cracks = smoothstep(0.08, 0.0, voronoi(uv * 12.0));
    iceColor += cracks * 0.4;

    float2 px = float2(1.0 / max(resX, 1.0), 1.0 / max(resY, 1.0));

// alpha 梯度
    float aC = rawCol.a;
    float aR = gTex.Sample(gSamp, uv + float2(px.x, 0)).a;
    float aL = gTex.Sample(gSamp, uv - float2(px.x, 0)).a;
    float aU = gTex.Sample(gSamp, uv - float2(0, px.y)).a;
    float aD = gTex.Sample(gSamp, uv + float2(0, px.y)).a;

    float grad = abs(aR - aL) + abs(aD - aU); 
    float rim = smoothstep(0.08, 0.35, grad);
    rim *= smoothstep(0.02, 0.25, aC); 
    rim *= rangeMask; // 只在 boss 周围

    float rimSharp = pow(saturate(rim), 1.6);

    float crackHi = smoothstep(0.10, 0.0, voronoi(uv * 18.0));
    crackHi = pow(saturate(crackHi), 1.2);


    float sp = noise(uv * 120.0 + time * 3.0);
    float sparkle = pow(saturate(sp), 18.0) * (0.6 + 0.4 * noise(uv * 25.0 - time * 1.5));
    sparkle *= rimSharp;

// 冰洁颜色
    float3 rimColOuter = float3(0.85, 0.98, 1.00);
    float3 rimColInner = float3(0.35, 0.80, 1.00);
    float3 rimCol = lerp(rimColInner, rimColOuter, smoothstep(0.0, 1.0, gray));

// 组合
    float rimAmp = 2.8;
    float crackAmp = 1.6;
    float sparkleAmp = 2.2;

    iceColor += rimCol * (rimSharp * rimAmp);
    iceColor += rimColOuter * (crackHi * rimSharp * crackAmp);
    iceColor += rimColOuter * (sparkle * sparkleAmp);

    // 混合实体
    float3 finalRGB = lerp(rawCol.rgb, iceColor, freezeMask);

    if (rawCol.a < 0.01)
    {
        return auraCol;
    }

    finalRGB += auraCol.rgb * auraCol.a * 0.5;

    return float4(finalRGB, rawCol.a);
}