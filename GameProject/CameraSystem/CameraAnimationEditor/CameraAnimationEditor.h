#pragma once

#ifdef _DEBUG

#include "CameraAnimation/CameraAnimation.h"
#include "CameraAnimation/CameraKeyframe.h"
#include "Camera.h"
#include <memory>
#include <vector>

// Forward declarations
class CameraAnimationTimeline;
class CameraAnimationCurveEditor;
class CameraAnimationHistory;

/// <summary>
/// タイムライン・カーブ・インスペクターを統合したカメラアニメーション編集 UI
/// </summary>
class CameraAnimationEditor {
private: //定数
    static constexpr int kPathDivision = 24;    ///< 経路線の描画で1区間を何本の線分に分けるか

public: //メンバー関数
    CameraAnimationEditor();
    ~CameraAnimationEditor();

    void Initialize(CameraAnimation* animation, Tako::Camera* camera);

    /// <summary>
    /// コントローラー経由で初期化（複数アニメーション管理）
    /// </summary>
    void Initialize(class CameraAnimationController* controller, Tako::Camera* camera);

    void Draw();
    void Update(float deltaTime);
    void ProcessShortcuts();
    void Open() { isOpen_ = true; }
    void Close() { isOpen_ = false; }

    //==========================================================================
    //Setter
    //==========================================================================
    /// <summary>
    /// 相対座標の基準ターゲットを設定。アニメーションとコントローラーにも反映
    /// </summary>
    /// <param name="target">基準 Transform（nullptr で解除）</param>
    /// <param name="name">表示用の名前。空なら target の有無に応じて "Target"/"None"</param>
    void SetTarget(const Tako::Transform* target, const std::string& name = "");

    //==========================================================================
    //Getter
    //==========================================================================
    bool IsOpen() const { return isOpen_; }
    const Tako::Transform* GetTarget() const { return targetTransform_; }

private: //非公開関数
    void DrawMenuBar();
    void DrawAnimationSelector();
    void DrawTimelinePanel();
    void DrawInspectorPanel();
    void DrawCurveEditorPanel();
    void DrawStatusBar();
    void DrawPlaybackControls();

    /// <summary>
    /// 編集中アニメーションの位置経路を LineRenderer で3D空間へ描画
    /// </summary>
    void DrawPathVisualization();

    /// <summary>
    /// 時刻をグリッド間隔へ丸める
    /// </summary>
    /// <param name="time">入力時刻（秒）</param>
    /// <returns>スナップ有効時は丸めた時刻、無効時は time をそのまま返す</returns>
    float SnapToGrid(float time) const;

    void CopySelectedKeyframes();
    void PasteKeyframes();
    void DeleteSelectedKeyframes();
    void Undo();
    void Redo();

private: //メンバー変数
    bool isOpen_   = false;
    bool showPath_ = true;    ///< カメラ経路を3D空間に線で表示するか

    //編集対象
    CameraAnimation*                 animation_  = nullptr;
    Tako::Camera*                    camera_     = nullptr;
    class CameraAnimationController* controller_ = nullptr;  ///< 複数アニメーション管理用

    //UI コンポーネント
    std::unique_ptr<CameraAnimationTimeline>    timeline_;
    std::unique_ptr<CameraAnimationCurveEditor> curveEditor_;
    std::unique_ptr<CameraAnimationHistory>     history_;

    //選択状態
    std::vector<int> selectedKeyframes_;
    int              hoveredKeyframe_   = -1;

    //ドラッグ状態
    bool               isDragging_     = false;
    std::vector<float> dragStartTimes_;          ///< ドラッグ開始時の各キーフレーム時刻

    //タイムライン設定
    float gridSnapInterval_ = 0.1f;  ///< 秒
    bool  enableGridSnap_   = true;

    std::vector<CameraKeyframe> clipboard_;

    //プレビュー機能
    bool        enablePreview_          = false;
    std::string previousControllerName_;          ///< プレビュー復帰先のコントローラー名

    //ターゲット設定
    const Tako::Transform* targetTransform_ = nullptr;
    std::string            targetName_      = "None";   ///< 表示用
};

#endif // _DEBUG