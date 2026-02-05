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
        p = p * 2.02 + 19.3;
        a *= 0.5;
    }
    return v;
}

float Soft(float x)
{
    x = saturate(x);
    return x * x * (3.0 - 2.0 * x);
}

float3 WarmGrade(float3 c)
{
    float lum = dot(c, float3(0.299, 0.587, 0.114));
    float3 warm = lerp(float3(1.15, 0.85, 0.70), float3(1.30, 1.00, 0.78), lum);
    return saturate(c * warm);
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

    float n1 = FBM(float2(i.uv.x * 12.0, i.uv.y * 28.0) + float2(time * 0.85, -time * 0.62));
    float n2 = FBM(float2(i.uv.x * 32.0, i.uv.y * 22.0) + float2(-time * 0.55, time * 0.28));
    float n3 = FBM(float2(i.uv.x * 80.0, i.uv.y * 55.0) + float2(time * 0.25, -time * 0.20));
    float wobble = (n1 - 0.5) * edgePx * noiseAmt * 1.45 + (n2 - 0.5) * edgePx * noiseAmt * 0.80;

    float yPx = i.uv.y * resolutionY + wobble;

    float edgeCore = edgePx * 0.70;
    float bandPx = edgePx * 4.6;

    float m = smoothstep(cutY + edgeCore, cutY - edgeCore, yPx);

    float dist = abs(yPx - cutY);
    float band = 1.0 - saturate(dist / bandPx);
    band = Soft(band);

    float heat = FBM(float2(i.uv.x * 18.0, i.uv.y * 20.0) + float2(time * 1.25, time * 0.35));
    float2 uv2 = i.uv + float2((heat - 0.5) * texel.x * (34.0 * band * fxStr * tail), 0.0);

    float3 col = gTex.Sample(gSamp, uv2).rgb;

    float3 graded = WarmGrade(col);
    col = lerp(col, graded, saturate((0.55 + 0.45 * band) * fxStr * tail));

    float flame = FBM(float2(i.uv.x * 28.0, i.uv.y * 46.0) + float2(time * 1.15, -time * 0.78));
    flame = smoothstep(0.28, 0.95, flame);

    float sparks = FBM(float2(i.uv.x * 150.0, i.uv.y * 110.0) + float2(-time * 1.55, time * 1.05));
    sparks = smoothstep(0.86, 0.995, sparks) * band;

    float pulse = sin((i.uv.y * 8.5 - time * 2.1) * 6.28318);
    pulse = (pulse * 0.5 + 0.5);
    pulse = smoothstep(0.62, 1.0, pulse) * band;

    float glow = band * (0.70 + 0.85 * flame) + sparks * 1.15 + pulse * 0.65;
    glow *= fxStr * tail;

    float3 fireHot = float3(1.20, 0.68, 0.14);
    float3 fireDeep = float3(0.95, 0.18, 0.05);
    float3 fireGold = float3(1.35, 1.05, 0.40);
    float3 fireGlow = lerp(fireDeep, fireHot, flame);
    fireGlow = lerp(fireGlow, fireGold, pulse * 0.25);

    col += fireGlow * glow * 1.10;
    col += fireGold * sparks * (1.05 * fxStr * tail);

    float smoke = (n3 - 0.5) * 0.14 * band * fxStr * tail;
    col += float3(0.20, 0.06, 0.02) * smoke;

    float aBand = band * (0.60 + 0.30 * sparks) * fxStr * tail;
    float aOut = saturate(m + aBand);

    return float4(saturate(col), aOut);
}
