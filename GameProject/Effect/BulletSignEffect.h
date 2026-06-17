#pragma once
#include "Vector3.h"

class Boss;

/// <summary>
/// ボス射撃の予備動作。チャージ中にエミッターを表示しスケールを補間する
/// </summary>
class BulletSignEffect {
private: //定数
    static constexpr float kForwardDistance = 2.0f;
    static constexpr float kScaleMin = 0.01f;
    static constexpr float kScaleMax = 15.0f;

public: //メンバー関数
    /// <summary>
    /// 予備動作を開始し、予兆エミッターを最小スケールで表示する
    /// </summary>
    /// <param name="boss">エミッターの位置・スケールを操作する対象ボス</param>
    /// <param name="duration">チャージにかける時間（秒）。スケール補間の分母</param>
    void Start(Boss* boss, float duration);

    /// <summary>
    /// 経過時間に応じてエミッターのスケールを補間し、位置を更新する
    /// </summary>
    /// <param name="boss">操作対象のボス</param>
    /// <param name="deltaTime">前フレームからの経過時間（秒）</param>
    void Update(Boss* boss, float deltaTime);

    /// <summary>
    /// 予備動作を終了し、予兆エミッターを非表示にする
    /// </summary>
    /// <param name="boss">操作対象のボス</param>
    void End(Boss* boss);

    //===================
    //Getter
    //===================
    bool IsActive() const { return isActive_; }

private: //非公開関数
    /// <summary>
    /// ボスの正面方向へ kForwardDistance だけ進めたエミッター配置位置を求める
    /// </summary>
    /// <param name="boss">位置・向きの取得元のボス</param>
    /// <returns>ボス前方のワールド座標</returns>
    Tako::Vector3 CalculateEmitterPosition(Boss* boss);

private: //メンバー変数
    float duration_    = 0.9f;
    float elapsedTime_ = 0.0f;
    bool  isActive_    = false;
};
