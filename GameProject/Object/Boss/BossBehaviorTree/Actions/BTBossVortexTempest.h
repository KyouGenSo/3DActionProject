#pragma once
#include "../../../../BehaviorTree/Core/BTNode.h"
#include "../../../../BehaviorTree/Core/BTBlackboard.h"
#include "Decal.h"
#include "ParticleStruct.h"
#include "Vector3.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace Tako {
    class ForceFieldManager;
    class EmitterManager;
}

class Boss;

/// <summary>
/// ボス Phase2 新攻撃：ヴォルテックス・テンペスト
/// </summary>
/// <remarks>
/// ボスの周りを公転する 4 つの「渦点」を生成し、各渦点で 3 つの ForceField プリセット
/// （Attract / Directional / Vortex）を重ねて 1 本の合成トルネードを作る。
/// 4 本のトルネードが同時にボス周囲を公転して空間を制圧する。
///
/// 各プリセットの役割:
///   - Attract:     周囲のプレイヤー本体・弾を渦中心へ巻き込む
///   - Directional: 上向き気流（粒子のみに作用 / affectMask=0 の想定）
///   - Vortex:      鉛直軸まわりに旋回させる
///
/// プリセットの strength / radius / falloff / affectMask を尊重し、
/// 本ノードはランタイムで position（4 渦点に上書き）と strength（フェーズ係数）のみ上書きする。
/// 合計 ForceField 数 = 4 渦点 × 3 プリセット = **12 個**（kMaxForceFields=64 内）。
///
/// 内部フェーズ（既定パラメータでは 7 秒の決め技）:
///   Phase 0 (0〜warningTime_)               予兆: 強度ゼロ / マーカーのみ
///   Phase 1 (warningTime_ 〜 +expandTime_)  展開: 強度を 0 → preset 値へランプアップ
///   Phase 2 (+expandTime_ 〜 +sustainTime_) 持続: 公転位置更新 / 巻き込み / DoT
///   Phase 3 (+sustainTime_ 〜 +decayTime_)  終息: 強度をランプダウン
///   Phase 4                                  終了: 12 力場を逆順削除 / エミッター停止
///
/// インデックス安定性: 12 個の力場を Add / 逆順で Remove するため、
/// 本ノード実行中に他系統が ForceField を Add/Remove しないことが前提。
/// （BTParallel と組み合わせる場合、並列子は ForceField を直接操作しない既存攻撃のみとする）
/// </remarks>
class BTBossVortexTempest : public BTNode {
public:
    /// <summary>同時公転する渦点の数（仕様固定 = 4）</summary>
    static constexpr int kVortexCount = 4;

    /// <summary>1 本の渦点を構成する ForceField プリセットの数（Attract + Dir + Vortex = 3）</summary>
    static constexpr int kFieldsPerVortex = 3;

    /// <summary>preset slot のインデックス意味付け（コード内部で使用）</summary>
    enum FieldSlot {
        kSlotAttract = 0,
        kSlotDirectional = 1,
        kSlotVortex = 2,
    };

    BTBossVortexTempest();
    virtual ~BTBossVortexTempest() = default;

    /// <summary>ノードの実行</summary>
    BTNodeStatus Execute(BTBlackboard* blackboard) override;

    /// <summary>ノードのリセット（中断時の力場・エミッター後始末も含む）</summary>
    void Reset() override;

    /// <summary>JSON からパラメータを適用</summary>
    void ApplyParameters(const nlohmann::json& params) override;

    /// <summary>パラメータを JSON として抽出</summary>
    [[nodiscard]] nlohmann::json ExtractParameters() const override;

#ifdef _DEBUG
    /// <summary>ImGui でパラメータ編集 UI を描画</summary>
    bool DrawImGui() override;
#endif

    /// <summary>総所要時間</summary>
    [[nodiscard]] float GetTotalDuration() const {
        return warningTime_ + expandTime_ + sustainTime_ + decayTime_;
    }

private:
    /// <summary>ForceField 12 個を逆順削除 / エミッター停止 / 状態フラグ初期化</summary>
    void Cleanup();

    /// <summary>i 番目の渦エミッター名を生成（vortexEmitterBaseName_ + "_" + i）</summary>
    [[nodiscard]] std::string MakeVortexEmitterName(int i) const;

    //======================== パラメータ ========================
    float warningTime_ = 1.5f;          ///< 予兆時間
    float expandTime_ = 0.5f;            ///< 強度ランプアップ時間
    float sustainTime_ = 4.5f;           ///< 持続時間（フル稼働）
    float decayTime_ = 0.5f;             ///< 終息時間（ランプダウン）

