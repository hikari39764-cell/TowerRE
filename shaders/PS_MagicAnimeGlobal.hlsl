Texture2D gTex : register(t0);
SamplerState gSamp : register(s0);

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

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

static const float PI = 3.14159265;

float Hash21(float2 p)
{
    p = frac(p * float2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return frac(p.x * p.y);
}

float Luma(float3 c)
{
    return dot(c, float3(0.299, 0.587, 0.114));
}

float3 Lerp3(float3 a, float3 b, float t)
{
    return a + (b - a) * t;
}

float3 HueShift(float3 c, float a)
{
    float3 k = float3(0.57735, 0.57735, 0.57735);
    float cs = cos(a), sn = sin(a);
    return c * cs + cross(k, c) * sn + k * dot(k, c) * (1.0 - cs);
}

float3 SoftGlow(float2 uv, float2 texel)
{
    float3 c0 = gTex.Sample(gSamp, uv).rgb;
    float3 c1 = gTex.Sample(gSamp, uv + texel * float2(1, 0)).rgb;
    float3 c2 = gTex.Sample(gSamp, uv + texel * float2(-1, 0)).rgb;
    float3 c3 = gTex.Sample(gSamp, uv + texel * float2(0, 1)).rgb;
    float3 c4 = gTex.Sample(gSamp, uv + texel * float2(0, -1)).rgb;

    float l = max(Luma(c0), max(max(Luma(c1), Luma(c2)), max(Luma(c3), Luma(c4))));
    float glow = saturate((l - 0.65) / 0.35);
    glow = glow * glow;

    float3 tint = Lerp3(float3(1.0, 0.25, 0.75), float3(0.55, 0.25, 1.0), 0.5);
    return tint * glow;
}

float ValueNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    float2 u = f * f * (3.0 - 2.0 * f);

    float a = Hash21(i + float2(0, 0));
    float b = Hash21(i + float2(1, 0));
    float c = Hash21(i + float2(0, 1));
    float d = Hash21(i + float2(1, 1));

    float x1 = lerp(a, b, u.x);
    float x2 = lerp(c, d, u.x);
    return lerp(x1, x2, u.y);
}

float FogNoise(float2 uv, float t)
{
    float2 p = uv * float2(1.6, 1.1);

    float2 w1 = float2(
        ValueNoise(p * 2.0 + float2(t * 0.06, t * 0.04)),
        ValueNoise(p * 2.0 + float2(-t * 0.05, t * 0.03))
    );
    p += (w1 - 0.5) * 0.22;

    float n0 = ValueNoise(p * 2.0 + float2(t * 0.06, t * 0.04));
    float n1 = ValueNoise(p * 4.0 + float2(-t * 0.04, t * 0.05));
    float n2 = ValueNoise(p * 8.0 + float2(t * 0.03, -t * 0.06));

    float n = n0 * 0.62 + n1 * 0.28 + n2 * 0.10;
    n = smoothstep(0.35, 0.92, n);

    return n;
}

float4 main(PS_IN i) : SV_TARGET
{
    // -------- 原图--------
    float3 baseCol = gTex.Sample(gSamp, i.uv).rgb;

    // -------- 1280x1000 区域 mask（左上角为原点）--------
    float2 px = i.uv * float2(resolutionX, resolutionY);
    float areaMask = step(px.x, 1280.0) * step(px.y, 1000.0);

    // -------- 参数 --------
    float jitterPx = user[0].x;
    float waveAmpPx = user[0].y;
    float waveFreq = user[0].z;
    float gradeInt = user[0].w;

    float hueAmt = user[1].x;
    float grainInt = user[1].y;
    float glowInt = user[1].z;
    float vignette = user[1].w;

    float2 texel = float2(1.0 / resolutionX, 1.0 / resolutionY);

    // -------- 抖动 + 波形偏移（照常计算，但最后用 mask 混合）--------
    float n = Hash21(float2(time * 60.0, i.uv.y * 200.0));
    float2 jitter = (n - 0.5) * jitterPx * texel;

    float wave = sin((i.uv.y * 2.0 * PI * waveFreq) + time * 2.0) * waveAmpPx;
    float2 waveOff = float2(wave, 0.0) * texel;

    float2 uv = i.uv + jitter + waveOff;
    uv = clamp(uv, texel * 0.5, 1.0 - texel * 0.5);

    float3 col = gTex.Sample(gSamp, uv).rgb;

    // -------- 轻微提亮/伽马 --------
    col = pow(col, 0.92);

    // -------- 分级调色 --------
    float lum = Luma(col);
    float3 shadowTint = float3(0.20, 0.08, 0.30);
    float3 highTint = float3(1.00, 0.25, 0.75);

    float3 graded = Lerp3(shadowTint, highTint, smoothstep(0.05, 0.95, lum));

    float gradeK = gradeInt * 0.55;
    col = Lerp3(col, col * graded, gradeK);

    // -------- HueShift --------
    float h = sin(time * 1.2) * hueAmt;
    col = HueShift(col, h * 0.60);

    // -------- SoftGlow --------
    float3 glow = SoftGlow(uv, texel);
    col += glow * (glowInt * 1.85);

    // -------- Grain --------
    float gr = Hash21(uv * resolutionX + time * 13.7);
    float hi = smoothstep(0.60, 0.95, lum);
    col += (gr - 0.5) * grainInt * 0.45 * (1.0 - 0.65 * hi);

    // -------- 呼吸式轻微增亮 --------
    float breath = 1.5 + 0.5 * sin(time * 0.9 + i.uv.y * 3.0);
    col *= 1.0 + (breath - 0.5) * 0.035 * saturate(intensity);

    // -------- Fog--------
    float bottom = smoothstep(0.42, 1.00, i.uv.y);
    float fn = FogNoise(i.uv, time);

    float combo01 = saturate(combo / 5.0);
    float fogInt = (0.040 + 0.060 * saturate(intensity) + 0.035 * combo01) * areaMask * bottom;

    float clump = smoothstep(0.48, 0.88, fn);
    float fogA = fogInt * (0.55 + 0.55 * clump);

    float3 fogTint = float3(0.24, 0.17, 0.30);
    float3 fogAdd = lerp(float3(1.0, 1.0, 1.0), fogTint, 0.30 + 0.20 * clump);

    col += fogAdd * fogA * 0.85;

    // -------- 最终小增强 --------
    col *= 1.03;

    // -------- Vignette--------
    float2 p = i.uv * 2.0 - 1.0;
    float v = dot(p, p);
    col *= 1.0 - vignette * v;

    float3 finalCol = lerp(baseCol, col, areaMask);
    return float4(saturate(finalCol), 1.0);
}
