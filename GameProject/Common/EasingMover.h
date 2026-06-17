#pragma once
#include "Vector3.h"

/// <summary>
/// 開始位置から目標位置へイージング補間で移動する
/// </summary>
class EasingMover
{
public: //構造体
    enum class EasingType {
        Linear,
        SmoothStep,  ///< デフォルト
        EaseOut
    };

public: //メンバー関数
    EasingMover() = default;
    ~EasingMover() = default;

    /// <summary>
    /// duration 秒で start から target へ移動するよう初期化
    /// </summary>
    /// <param name="start">開始位置</param>
    /// <param name="target">目標位置</param>
    /// <param name="duration">移動にかける時間（秒）</param>
    void Initialize(const Tako::Vector3& start, const Tako::Vector3& target, float duration);

    /// <summary>
    /// speed から所要時間を逆算して初期化（移動距離は Y を除く XZ 水平距離）
    /// </summary>
    /// <param name="start">開始位置</param>
    /// <param name="target">目標位置</param>
    /// <param name="speed">移動速度（単位/秒）。0 以下なら所要時間 0</param>
    void InitializeWithSpeed(const Tako::Vector3& start, const Tako::Vector3& target, float speed);

    /// <summary>
    /// 補間を進め、現在位置を返す
    /// </summary>
    /// <param name="deltaTime">経過時間（秒）</param>
    /// <returns>イージング適用後の現在位置。未初期化または duration 0 以下なら目標位置</returns>
    Tako::Vector3 Update(float deltaTime);

    void Reset();

    //=============================================
    //Setter
    //=============================================
    void SetEasingType(EasingType type);

    //=============================================
    //Getter
    //=============================================
    bool HasReached() const;
    bool IsInitialized() const;

    /// <summary>
    /// 進捗 0.0(開始)〜1.0(到達) を返す
    /// </summary>
    float GetProgress() const;

    const Tako::Vector3& GetTargetPosition() const;
    const Tako::Vector3& GetStartPosition() const;

private: //非公開関数
    /// <summary>
    /// 正規化時間 t(0.0〜1.0) にイージングを適用
    /// </summary>
    float ApplyEasing(float t) const;

private: //メンバー変数
    Tako::Vector3 startPosition_;
    Tako::Vector3 targetPosition_;
    float         elapsedTime_    = 0.0f;
    float         duration_       = 0.0f;
    bool          isInitialized_  = false;
    EasingType    easingType_     = EasingType::SmoothStep;
};