    float orbitRadius_ = 8.0f;           ///< ボス → 渦点の距離（公転半径）
    float angularSpeed_ = 0.6f;          ///< 公転角速度（rad/秒）

    float minDoT_ = 5.0f;                ///< 巻き込み外周時の DoT（damage / 秒）
    float maxDoT_ = 15.0f;               ///< 中心時の DoT（damage / 秒）

    /// <summary>
    /// DoT を Player::OnHit に適用する間隔（秒）。
    /// 毎フレーム OnHit を呼ぶと HitFlash/Shake/CameraShake/Vibration/Vignette などの
    /// 副作用が連打されてゲーム体験が破綻するため、累積バッファ + Tick 方式で間引く。
    /// </summary>
    float damageTickInterval_ = 0.5f;

    //======================== ForceField プリセット名 ========================
    std::string attractPresetName_ = "boss_vortexattacck_attract";
    std::string dirPresetName_ = "boss_vortexattacck_dir";
    std::string vortexPresetName_ = "boss_vortexattack_vortex";

    //======================== エミッター名 ========================
    /// <summary>渦エミッターのベース名（実際のエミッター名は base + "_0".."_3"）</summary>
    std::string vortexEmitterBaseName_ = "boss_vortex";

    //======================== デカール演出（地面マーカー）========================
    /// <summary>持続フェーズの基本アルファ値（0..1）</summary>
    static constexpr float kDecalBaseAlpha = 0.5f;
    /// <summary>予兆フェーズの点滅アルファ最小値</summary>
    static constexpr float kBlinkAlphaMin = 0.15f;
    /// <summary>予兆フェーズの点滅アルファ振幅（kBlinkAlphaMin + 振幅 * |sin(...)| が最大）</summary>
    static constexpr float kBlinkAlphaAmplitude = 0.55f;

    /// <summary>予兆フェーズのアルファ点滅周波数（Hz）</summary>
    float markerBlinkFrequency_ = 4.0f;

    //======================== ランタイム状態 ========================
    float elapsedTime_ = 0.0f;
    bool isFirstExecute_ = true;

    /// <summary>プリセットから読み込んだ ForceField 1 件分のランタイム情報</summary>
    struct LoadedField {
        Tako::ForceFieldData base{};   ///< プリセット値（strength は base 値として保持）
        int32_t index = -1;            ///< AddForceField で取得したインデックス（-1 = 未登録）
    };
    /// <summary>
    /// 4 渦点 × 3 スロット = 12 個の ForceField を二次元で管理。
    /// loadedFields_[v][s] : v 番目の渦点における s スロット (Attract/Dir/Vortex)
    /// </summary>
    LoadedField loadedFields_[kVortexCount][kFieldsPerVortex];

    /// <summary>
    /// DoT 累積バッファ（巻き込み中に dotPerSec * dt を加算）。
    /// damageTickInterval_ ごとに Player::OnHit でまとめて適用し、ゼロクリア。
    /// </summary>
    float pendingDamage_ = 0.0f;
    /// <summary>DoT Tick 用タイマー（damageTickInterval_ を超えたら適用 + リセット）</summary>
    float damageTickTimer_ = 0.0f;

    /// <summary>
    /// 4 渦点それぞれに 1 個ずつ貼る地面マーカーデカール。
    /// Decal::Initialize() 内で DecalManager::AddDecal() が呼ばれ、
    /// unique_ptr::reset() / array 解体で自動的に DecalManager から外れる
    /// （DecalManager は std::list ベースなので erase 順制約なし）。
    /// </summary>
    std::array<std::unique_ptr<Tako::Decal>, kVortexCount> vortexDecals_{};

    //======================== Reset 用キャッシュ ========================
    Tako::ForceFieldManager* cachedForceFieldManager_ = nullptr;
    Tako::EmitterManager* cachedEmitterManager_ = nullptr;
    /// <summary>
    /// Reset 経由（BTParallel が子ノードを中断するケース含む）でも Boss::ExitRecovery を
    /// 確実に呼ぶためのキャッシュ。Reset() からは Blackboard が取れないため Boss* を保持する。
    /// </summary>
    Boss* cachedBoss_ = nullptr;
    /// <summary>EnterRecovery 済みか（多重 ExitRecovery と未呼び出しの両方を防ぐガード）</summary>
    bool enteredRecovery_ = false;
};
