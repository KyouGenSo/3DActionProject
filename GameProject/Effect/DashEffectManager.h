#pragma once
#include "Vector3.h"
#include <string>

namespace Tako {
    class EmitterManager;
}

/// <summary>
/// プレイヤーダッシュ時のエミッター位置を Lerp 補間で追従させる
/// </summary>
class DashEffectManager
{
public: //構造体
    struct Params {
        float lerpSpeed = 35.0f;                        ///< 指数減衰の係数
        float stopThreshold = 0.65f;                    ///< ダッシュ終了後に無効化する距離
        std::string emitterName = "player_dash";
    };

public: //メンバー関数
    explicit DashEffectManager(Tako::EmitterManager* emitterManager);
    ~DashEffectManager() = default;

    /// <summary>
    /// ダッシュ開始でエミッターを有効化し、毎フレーム位置を指数補間で追従させる
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過時間（秒）</param>
    /// <param name="playerPosition">追従先のプレイヤー位置</param>
    /// <param name="isDashing">ダッシュ中か。false でも追いつくまで補間を継続</param>
    void Update(float deltaTime, const Tako::Vector3& playerPosition, bool isDashing);

    /// <summary>
    /// 補間中のエミッター位置を即座に指定値へ初期化する
    /// </summary>
    /// <param name="position">設定する初期位置</param>
    void InitializePosition(const Tako::Vector3& position);

    //==================================
    //Setter
    //==================================
    void SetParams(const Params& params) { params_ = params; }

    //==================================
    //Getter
    //==================================
    bool IsActive() const { return isActive_; }
    const Params& GetParams() const { return params_; }

private: //メンバー変数
    Tako::EmitterManager* emitterManager_ = nullptr;
    Tako::Vector3         emitterPosition_{};            ///< 補間中のエミッター位置
    bool                  previousIsDashing_ = false;
    bool                  isActive_          = false;
    Params                params_;
};
