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
/// カメラシステム統合管理クラス
/// 優先度ベースの権限管理とコントローラー調停を担当
/// </summary>
class CameraManager {
public:
    /// <summary>
    /// シングルトンインスタンスを取得
    /// </summary>
    /// <returns>CameraManager のインスタンス</returns>
    static CameraManager* GetInstance();

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="camera">管理対象のカメラ</param>
    void Initialize(Tako::Camera* camera);

    /// <summary>
    /// 終了処理
    /// </summary>
    void Finalize();

    /// <summary>
    /// 更新処理
    /// 最高優先度のアクティブなコントローラーのみを実行
    /// </summary>
    /// <param name="deltaTime">前フレームからの経過時間（秒）</param>
    void Update(float deltaTime);

    //==================== コントローラー管理 ====================

    /// <summary>
    /// コントローラーを登録
    /// </summary>
    /// <param name="name">コントローラー識別名</param>
    /// <param name="controller">コントローラーインスタンス</param>
    void RegisterController(const std::string& name,
        std::unique_ptr<ICameraController> controller);

    /// <summary>
    /// コントローラーを取得
    /// </summary>
    /// <param name="name">コントローラー識別名</param>
    /// <returns>コントローラーのポインタ（存在しない場合 nullptr）</returns>
    ICameraController* GetController(const std::string& name);

    /// <summary>
    /// コントローラーを削除
    /// </summary>
    /// <param name="name">コントローラー識別名</param>
    /// <returns>削除成功した場合 true</returns>
    bool RemoveController(const std::string& name);

    /// <summary>
    /// コントローラーをアクティブ化
    /// </summary>
    /// <param name="name">コントローラー識別名</param>
    /// <returns>アクティブ化成功した場合 true</returns>
    bool ActivateController(const std::string& name);

    /// <summary>
    /// コントローラーを非アクティブ化
    /// </summary>
    /// <param name="name">コントローラー識別名</param>
    /// <returns>非アクティブ化成功した場合 true</returns>
    bool DeactivateController(const std::string& name);

    /// <summary>
    /// 全コントローラーを非アクティブ化
    /// </summary>
    void DeactivateAllControllers();

    //==================== 状態取得 ====================

    /// <summary>
    /// 現在アクティブな最高優先度コントローラーを取得
    /// </summary>
    /// <returns>アクティブなコントローラー（存在しない場合 nullptr）</returns>
    ICameraController* GetActiveController() const;

    /// <summary>
    /// 現在アクティブな最高優先度コントローラーの名前を取得
    /// </summary>
    /// <returns>コントローラー名（存在しない場合空文字列）</returns>
    std::string GetActiveControllerName() const;

    /// <summary>
    /// 登録されているコントローラー数を取得
    /// </summary>
    /// <returns>コントローラー数</returns>
    size_t GetControllerCount() const { return controllers_.size(); }

    /// <summary>
    /// カメラを取得
    /// </summary>
    /// <returns>管理対象のカメラ</returns>
    Tako::Camera* GetCamera() const { return camera_; }

    //==================== カメラシェイク ====================

    /// <summary>
    /// カメラシェイクを開始
    /// </summary>
    /// <param name="intensity">シェイク強度（0以下でデフォルト値使用）</param>
    void StartShake(float intensity = 0.0f);

    //==================== デバッグ ====================

    /// <summary>
    /// デバッグ情報を取得
    /// </summary>
    /// <returns>デバッグ情報文字列</returns>
    std::string GetDebugInfo() const;

private:
    /// <summary>
    /// コンストラクタ（シングルトン）
    /// </summary>
    CameraManager() = default;

    /// <summary>
    /// デストラクタ
    /// </summary>
    ~CameraManager() = default;

    friend struct std::default_delete<CameraManager>;

public:
    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

private:

    /// <summary>
    /// コントローラーを優先度順にソート
    /// </summary>
    void SortControllersByPriority();

    /// <summary>
    /// 最高優先度のアクティブなコントローラーを検索
    /// </summary>
    /// <returns>見つかったコントローラーのインデックス（-1 = 見つからない）</returns>
    int FindHighestPriorityActiveController() const;

    /// <summary>
    /// シェイクエフェクトの更新
    /// </summary>
    /// <param name="deltaTime">フレーム間隔（秒）</param>
    void UpdateShake(float deltaTime);

    /// <summary>
    /// シェイクオフセットをカメラに適用
    /// </summary>
    void ApplyShakeOffset();

    /// <summary>
    /// GlobalVariables からシェイクパラメータを読み込み
    /// </summary>
    void LoadShakeParameters();

private:
    /// <summary>
    /// コントローラー管理エントリ
    /// </summary>
    struct ControllerEntry {
        std::string name;                               ///< 識別名
        std::unique_ptr<ICameraController> controller;  ///< コントローラー実体

        /// <summary>
        /// 優先度による比較演算子
        /// </summary>
        bool operator<(const ControllerEntry& other) const {
            return static_cast<int>(controller->GetPriority()) >
                static_cast<int>(other.controller->GetPriority());
        }
    };

    // シングルトンインスタンス
    static std::unique_ptr<CameraManager> instance_;

    // 管理対象カメラ
    Tako::Camera* camera_ = nullptr;

    // コントローラーリスト（優先度順）
    std::vector<ControllerEntry> controllers_;

    // 名前からインデックスへのマップ（検索用）
    std::unordered_map<std::string, size_t> nameToIndex_;

    // ソートが必要かのフラグ
    bool needsSort_ = false;

    //==================== カメラシェイク ====================
    // シェイク中フラグ
    bool isShaking_ = false;
    // シェイクタイマー（経過時間）
    float shakeTimer_ = 0.0f;
    // シェイク持続時間
    float shakeDuration_ = CameraConfig::Shake::DEFAULT_DURATION;
    // シェイク強度（デフォルト）
    float shakeIntensity_ = CameraConfig::Shake::DEFAULT_INTENSITY;
    // 現在のシェイク強度（実行時）
    float currentShakeIntensity_ = 0.0f;
    // 描画用シェイクオフセット
    Tako::Vector3 shakeOffset_ = { 0.0f, 0.0f, 0.0f };
};