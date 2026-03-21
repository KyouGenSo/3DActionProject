static const int kMaxParticles = 1000000;

// �G�~�b�^�[�^�C�v�̒�`
#define EMITTER_TYPE_SPHERE 0
#define EMITTER_TYPE_BOX 1
#define EMITTER_TYPE_TRIANGLE 2

struct VertexShaderOutput
{
    float4 pos : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float4 color : COLOR0;
};

struct Particle
{
    float3 translate;      // 現在位置
    float3 prevPosition;   // 前フレーム位置（Verlet積分用）
    float3 scale;          // スケール
    float3 rotate;         // 回転（オイラー角）
    float3 velocity;       // 速度ベクトル
    float4 startColor;     // 開始色（RGBA）
    float4 endColor;       // 終了色（RGBA）
    float lifeTime;        // 寿命（秒）
    float currentTime;     // 経過時間（秒）
    float mass;            // 質量（衝突応答用）
    uint  cellIndex;       // 空間ハッシュ用セルインデックス
};

// �G�~�b�^�[���ʍ\����
struct Emitter
{
    // ��{���
    uint type;                // �G�~�b�^�[�^�C�v
    uint isActive;            // �A�N�e�B�u��ԁi1=�L���A0=�����j
    uint isEmit;              // �ˏo�t���O�i1=�ˏo����A0=�ˏo���Ȃ��j
    uint isNormalize;         // ���K���t���O
    uint isRandomRotateZ; // Z�������_����]�t���O�i1=�����_���A0=�Œ��]�j
    uint emitterID;           // �G�~�b�^�[ID
    
    float3 position;          // ���S/��ʒu
    float2 scaleRangeX;       // �p�[�e�B�N����X�T�C�Y�͈́i�ŏ��A�ő�j
    float2 scaleRangeY;       // �p�[�e�B�N����Y�T�C�Y�͈́i�ŏ��A�ő�j
    float2 velRangeX;         // �p�[�e�B�N����X���x�͈́i�ŏ��A�ő�j
    float2 velRangeY;         // �p�[�e�B�N����Y���x�͈́i�ŏ��A�ő�j
    float2 velRangeZ;         // �p�[�e�B�N����Z���x�͈́i�ŏ��A�ő�j
    float2 lifeTimeRange;     // �p�[�e�B�N���̎����͈́i�ŏ��A�ő�j
    float4 startColorTint;    // �p�[�e�B�N���̊J�n�F�iRGBA�j
    float4 endColorTint;      // �p�[�e�B�N���̏I���F�iRGBA�j
    
    uint  count;              // 1��̎ˏo�Ő�������p�[�e�B�N����
    float frequency;          // �ˏo�p�x�i�b�j
    float frequencyTime;      // �o�ߎ��ԃJ�E���^�[

    uint isTemp;              // �ꎞ�I�ȃG�~�b�^�[���ǂ����i1=�ꎞ�I�A0=�i���j
    float emitterLifeTime;    // �G�~�b�^�[�̎����i�b�j
    float emitterCurrentTime; // �G�~�b�^�[�̌o�ߎ���

    // ���̗p�p�����[�^
    float radius;             // ���̂̔��a
    
    // ���^�p�p�����[�^
    float3 boxSize;           // ���̑傫���i���A�����A���s���j
    float3 boxRotation;       // ���̉�]�iX,Y,Z���A�x���@�j
    
    // �O�p�`�p�p�����[�^
    float3 triangleV1;        // �O�p�`�̒��_1�i���΍��W�j
    float3 triangleV2;        // �O�p�`�̒��_2�i���΍��W�j
    float3 triangleV3;        // �O�p�`�̒��_3�i���΍��W�j
};

// �p�[�t���[�����\����
struct PerFrame
{
    float time;                 // ����
    float deltaTime;            // �f���^�^�C��
    uint activeEmitterCount;    // �A�N�e�B�u�ȃG�~�b�^�[��
    uint pad;                   // �p�f�B���O
};

// PerView���\����
struct PerView
{
    float4x4 viewProj;          // �r���[�v���W�F�N�V�����s��
    float4x4 billboardMat;      // �r���{�[�h�s��
};