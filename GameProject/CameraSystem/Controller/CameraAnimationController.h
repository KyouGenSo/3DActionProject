#pragma once
#include "ICameraController.h"
#include "CameraAnimation/CameraAnimation.h"
#include <memory>
#include <string>

/// <summary>
/// アニメーション再生を優先度システムで管理するカメラコントローラー
/// </summary>
class CameraAnimationController : public ICameraController {
public: //メンバー関数
    CameraAnimationController();
    ~CameraAnimationController() override = default;

    void Update(float deltaTime) override;
    void Activate() override;
    void Deactivate() override;

    //========================================================================================
    //アニメーション制御
    //========================================================================================
    /// <summary>
    /// 現在のアニメーションに JSON を読み込む（後方互換用）
    /// </summary>
    /// <param name="name">JSON ファイルパス</param>
    /// <returns>読み込み成功で true。現在のアニメーションが無い、または読込失敗で false</returns>
    bool LoadAnimation(const std::string& name);

    void Play();
    void Pause();
    void Stop();
    void Reset();

    /// <summary>
    /// 現在のアニメーションの開始モードとブレンド時間を設定
    /// </summary>
    /// <param name="mode">再生開始モード</param>
    /// <param name="blendDuration">ブレンド時間（秒）</param>
    void SetAnimationStartMode(CameraAnimation::StartMode mode, float blendDuration = 0.5f);

    /// <summary>
    /// 指定名のアニメーションの開始モードとブレンド時間を設定
    /// </summary>
    /// <param name="animationName">対象アニメーション名</param>
    /// <param name="mode">再生開始モード</param>
    /// <param name="blendDuration">ブレンド時間（秒）</param>
    void SetAnimationStartModeByName(const std::string& animationName,
        CameraAnimation::StartMode mode, float blendDuration = 0.5f);

    //========================================================================================
    //キーフレーム管理
    //========================================================================================
    void AddKeyframe(const CameraKeyframe& keyframe);

    /// <summary>
    /// 現在のカメラ状態をキーフレームとして追加
    /// </summary>
    /// <param name="time">キーフレームの時刻（秒）</param>
    /// <param name="interpolation">補間方式</param>
    void AddKeyframeFromCurrentCamera(float time,
        CameraKeyframe::InterpolationType interpolation =
        CameraKeyframe::InterpolationType::LINEAR);

    void RemoveKeyframe(size_t index);
    void ClearKeyframes();

    //========================================================================================
    //アニメーション管理
    //========================================================================================
    /// <summary>
    /// 空のアニメーションを新規作成
    /// </summary>
    /// <param name="name">作成するアニメーション名</param>
    /// <returns>作成成功で true。同名が既に存在すれば false</returns>
    bool CreateAnimation(const std::string& name);

    /// <summary>
    /// 現在のアニメーションを指定名のものへ切り替える
    /// </summary>
    /// <param name="name">切り替え先のアニメーション名</param>
    /// <returns>切り替え成功で true。存在しなければ false</returns>
    bool SwitchAnimation(const std::string& name);

    /// <summary>
    /// 指定アニメーションを削除する
    /// </summary>
    /// <param name="name">削除するアニメーション名</param>
    /// <returns>削除成功で true。"Default" 指定または存在しない場合 false</returns>
    bool DeleteAnimation(const std::string& name);

    /// <summary>
    /// アニメーションをリネームする
    /// </summary>
    /// <param name="oldName">変更前の名前</param>
    /// <param name="newName">変更後の名前</param>
    /// <returns>成功で true。"Default" 指定、oldName が無い、newName が既存の場合 false</returns>
    bool RenameAnimation(const std::string& oldName, const std::string& newName);

    /// <summary>
    /// アニメーションを複製する（キーフレームとループ設定をコピー）
    /// </summary>
    /// <param name="sourceName">複製元の名前</param>
    /// <param name="newName">複製先の名前</param>
    /// <returns>成功で true。sourceName が無い、newName が既存の場合 false</returns>
    bool DuplicateAnimation(const std::string& sourceName, const std::string& newName);

    /// <summary>
    /// JSON ファイルから新規アニメーションとして読み込む
    /// </summary>
    /// <param name="name">アニメーション名兼ファイル名</param>
    /// <returns>成功で true。同名が既存、または読込失敗（作成分は巻き戻す）の場合 false</returns>
    bool LoadAnimationFromFile(const std::string& name);

    /// <summary>
    /// 指定アニメーションを JSON ファイルへ保存する
    /// </summary>
    /// <param name="name">保存するアニメーション名</param>
    /// <returns>保存成功で true。該当アニメーションが無ければ false</returns>
    bool SaveAnimationToFile(const std::string& name);

    //========================================================================================
    //Setter
    //========================================================================================
    void SetCamera(Tako::Camera* camera) override;

    /// <param name="target">相対座標の基準（nullptr で解除）</param>
    /// <param name="applyToAll">全アニメーションに適用する場合 true</param>
    void SetAnimationTarget(const Tako::Transform* target, bool applyToAll = false);

    /// <param name="target">相対座標の基準（nullptr で解除）</param>
    void SetAnimationTargetByName(const std::string& animationName, const Tako::Transform* target);

    /// <param name="target">相対座標の基準（nullptr で解除）</param>
    void SetCurrentAnimationTarget(const Tako::Transform* target);

    void SetLooping(bool loop);
    void SetPlaySpeed(float speed);
    void SetAnimationName(const std::string& name);

    //========================================================================================
    //Getter
    //========================================================================================
    bool IsActive() const override;

    CameraControlPriority GetPriority() const override {
        return CameraControlPriority::ANIMATION;
    }

    CameraAnimation* GetCurrentAnimation();

    /// <returns>存在しない場合 nullptr</returns>
    CameraAnimation* GetAnimation(const std::string& name);

    std::vector<std::string> GetAnimationList() const;
    size_t GetAnimationCount() const { return animations_.size(); }
    const std::string& GetCurrentAnimationName() const { return currentAnimationName_; }
    CameraAnimation::PlayState GetPlayState() const;

    /// <returns>現在のアニメーションの長さ（秒）。無ければ 0</returns>
    float GetDuration() const;

    /// <returns>現在の再生位置（秒）。無ければ 0</returns>
    float GetCurrentTime() const;

    bool IsEditingKeyframe() const;

    /// <returns>未設定の場合 nullptr</returns>
    const Tako::Transform* GetAnimationTarget() const;

private: //メンバー変数
    std::map<std::string, std::unique_ptr<CameraAnimation>> animations_;

    std::string currentAnimationName_ = "Default";

    bool autoDeactivateOnComplete_ = true;  ///< ワンショット再生完了時に自動で非アクティブ化するか
};
