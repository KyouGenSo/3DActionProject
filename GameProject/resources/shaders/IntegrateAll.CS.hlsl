#include "Particle.hlsli"
#include "ForceField.hlsli"
#include "PhysicsParams.hlsli"

// リソースバインディング
RWStructuredBuffer<Particle> gParticles : register(u0);
RWStructuredBuffer<int> gFreeListIndex : register(u1);
RWStructuredBuffer<uint> gFreeList : register(u2);

StructuredBuffer<ForceField> gForceFields : register(t0);

ConstantBuffer<PerFrame> gPerFrame : register(b0);
ConstantBuffer<PhysicsParams> gPhysicsParams : register(b1);

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint particleIndex = DTid.x;

    // 有効な範囲のパーティクルのみ処理
    if (particleIndex >= kMaxParticles)
    {
        return;
    }

    // アルファ値が 0 より大きいアクティブなパーティクルのみ更新
    if (!(gParticles[particleIndex].startColor.a > 0.0f && gParticles[particleIndex].endColor.a > 0.0f))
    {
        return;
    }

    // 現在の状態を読み取り
    float3 currentPos = gParticles[particleIndex].translate;
    float3 prevPos = gParticles[particleIndex].prevPosition;
    float dt = gPerFrame.deltaTime;

    // --- 加速度の計算 ---
    float3 acceleration = float3(0.0f, 0.0f, 0.0f);

    // フォースフィールドの評価
    uint forceFieldCount = gPhysicsParams.activeForceFieldCount;
    for (uint i = 0; i < forceFieldCount; i++)
    {
        acceleration += EvaluateForceField(gForceFields[i], currentPos);
    }

    // --- Verlet 積分 ---
    // newPos = 2 * currentPos - prevPos + acceleration * dt^2
    // 速度は暗黙的に (currentPos - prevPos) / dt で導出
    float3 velocity = currentPos - prevPos;

    // 速度減衰の適用
    velocity *= gPhysicsParams.damping;

    // 新しい位置を計算
    float3 newPos = currentPos + velocity + acceleration * dt * dt;

    // 位置の更新（Verlet 積分）
    gParticles[particleIndex].prevPosition = currentPos;
    gParticles[particleIndex].translate = newPos;

    // 明示的な速度も更新（描画やエミッション参照用）
    gParticles[particleIndex].velocity = (newPos - currentPos) / max(dt, 0.0001f);

    // --- 寿命管理 ---
    // 経過時間の更新
    gParticles[particleIndex].currentTime += dt;

    // 寿命に基づいてアルファ値を計算
    float alpha = 1.0f - (gParticles[particleIndex].currentTime / gParticles[particleIndex].lifeTime);
    gParticles[particleIndex].startColor.a = saturate(alpha);
    gParticles[particleIndex].endColor.a = saturate(alpha);

    // 寿命切れならフリーリストに戻す
    if (alpha <= 0.0f)
    {
        gParticles[particleIndex].startColor.a = 0.0f;
        gParticles[particleIndex].endColor.a = 0.0f;
        gParticles[particleIndex].scale = float3(0.0f, 0.0f, 0.0f);

        // フリーリストに追加
        int freeListIndex;
        InterlockedAdd(gFreeListIndex[0], 1, freeListIndex);

        // 範囲チェック
        if (freeListIndex >= 0 && (freeListIndex + 1) < kMaxParticles)
        {
            gFreeList[freeListIndex + 1] = particleIndex;
        }
        else
        {
            // エラーケースの処理
            InterlockedAdd(gFreeListIndex[0], -1);
        }
    }
}
