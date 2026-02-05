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

    // 1. 基础形状判定
    // 我们不再依赖外部传入的中心点，而是直接看贴图有没有 Alpha
    // 如果这里是透明的，就直接丢弃 (不做外部光环了)
    if (rawCol.a <= 0.01)
        discard;

    // 2. 热浪扭曲 (Heat Haze) - 仅作用于颜色采样，不改变形状
    // 这样 Boss 的轮廓还是清晰的，但里面的纹理在流动
    float2 flow = float2(0.0, -time * 1.5);
    float noiseVal = noise(uv * 12.0 + flow);
    float2 warp = (float2(noiseVal, noiseVal) - 0.5) * 0.02 * progress;
    
    // 采样扭曲后的颜色
    float4 distCol = gTex.Sample(gSamp, uv + warp);

    // 3. 颜色重映射 (Color Grading) -> 制造 "焦炭与岩浆" 效果
    // 计算灰度
    float gray = dot(distCol.rgb, float3(0.299, 0.587, 0.114));
    
    // 生成流动的岩浆遮罩
    float magma = flowNoise(uv + warp); // 使用扭曲后的 UV 采样噪声
    
    // --- 调色板定义 (针对暗红背景优化) ---
    // 暗部：深褐红 (比背景稍亮一点，融入环境)
    float3 darkCharcoal = float3(0.3, 0.05, 0.05);
    // 中间：鲜红/猩红 (主要视觉颜色)
    float3 brightRed = float3(1.0, 0.1, 0.05);
    // 高光：炽热黄白 (用于岩浆流动)
    float3 hotLava = float3(1.0, 0.9, 0.6);

    // 混合逻辑：
    // 基于原始亮度(gray) 和 噪声(magma) 共同决定
    float heatMap = gray + magma * 0.6 * progress; // 噪声影响热度
    
    float3 burnColor = lerp(darkCharcoal, brightRed, smoothstep(0.2, 0.6, heatMap));
    burnColor = lerp(burnColor, hotLava, smoothstep(0.6, 1.1, heatMap));

    // 4. 强力边缘光 (Rim Light / Outline) - 关键步骤
    // 利用 Alpha 梯度来检测边缘。
    // rawCol.a 在物体内部是 1，在边缘会从 1 变到 0。
    // 我们提取 0.5 ~ 0.9 这个区间的 Alpha，把它变成高亮的边。
    float rimMask = smoothstep(0.4, 0.8, rawCol.a) * (1.0 - smoothstep(0.95, 1.0, rawCol.a));
    
    // 边缘颜色：极其明亮的金黄色，用来把 Boss 从暗红背景里“抠”出来
    float3 rimColor = float3(1.0, 0.8, 0.4) * 4.0; // 强度乘 4，产生辉光感

    // 5. 最终混合
    // 在转场过程中，从原色渐变到燃烧色
    float3 finalRGB = lerp(rawCol.rgb, burnColor, progress);
    
    // 叠加边缘光 (只在燃烧时显示)
    finalRGB += rimColor * rimMask * progress;

    // 6. 整体提亮
    // 为了防止在暗红背景里看不清，整体稍微加一点自发光
    finalRGB *= 1.2;

    return float4(finalRGB, rawCol.a);
}