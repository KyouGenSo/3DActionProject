#pragma once
#include "AttackNode.h"
#include "Decal.h"
#include "ParticleStruct.h"
#include "Vector3.h"
#include <array>
#include <cstdint>
#include <memory>
#include <string>

namespace Tako {
    class ForceFieldManager;
}

class Boss;

/// <summary>
/// ボスの周りを公転する 4 つの「渦点」を生成し、各渦点で 3 つの ForceField プリセット
/// （Attract / Directional / Vortex）を重ねて 1 本の合成トルネードを作る。
/// </summary>
/// <remarks>
/// 内部フェーズ:
///   Phase 0 (0〜warningTime_)               予兆: 強度ゼロ / デカール点滅
///   Phase 1 (warningTime_ 〜 +expandTime_)  展開: 強度を 0 → preset 値へランプアップ
///   Phase 2 (+expandTime_ 〜 +sustainTime_) 持続: 公転位置更新 / 巻き込み / DoT
///   Phase 3 (+sustainTime_ 〜 +decayTime_)  終息: ランプダウン + Recovery 突入（隙）
///   Phase 4                                 終了: 12 力場を逆順削除 / エミッター停止
/// </remarks>
class BTBossVortexTempest : public AttackNode {
public:
    /// <summary>同時公転する渦点の数（仕様固定 = 4）</summary>
    static constexpr int kVortexCount = 4;

    /// <summary>1 本の渦点を構成する ForceField プリセットの数（Attract + Dir + Vortex = 3）</summary>
    static constexpr int kFieldsPerVortex = 3;

    /// <summary>preset slot のインデックス意味付け</summary>
    enum FieldSlot {
        kSlotAttract = 0,
        kSlotDirectional = 1,
        kSlotVortex = 2,
    };

    BTBossVortexTempest();
    virtual ~BTBossVortexTempest() = default;

    /// <summary>総所要時間</summary>
    [[nodiscard]] float GetTotalDuration() const {
        return warningTime_ + expandTime_ + sustainTime_ + decayTime_;
    }

protected:
    /// <summary>
    /// 固有攻撃ロジック本体（予兆 → 展開 → 持続 → 終息のフェーズ制御と公転渦点の更新）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    /// <param name="deltaTime">1フレームの経過時間</param>
    /// <returns>BTNodeStatus::Running（攻撃継続中） / BTNodeStatus::Success（攻撃完了）</returns>
    BTNodeStatus OnExecute(BTBlackboard* blackboard, Boss* boss, float deltaTime) override;

    /// <summary>
    /// 固有初期化処理（ForceFieldManager のキャッシュ、4 渦点 × 3 プリセットの ForceField 登録、Decal/エミッターの準備）
    /// </summary>
    /// <param name="blackboard">BTBlackboardへのポインタ</param>
    /// <param name="boss">攻撃を行うBossへのポインタ</param>
    void OnInitialize(BTBlackboard* blackboard, Boss* boss) override;

    /// <summary>
    /// 固有クリーンアップ処理（12 力場の逆順削除、エミッター停止、Decal 解放、ランタイム状態リセット）
    /// </summary>
    void OnCleanup() override;

    /// <summary>
    /// 固有のjsonパラメータ適用
    /// </summary>
    /// <param name="params">適用するjsonパラメータ</param>
    void OnApplyParameters(const nlohmann::json& params) override;

    /// <summary>
    /// 固有のjsonパラメータ抽出処理
    /// </summary>
    /// <param name="out">抽出したパラメータを格納するjsonオブジェクトへの参照</param>
    void OnExtractParameters(nlohmann::json& out) const override;
#ifdef _DEBUG
    /// <summary>
    /// 固有のImGuiデバッグ表示
    /// </summary>
    bool OnDrawImGui() override;
#endif

private:
    /// <summary>i 番目の渦エミッター名を生成（vortexEmitterBaseName_ + "_" + i）</summary>
    [[nodiscard]] std::string MakeVortexEmitterName(int i) const;

    //======================== パラメータ ========================
    float warningTime_ = 1.5f;          ///< 予兆時間
    float expandTime_ = 0.5f;            ///< 強度ランプアップ時間
    float sustainTime_ = 4.5f;           ///< 持続時間（フル稼働）
    float decayTime_ = 0.5f;             ///< 終息時間（ランプダウン + 硬直）

    float orbitRadius_ = 8.0f;           ///< ボス → 渦点の距離（公転半径）
    float angularSpeed_ = 0.6f;          ///< 公転角速度（rad/秒）

    float minDoT_ = 5.0f;                ///< 巻き込み外周時の DoT（damage / 秒）
    float maxDoT_ = 15.0f;               ///< 中心時の DoT（damage / 秒）

    /// <summary>
    /// DoT を Player::OnHit に適用する間隔（秒）。
    /// 毎フレーム OnHit を呼ぶと HitFlash/Shake 等の副作用が連打されてゲーム体験が破綻するため、
    /// 累積バッファ + Tick 方式で間引く。
    /// </summary>
    float damageTickInterval_ = 0.5f;

    //======================== ForceField プリセット名 ========================
    std::string attractPresetName_ = "boss_vortexattacck_attract";
    std::string dirPresetName_ = "boss_vortexattacck_dir";
    std::string vortexPresetName_ = "boss_vortexattack_vortex";

    //======================== エミッター名 ========================
    std::string vortexEmitterBaseName_ = "boss_vortex";

    //======================== デカール演出 ========================
    static constexpr float kDecalBaseAlpha = 0.5f;
    static constexpr float kBlinkAlphaMin = 0.15f;
    static constexpr float kBlinkAlphaAmplitude = 0.55f;

    /// <summary>予兆フェーズのアルファ点滅周波数（Hz）</summary>
    float markerBlinkFrequency_ = 4.0f;

    //======================== ランタイム状態 ========================
    /// <summary>プリセットから読み込んだ ForceField 1 件分のランタイム情報</summary>
    struct LoadedField {
        Tako::ForceFieldData base{};
        int32_t index = -1;
    };
    LoadedField loadedFields_[kVortexCount][kFieldsPerVortex];

    /// <summary>DoT 累積バッファ</summary>
    float pendingDamage_ = 0.0f;
    /// <summary>DoT Tick 用タイマー</summary>
    float damageTickTimer_ = 0.0f;

    /// <summary>4 渦点それぞれに 1 個ずつ貼る地面マーカーデカール</summary>
    std::array<std::unique_ptr<Tako::Decal>, kVortexCount> vortexDecals_{};

    //======================== Reset 用キャッシュ ========================
    Tako::ForceFieldManager* cachedForceFieldManager_ = nullptr;
};
