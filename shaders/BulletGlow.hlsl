// --- BulletPhantom.hlsl ---
Texture2D gTex : register(t0);
SamplerState gSamp : register(s0);

cbuffer CommonParams : register(b0)
{
    float time;
    float deltaTime;
    float combo;
    float intensity; // 全局强度
    float2 resolution;
    float2 pad;
    // user[0].x = Ghost Radius (残影偏移距离, 1.0 ~ 3.0)
    // user[0].y = Ghost Speed (残影旋转/呼吸速度, 2.0 ~ 6.0)
    // user[0].z = Core Pulse (核心亮度脉动幅度, 0.1 ~ 0.3)
    float4 user[16];
};

struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};

float4 main(PS_IN input) : SV_TARGET
{
    float2 texelSize = 1.0f / resolution;
    
    // 参数提取
    float radius = user[0].x;
    float speed = user[0].y;
    float pulseAmt = user[0].z;
    
    // 1. 采样原始核心 (Base)
    float4 coreCol = gTex.Sample(gSamp, input.uv);
    
    // 2. 制作动态残影 (Ghosts)
    // 我们生成两个“虚影”，它们以相反的方向围绕中心微小旋转/浮动
    // 这模拟了视觉残留或能量溢出
    
    float2 offset1 = float2(cos(time * speed), sin(time * speed)) * radius * texelSize;
    float2 offset2 = float2(cos(time * speed * 0.8 + 3.14), sin(time * speed * 0.8 + 3.14)) * radius * texelSize;
    
    // 采样虚影，透明度减半
    float4 ghost1 = gTex.Sample(gSamp, input.uv + offset1);
    float4 ghost2 = gTex.Sample(gSamp, input.uv + offset2);
    
    // 3. 合成虚影
    // 虚影颜色稍微提亮一点饱和度，模拟辉光
    float4 ghostTotal = (ghost1 + ghost2) * 0.5f; // 平均
    
    // 4. 核心动态加强 (Core Dynamic)
    // 让原本的子弹亮度随时间微弱呼吸，模拟头部能量聚集
    float coreBreath = 1.0f + sin(time * 10.0f) * pulseAmt;
    
    // 5. 最终混合
    // 逻辑：核心颜色 + (虚影颜色 * 衰减系数)
    // 这样核心永远清晰，周围有一圈动态的光晕在转
    float4 finalCol = coreCol * coreBreath;
    
    // 叠加虚影 (使用 Additive 叠加，但强度由 intensity 控制)
    // ghostTotal.a * 0.5f 意味着虚影是不透明度的一半，不会太抢眼
    finalCol += ghostTotal * intensity * 0.6f;
    
    // 6. Alpha 处理
    // 这一步很重要，防止虚影在透明背景上产生黑边
    // 最终 alpha 应该是核心和虚影 alpha 的最大值或总和
    float combinedAlpha = saturate(coreCol.a + ghostTotal.a * 0.5f);
    finalCol.a = combinedAlpha;
    
    // 简单的防止过曝保护 (可选)
    // finalCol.rgb = min(finalCol.rgb, float3(1.5, 1.5, 1.5));

    return finalCol;
}