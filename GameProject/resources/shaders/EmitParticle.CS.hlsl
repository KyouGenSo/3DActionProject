#include "Particle.hlsli"
#include "Random.hlsli"

// ��]�s��̌v�Z
float3x3 CalculateRotationMatrix(float3 eulerAngles)
{
    // �x�����烉�W�A���ɕϊ�
    float3 rad = eulerAngles * (3.14159265f / 180.0f);
    
    // X����]�s��
    float3x3 rotX = float3x3(
        1.0f, 0.0f, 0.0f,
        0.0f, cos(rad.x), -sin(rad.x),
        0.0f, sin(rad.x), cos(rad.x)
    );
    
    // Y����]�s��
    float3x3 rotY = float3x3(
        cos(rad.y), 0.0f, sin(rad.y),
        0.0f, 1.0f, 0.0f,
        -sin(rad.y), 0.0f, cos(rad.y)
    );
    
    // Z����]�s��
    float3x3 rotZ = float3x3(
        cos(rad.z), -sin(rad.z), 0.0f,
        sin(rad.z), cos(rad.z), 0.0f,
        0.0f, 0.0f, 1.0f
    );
    
    // �s��̍����iZ��Y��X���j
    return mul(mul(rotZ, rotY), rotX);
}

// ���̓��̃����_���ȓ_�𐶐�
float3 GetRandomPointInSphere(RandomGenerator generator, float3 center, float radius)
{
    // �����x�N�g���̐���
    float3 dir = generator.Generate3d() * 2.0f - 1.0f;
    float len = length(dir);
    
    // �[�����Z�h�~
    if (len < 0.0001f)
    {
        dir = float3(0.0f, 1.0f, 0.0f);
        len = 1.0f;
    }
    
    // ���K��
    dir /= len;
    
    // ���̓��̋ψꕪ�z�̂��߂̃X�P�[�����O�i�̐ςɔ��j
    float r = radius * pow(generator.Generate1d(), 1.0f / 3.0f);
    
    return center + dir * r;
}

// �����̃����_���ȓ_�𐶐�
float3 GetRandomPointInBox(RandomGenerator generator, float3 center, float3 size, float3 rotation)
{
    // ���[�J�����W���̃����_���ȓ_
    float3 localPoint = (generator.Generate3d() - 0.5f) * size;
    
    // ��]�s��̓K�p
    float3x3 rotMatrix = CalculateRotationMatrix(rotation);
    float3 rotatedPoint = mul(rotMatrix, localPoint);
    
    return center + rotatedPoint;
}

// �O�p�`��̃����_���ȓ_�𐶐�
float3 GetRandomPointOnTriangle(RandomGenerator generator, float3 p0, float3 p1, float3 p2)
{
    // �o���Z���g���b�N���W��p�����O�p�`��̃����_���ȓ_
    float r1 = generator.Generate1d();
    float r2 = generator.Generate1d();
    
    // �O�p�`��̋ψꕪ�z���m��
    if (r1 + r2 > 1.0f)
    {
        r1 = 1.0f - r1;
        r2 = 1.0f - r2;
    }
    
    float a = 1.0f - r1 - r2;
    float b = r1;
    float c = r2;
    
    return a * p0 + b * p1 + c * p2;
}

RWStructuredBuffer<Particle> gParticles : register(u0); // �p�[�e�B�N���o�b�t�@
RWStructuredBuffer<int> gFreeListIndex : register(u1);  // �t���[���X�g�C���f�b�N�X
RWStructuredBuffer<uint> gFreeList : register(u2);      // �t���[���X�g
StructuredBuffer<Emitter> gEmitters : register(t0);     // �G�~�b�^�[���X�g

ConstantBuffer<PerFrame> gPerFrame : register(b0);      // �t���[�����

