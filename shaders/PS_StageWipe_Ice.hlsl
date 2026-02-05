Texture2D gTex : register(t0);
SamplerState gSamp : register(s0);

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

cbuffer CommonParams : register(b0)
{
    float time, deltaTime, combo, intensity;
    float resolutionX, resolutionY, pad0, pad1;
    float4 user[16];
};

float Hash21(float2 p)
{
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}

float Noise(float2 p)
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
    for (int k = 0; k < 5; k++)
    {
        v += a * Noise(p);
        p = p * 2.03 + 17.1;
        a *= 0.5;
    }
    return v;
}

float Soft(float x)
{
    x = saturate(x);
    return x * x * (3.0 - 2.0 * x);
}

float3 ColdGrade(float3 c)
{
    float lum = dot(c, float3(0.299, 0.587, 0.114));
    float3 cool = lerp(float3(0.55, 0.75, 1.10), float3(0.70, 0.85, 1.30), lum);
    return saturate(c * cool);
}

float4 main(PS_IN i) : SV_TARGET
{
    float progress = user[0].x;
    float edgePx = max(user[0].y, 1.0);
    float noiseAmt = user[0].z;
    float fxStr = user[0].w;

    float gameU = 1280.0 / resolutionX;
    if (i.uv.x >= gameU)
    {
        return float4(0, 0, 0, 0);
    }

    float tail = 1.0 - smoothstep(0.88, 1.0, progress);

    float2 texel = float2(1.0 / resolutionX, 1.0 / resolutionY);

    float cutY = progress * resolutionY;

    float n1 = FBM(float2(i.uv.x * 8.0, i.uv.y * 14.0) + float2(time * 0.22, -time * 0.17));
    float n2 = FBM(float2(i.uv.x * 24.0, i.uv.y * 18.0) + float2(-time * 0.31, time * 0.12));
    float n3 = FBM(float2(i.uv.x * 60.0, i.uv.y * 42.0) + float2(time * 0.10, time * 0.08));
    float wobble = (n1 - 0.5) * edgePx * noiseAmt * 1.10 + (n2 - 0.5) * edgePx * noiseAmt * 0.70;

    float yPx = i.uv.y * resolutionY + wobble;

    float edgeCore = edgePx * 0.65;
    float bandPx = edgePx * 4.2;

    float m = smoothstep(cutY + edgeCore, cutY - edgeCore, yPx);

    float dist = abs(yPx - cutY);
    float band = 1.0 - saturate(dist / bandPx);
    band = Soft(band);

    float2 warp = float2((n2 - 0.5), (n1 - 0.5)) * texel * (22.0 * band * fxStr * tail);
    float2 uv2 = i.uv + warp;

    float3 col = gTex.Sample(gSamp, uv2).rgb;

    float3 graded = ColdGrade(col);
    col = lerp(col, graded, saturate((0.55 + 0.45 * band) * fxStr * tail));

    float frost = FBM(float2(i.uv.x * 28.0, i.uv.y * 34.0) + float2(time * 0.09, time * 0.06));
    frost = smoothstep(0.30, 0.95, frost);

    float shard = FBM(float2(i.uv.x * 75.0, i.uv.y * 55.0) + float2(-time * 0.15, time * 0.11));
    shard = smoothstep(0.74, 0.99, shard) * band;

    float ring = sin((i.uv.y * 9.0 + time * 1.6) * 6.28318);
    ring = (ring * 0.5 + 0.5);
    ring = smoothstep(0.65, 1.0, ring) * band;

    float spark = FBM(float2(i.uv.x * 140.0, i.uv.y * 95.0) + float2(time * 0.9, -time * 0.7));
    spark = smoothstep(0.84, 0.99, spark) * band;

    float glow = band * (0.55 + 0.75 * frost) + shard * 1.05 + ring * 0.55 + spark * 0.95;
    glow *= fxStr * tail;

    float3 iceA = float3(0.18, 0.55, 1.10);
    float3 iceB = float3(0.55, 0.92, 1.35);
    float3 iceC = float3(0.35, 0.25, 0.95);
    float3 iceGlow = lerp(iceA, iceB, frost);
    iceGlow = lerp(iceGlow, iceC, ring * 0.25);

    col += iceGlow * glow * 0.95;

    float haze = (n3 - 0.5) * 0.16 * band * fxStr * tail;
    col += float3(0.10, 0.22, 0.40) * haze;

    float aBand = band * (0.55 + 0.25 * spark) * fxStr * tail;
    float aOut = saturate(m + aBand);

    return float4(saturate(col), aOut);
}
