#pragma once
#include "Controller/ICameraController.h"
#include "CameraConfig.h"
#include "Camera.h"
#include "Vector3.h"
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

/// <summary>
/// 優先度ベースでコントローラーを調停するカメラ統合管理クラス
/// </summary>
class CameraManager {
private: //構造体
    struct ControllerEntry {
        std::string name;
        std::unique_ptr<ICameraController> controller;

        // 優先度の降順で並ぶよう逆向きに比較
        bool operator<(const ControllerEntry& other) const {
            return static_cast<int>(controller->GetPriority()) >
                static_cast<int>(other.controller->GetPriority());
        }
    };

public: //メンバー関数
    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    void Initialize(Tako::Camera* camera);
    void Finalize();

    /// <summary>
    /// 最高優先度のアクティブなコントローラーのみを実行
    /// </summary>
    void Update(float deltaTime);

    //=======================================================
    //コントローラー管理
    //=======================================================
    /// <summary>
    /// コントローラーを登録する。同名が既にあれば置き換える
    /// </summary>
    /// <param name="name">コントローラー識別名</param>
    /// <param name="controller">登録するコントローラー（nullptr なら何もしない）</param>
    void RegisterController(const std::string& name,
        std::unique_ptr<ICameraController> controller);

    /// <summary>
    /// 名前でコントローラーを削除
    /// </summary>
    /// <param name="name">コントローラー識別名</param>
    /// <returns>削除できれば true、存在しなければ false</returns>
    bool RemoveController(const std::string& name);

    /// <summary>
    /// 指定コントローラーのみをアクティブ化（他は非アクティブ化）
    /// </summary>
    /// <param name="name">コントローラー識別名</param>
    /// <returns>成功すれば true、存在しなければ false</returns>
    bool ActivateController(const std::string& name);

    /// <summary>
    /// 指定コントローラーを非アクティブ化
    /// </summary>
    /// <param name="name">コントローラー識別名</param>
    /// <returns>成功すれば true、存在しなければ false</returns>
    bool DeactivateController(const std::string& name);

    void DeactivateAllControllers();

    //=======================================================
    //カメラシェイク
    //=======================================================
    /// <summary>
    /// カメラシェイクを開始する
    /// </summary>
    /// <param name="intensity">揺れの強度。0以下でデフォルト値を使用</param>
    void StartShake(float intensity = 0.0f);

    //=======================================================
    //Getter
    //=======================================================
    static CameraManager* GetInstance();

    /// <summary>
    /// 名前でコントローラーを取得
    /// </summary>
    /// <param name="name">コントローラー識別名</param>
    /// <returns>該当コントローラー。存在しない場合 nullptr</returns>
    ICameraController* GetController(const std::string& name);

    /// <returns>アクティブな最高優先度コントローラー（無ければ nullptr）</returns>
    ICameraController* GetActiveController() const;

    /// <returns>アクティブな最高優先度コントローラー名（無ければ空文字列）</returns>
    std::string GetActiveControllerName() const;

    size_t GetControllerCount() const { return controllers_.size(); }
    Tako::Camera* GetCamera() const { return camera_; }

    /// <summary>
    /// 登録コントローラーの一覧と状態を文字列化
    /// </summary>
    /// <returns>優先度順のコントローラー情報（名前・優先度・アクティブ状態）</returns>
    std::string GetDebugInfo() const;

private: //非公開関数
    struct Token {};  ///< 外部からの直接生成を防ぐ生成キー
    ~CameraManager() = default;
    friend struct std::default_delete<CameraManager>;

public:
    explicit CameraManager(Token) {}

private:
    void SortControllersByPriority();

    /// <summary>
    /// 最高優先度のアクティブなコントローラーの添字を取得
    /// </summary>
    /// <returns>controllers_ の添字。見つからなければ -1</returns>
    int FindHighestPriorityActiveController() const;

    void UpdateShake(float deltaTime);
    void ApplyShakeOffset();
    void LoadShakeParameters();

private: //メンバー変数
    static std::unique_ptr<CameraManager> instance_;

    Tako::Camera* camera_ = nullptr;

    //コントローラー（優先度順に並ぶ）
    std::vector<ControllerEntry>            controllers_;
    std::unordered_map<std::string, size_t> nameToIndex_;
    bool                                    needsSort_   = false;

    //カメラシェイク
    bool          isShaking_             = false;
    float         shakeTimer_            = 0.0f;
    float         shakeDuration_         = CameraConfig::Shake::DEFAULT_DURATION;
    float         shakeIntensity_        = CameraConfig::Shake::DEFAULT_INTENSITY;  ///< デフォルト強度
    float         currentShakeIntensity_ = 0.0f;                                    ///< 実行中の強度
    Tako::Vector3 shakeOffset_           = { 0.0f, 0.0f, 0.0f };
};