[numthreads(16, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    // �G�~�b�^�[�̃C���f�b�N�X�v�Z
    uint emitterIndex = DTid.x;
    
    // �͈̓`�F�b�N
    if (emitterIndex >= gPerFrame.activeEmitterCount)
    {
        return;
    }
    
    // ���̃G�~�b�^�[���A�N�e�B�u���`�F�b�N
    if (gEmitters[emitterIndex].isActive == 0)
    {
        return;
    }
    
    // �ˏo�t���O�������Ă��邩�`�F�b�N
    if (gEmitters[emitterIndex].isEmit == 0)
    {
        return;
    }
    
    // ����������̏�����
    RandomGenerator generator;
    // �V�[�h�l���� (���ԁ{�G�~�b�^�[ID�{�X���b�hID�j
    generator.seed = float3(
        DTid.x + gPerFrame.time * 0.1f,
        DTid.y + gPerFrame.time * 0.2f,
        gEmitters[emitterIndex].emitterID + gPerFrame.time * 0.3f
    );
    
    // ���̃G�~�b�^�[����w�萔�̃p�[�e�B�N�����ˏo
    for (uint particleIndex = 0; particleIndex < gEmitters[emitterIndex].count; ++particleIndex)
    {
        // FreeList����󂫃p�[�e�B�N���X���b�g���擾
        int freeListIndex;
        InterlockedAdd(gFreeListIndex[0], -1, freeListIndex);
        
        // �L���ȃC���f�b�N�X���\���ȋ󂫂����邩���m�F
        if (freeListIndex >= 0 && freeListIndex < kMaxParticles && gFreeList[freeListIndex] < kMaxParticles)
        {
            // FreeList������ۂ̃p�[�e�B�N��ID���擾
            uint particleID = gFreeList[freeListIndex];
            
            // �p�[�e�B�N���̈ʒu������i�G�~�b�^�[�`��ɉ����āj
            float3 particlePosition;
            
            switch (gEmitters[emitterIndex].type)
            {
                case EMITTER_TYPE_SPHERE:
                    particlePosition = GetRandomPointInSphere(
                        generator,
                        gEmitters[emitterIndex].position,
                        gEmitters[emitterIndex].radius
                    );
                    break;
                    
                case EMITTER_TYPE_BOX:
                    particlePosition = GetRandomPointInBox(
                        generator,
                        gEmitters[emitterIndex].position,
                        gEmitters[emitterIndex].boxSize,
                        gEmitters[emitterIndex].boxRotation
                    );
                    break;
                    
                case EMITTER_TYPE_TRIANGLE:
                    particlePosition = GetRandomPointOnTriangle(
                        generator,
                        gEmitters[emitterIndex].position + gEmitters[emitterIndex].triangleV1,
                        gEmitters[emitterIndex].position + gEmitters[emitterIndex].triangleV2,
                        gEmitters[emitterIndex].position + gEmitters[emitterIndex].triangleV3
                    );
                    break;
                    
                default:
                    // �f�t�H���g�̓G�~�b�^�[�̒��S
                    particlePosition = gEmitters[emitterIndex].position;
                    break;
            }
            
            // -----------------------------�p�[�e�B�N���̏�����----------------------------- //
            
            // �T�C�Y�ݒ�---------------------------------------------------------------------------------
            float3 particleScale;
            
            // X�������̃X�P�[��
            if (any(gEmitters[emitterIndex].scaleRangeX != float2(0.0f, 0.0f)))
            {
                particleScale.x = generator.Generate1d() * (gEmitters[emitterIndex].scaleRangeX.y - gEmitters[emitterIndex].scaleRangeX.x) + gEmitters[emitterIndex].scaleRangeX.x;
            }
            else
            {
                particleScale.x = 1.0f;
            }
            
            // Y�������̃X�P�[��
            if (any(gEmitters[emitterIndex].scaleRangeY != float2(0.0f, 0.0f)))
            {
                particleScale.y = generator.Generate1d() * (gEmitters[emitterIndex].scaleRangeY.y - gEmitters[emitterIndex].scaleRangeY.x) + gEmitters[emitterIndex].scaleRangeY.x;
            }
            else
            {
                particleScale.y = 1.0f;
            }
            
            // Z�������̃X�P�[��
            particleScale.z = 1.0f;
            
            // �p�[�e�B�N���̃X�P�[����ݒ�
            gParticles[particleID].scale.x = particleScale.x;
            gParticles[particleID].scale.y = particleScale.y;
            
            // 位置設定---------------------------------------------------------------------------------
            gParticles[particleID].translate = particlePosition;

            // 質量・セルインデックス設定--------------------------------------------------------------
            gParticles[particleID].mass = 1.0f;
            gParticles[particleID].cellIndex = 0xFFFFFFFF; // 未割り当て

        	// ��]�ݒ�---------------------------------------------------------------------------------
            if (gEmitters[emitterIndex].isRandomRotateZ > 0)
            {
                // �����_����]�t���O�������Ă���ꍇ�A�����_���ȉ�]��ݒ�
                gParticles[particleID].rotate.z = generator.Generate1d() * 360.0f; // Z����]
            }
            else
            {
                gParticles[particleID].rotate.z = 0.0f; // Z����]�Ȃ�
            }
            
            
            // ���x�ݒ�---------------------------------------------------------------------------------
            float3 particleVelocity;

            float3 randomVel = (generator.Generate3d() * 2.0f - 1.0f) * 0.1f;

            // X�������̑��x
            if (any(gEmitters[emitterIndex].velRangeX != float2(0.0f, 0.0f)))
            {
                particleVelocity.x = generator.Generate1d() * (gEmitters[emitterIndex].velRangeX.y - gEmitters[emitterIndex].velRangeX.x) + gEmitters[emitterIndex].velRangeX.x;
            }
            else
            {
                particleVelocity.x = randomVel.x;
            }

            // Y�������̑��x
            if (any(gEmitters[emitterIndex].velRangeY != float2(0.0f, 0.0f)))
            {
                particleVelocity.y = generator.Generate1d() * (gEmitters[emitterIndex].velRangeY.y - gEmitters[emitterIndex].velRangeY.x) + gEmitters[emitterIndex].velRangeY.x;
            }
			else
			{
                particleVelocity.y = randomVel.y;
            }

            // Z�������̑��x
            if (any(gEmitters[emitterIndex].velRangeZ != float2(0.0f, 0.0f)))
            {
                particleVelocity.z = generator.Generate1d() * (gEmitters[emitterIndex].velRangeZ.y - gEmitters[emitterIndex].velRangeZ.x) + gEmitters[emitterIndex].velRangeZ.x;
            }
            else
            {
                particleVelocity.z = randomVel.z;

            }

            // ���K��
            if (gEmitters[emitterIndex].isNormalize == 1)
            {
                // ���K���t���O�������Ă���ꍇ�A���x�𐳋K��
                particleVelocity = normalize(particleVelocity);
            }

            // パーティクルの速度を設定
            gParticles[particleID].velocity.x = particleVelocity.x;
            gParticles[particleID].velocity.y = particleVelocity.y;
            gParticles[particleID].velocity.z = particleVelocity.z;

            // 前フレーム位置の初期化（Verlet積分用）
            // prevPosition = translate - velocity で初速をエンコード
            gParticles[particleID].prevPosition = particlePosition - particleVelocity;


            // 寿命設定---------------------------------------------------------------------------------
            if (any(gEmitters[emitterIndex].lifeTimeRange != float2(0.0f, 0.0f)))
            {
                gParticles[particleID].lifeTime = generator.Generate1d() * (gEmitters[emitterIndex].lifeTimeRange.y - gEmitters[emitterIndex].lifeTimeRange.x) + gEmitters[emitterIndex].lifeTimeRange.x;
            }
            else
            {
                gParticles[particleID].lifeTime = 1.0f + generator.Generate1d() * 0.5f; // 1.0�`1.5�b
            }
            
            gParticles[particleID].currentTime = 0.0f;
            
            // �F�ݒ�---------------------------------------------------------------------------------
            gParticles[particleID].startColor.rgb = gEmitters[emitterIndex].startColorTint.rgb;
            gParticles[particleID].startColor.a = 1.0f;

            gParticles[particleID].endColor.rgb = gEmitters[emitterIndex].endColorTint.rgb;
            gParticles[particleID].endColor.a = 1.0f;

        }
        else
        {
            // �󂫃p�[�e�B�N��������Ȃ��ꍇ�͖߂��ďI��
            InterlockedAdd(gFreeListIndex[0], 1);
            break;
        }
    }
}