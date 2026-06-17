#pragma once

class CooldownTimer
{
public:
    CooldownTimer() = default;
    ~CooldownTimer() = default;

    /// <summary>
    /// duration 秒のクールダウンを開始
    /// </summary>
    /// <param name="duration">クールダウン時間（秒）</param>
    void Start(float duration);

    /// <summary>
    /// 残り時間を deltaTime 分減らす（0 未満にはしない）
    /// </summary>
    /// <param name="deltaTime">経過時間（秒）</param>
    void Update(float deltaTime);

    /// <summary>
    /// クールダウンが完了したかを返す
    /// </summary>
    /// <returns>残り時間が 0 以下なら true</returns>
    bool IsReady() const;

    /// <summary>
    /// クールダウンの残り時間を返す
    /// </summary>
    /// <returns>残り時間（秒）</returns>
    float GetRemainingTime() const;

    /// <summary>
    /// 進捗 0.0(開始直後)〜1.0(完了) を返す
    /// </summary>
    float GetProgress() const;

    void Reset();

private:
    float remainingTime_ = 0.0f;
    float duration_ = 0.0f;
};
