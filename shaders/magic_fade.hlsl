Texture2D gTex : register(t0);
SamplerState gSamp : register(s0);

cbuffer CommonParams : register(b0)
{
    float time;
    float deltaTime;
    float combo;
    float intensity;
    float resolutionX;
    float resolutionY;
    float pad0;
    float pad1;
    float4 user[16];
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float Hash21(float2 p)
{
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 34.345);
    return frac(p.x * p.y);
}

float4 main(PS_IN i) : SV_TARGET
{
    float4 col = gTex.Sample(gSamp, i.uv);
    if (col.a <= 0.001f)
        return col;

    float t = saturate(user[0].x); // 0=正常显示，1=完全被魔法幕布吞没/未显形
    float3 magicColor = saturate(user[0].yzw);

    if (t <= 0.001f)
        return col;

    float2 uv = i.uv;

    // 符文噪声（UI 内部滚动）
    float2 grid = floor((uv + float2(time * 0.06f, -time * 0.04f)) * 46.0f);
    float n = Hash21(grid);

    // dissolve 形状
    float dissolve = smoothstep(0.35f, 0.95f, n);

    // cover 越大，越被覆盖
    float cover = saturate(t * 1.1f);
    float mask = smoothstep(cover - 0.18f, cover + 0.18f, dissolve);

    // 边缘光
    float edge = 1.0f - abs(mask * 2.0f - 1.0f);
    edge = pow(saturate(edge), 6.0f);

    float3 base = col.rgb;
    float3 magic = base * 0.18f + magicColor * (0.35f + 0.65f * dissolve);
    magic += magicColor * edge * (1.7f + 0.3f * sin(time * 18.0f));

    float3 outRgb = lerp(base, magic, mask);
    outRgb *= (1.0f - 0.55f * mask);

    return float4(outRgb, col.a);
}
