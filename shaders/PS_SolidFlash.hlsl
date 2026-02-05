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
    // user[0].x = strength (0~1)
    // user[0].yzw = flashColor (RGB)
};

float Luma(float3 c)
{
    return dot(c, float3(0.299, 0.587, 0.114));
}

float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_TARGET
{
    float4 base = gTex.Sample(gSamp, uv);
    if (base.a <= 0.01)
        discard;

    float s = saturate(user[0].x);
    if (s <= 0.0001)
        return base;

    float3 flashColor = user[0].yzw;

    float2 px = float2(1.0 / max(resX, 1.0), 1.0 / max(resY, 1.0));

    // 自动中心（不传参）
    float2 center = float2(0.5, 0.5);
    float2 d = uv - center;
    float dist = length(d);
    float2 dir = (dist > 1e-5) ? (d / dist) : float2(0.0, 0.0);

    // 推进：比上一版慢一点，避免频闪刺眼
    float t = frac(time * (1.2 + 2.8 * s));

    // ----------------------------
    // 1) 冲击环（主打击感来源，亮度更克制）
    // ----------------------------
    float radius = lerp(0.10, 0.42, t);
    float ringWidth = lerp(0.020, 0.055, s);
    float ring = 1.0 - smoothstep(ringWidth, ringWidth * 2.0, abs(dist - radius));
    ring *= smoothstep(0.80, 0.0, dist);
    ring *= s;

    // ----------------------------
    // 2) 核心（大幅减弱，只留一点“啪”的中心响应）
    // ----------------------------
    float core = smoothstep(0.16, 0.0, dist);
    core = pow(core, 2.4);
    core *= (0.15 + 0.25 * s); // << 比上一版小很多
    core *= (0.75 + 0.25 * sin(time * 22.0)); // 微抖动，但很轻
    core = saturate(core);

    // ----------------------------
    // 3) 轻微色散 + 轻微形变（别太花）
    // ----------------------------
    float aberr = (ring * 0.55 + core * 0.25) * (0.6 + 0.9 * s);
    float2 off = px * (1.0 + 5.0 * s) * aberr;

    float warp = (ring * 0.55 + core * 0.20) * (0.001 + 0.004 * s);
    float2 uvWarp = uv + dir * warp;

    float r = gTex.Sample(gSamp, uvWarp + off).r;
    float g = gTex.Sample(gSamp, uvWarp).g;
    float b = gTex.Sample(gSamp, uvWarp - off).b;
    float3 rgb = float3(r, g, b);

    // ----------------------------
    // 4) 描边高光（STG里最实用：清晰、但不刺眼）
    // ----------------------------
    float lumR = Luma(gTex.Sample(gSamp, uv + float2(px.x, 0)).rgb);
    float lumL = Luma(gTex.Sample(gSamp, uv - float2(px.x, 0)).rgb);
    float lumU = Luma(gTex.Sample(gSamp, uv + float2(0, px.y)).rgb);
    float lumD = Luma(gTex.Sample(gSamp, uv - float2(0, px.y)).rgb);
    float edge = abs(lumR - lumL) + abs(lumU - lumD);

    // edge 强度收敛 + 只在 ring 周围更明显
    edge = saturate(edge * (5.0 + 12.0 * s));
    edge *= (0.20 + 0.80 * ring);

    // ----------------------------
    // 5) 很轻的余辉残影（保留一点“动感”，但不发光炸屏）
    // ----------------------------
    float2 trailUv = uv - dir * (0.003 + 0.008 * s);
    float3 trail = gTex.Sample(gSamp, trailUv).rgb;
    float trailAmt = (0.05 + 0.25 * ring) * s;

    // ----------------------------
    // 合成：整体更克制
    // ----------------------------
    float burst = saturate(ring * 0.85 + core * 0.55);

    // 向闪光色推进（比上一版更小；并且乘 base.a 防止半透明边缘刺眼）
    float tintAmt = burst * (0.25 + 0.35 * s) * base.a;
    float3 outRGB = lerp(rgb, flashColor, tintAmt);

    // 过曝/发光：几乎不做，只留一点“能量感”
    outRGB += flashColor * (ring * 0.18 + core * 0.10) * (0.25 + 0.35 * s) * base.a;

    // 描边：偏白一点更“清爽”，但强度很低
    outRGB += (flashColor * 0.6 + 0.4) * edge * (0.10 + 0.20 * s) * base.a;

    // 残影：轻量叠加
    outRGB = lerp(outRGB, outRGB + trail * 0.35, trailAmt);

    return float4(outRGB, base.a);
}
