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
    float4 user[16]; // user[0].x = progress
};

// --- 基础噪声函数 ---
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

// 模拟火焰流动的噪声
float flowNoise(float2 uv)
{
    // 双层噪声叠加，模拟复杂的火焰纹理
    float n1 = noise(uv * 8.0 + float2(0.0, -time * 2.0)); // 快流
    float n2 = noise(uv * 4.0 + float2(0.0, -time * 1.0)); // 慢流
    return (n1 + n2) * 0.5;
}

float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float progress = user[0].x;
    float4 rawCol = gTex.Sample(gSamp, uv);

    // 如果没有进度，直接返回原图
    if (progress <= 0.001)
    {
        if (rawCol.a <= 0.01)
            discard;
        return rawCol;
    }

    if (rawCol.a <= 0.01)
        discard;

    float2 flow = float2(0.0, -time * 1.5);
    float noiseVal = noise(uv * 12.0 + flow);
    float2 warp = (float2(noiseVal, noiseVal) - 0.5) * 0.02 * progress;

    float4 distCol = gTex.Sample(gSamp, uv + warp);


    float gray = dot(distCol.rgb, float3(0.299, 0.587, 0.114));
    
    // 生成流动的岩浆遮罩
    float magma = flowNoise(uv + warp); // 使用扭曲后的 UV 采样噪声
    
    // --- 调色板定义 ---
    // 暗部：深褐红
    float3 darkCharcoal = float3(0.3, 0.05, 0.05);
    // 中间：鲜红
    float3 brightRed = float3(1.0, 0.1, 0.05);
    // 高光：黄白
    float3 hotLava = float3(1.0, 0.9, 0.6);

    // 混合逻辑：
    float heatMap = gray + magma * 0.6 * progress; // 噪声影响热度
    
    float3 burnColor = lerp(darkCharcoal, brightRed, smoothstep(0.2, 0.6, heatMap));
    burnColor = lerp(burnColor, hotLava, smoothstep(0.6, 1.1, heatMap));

    // 4. 强力边缘光
    float rimMask = smoothstep(0.4, 0.8, rawCol.a) * (1.0 - smoothstep(0.95, 1.0, rawCol.a));
    float3 rimColor = float3(1.0, 0.8, 0.4) * 4.0; 

    // 5. 最终混合
    float3 finalRGB = lerp(rawCol.rgb, burnColor, progress);
    
    // 叠加边缘光
    finalRGB += rimColor * rimMask * progress;

    finalRGB *= 1.2;

    return float4(finalRGB, rawCol.a);
}