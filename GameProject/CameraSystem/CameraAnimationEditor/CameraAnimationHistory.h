#pragma once
#include "CameraAnimation/CameraAnimation.h"
#include "CameraAnimation/CameraKeyframe.h"
#include <vector>
#include <memory>
#include <string>

#ifdef _DEBUG

/// <summary>
/// カメラアニメーション編集のアンドゥ/リドゥ履歴
/// </summary>
class CameraAnimationHistory {
public:
    enum class ActionType {
        ADD_KEYFRAME,
        DELETE_KEYFRAME,
        EDIT_KEYFRAME,
        MOVE_KEYFRAME,
        BULK_EDIT,
        CLEAR_ALL
    };

    /// <summary>
    /// 編集操作1件を表す基底クラス（Execute / Undo を実装）
    /// </summary>
    class Action {
    public:
        virtual ~Action() = default;

        virtual void Execute(CameraAnimation* animation) = 0;

        virtual void Undo(CameraAnimation* animation) = 0;

        virtual ActionType GetType() const = 0;

        virtual std::string GetDescription() const = 0;
    };

    class AddKeyframeAction : public Action {
    public:
        AddKeyframeAction(const CameraKeyframe& keyframe, size_t index)
            : keyframe_(keyframe), index_(index) {
        }

        void Execute(CameraAnimation* animation) override;
        void Undo(CameraAnimation* animation) override;
        ActionType GetType() const override { return ActionType::ADD_KEYFRAME; }
        std::string GetDescription() const override { return "Add Keyframe"; }

    private:
        CameraKeyframe keyframe_;
        size_t index_;
    };

    class DeleteKeyframeAction : public Action {
    public:
        DeleteKeyframeAction(const CameraKeyframe& keyframe, size_t index)
            : keyframe_(keyframe), index_(index) {
        }

        void Execute(CameraAnimation* animation) override;
        void Undo(CameraAnimation* animation) override;
        ActionType GetType() const override { return ActionType::DELETE_KEYFRAME; }
        std::string GetDescription() const override { return "Delete Keyframe"; }

    private:
        CameraKeyframe keyframe_;
        size_t index_;
    };

    class EditKeyframeAction : public Action {
    public:
        /// <summary>
        /// キーフレーム編集アクション。Execute で newKf、Undo で oldKf を適用
        /// </summary>
        /// <param name="index">対象キーフレームの添字</param>
        /// <param name="oldKf">編集前の内容</param>
        /// <param name="newKf">編集後の内容</param>
        EditKeyframeAction(size_t index, const CameraKeyframe& oldKf, const CameraKeyframe& newKf)
            : index_(index), oldKeyframe_(oldKf), newKeyframe_(newKf) {
        }

        void Execute(CameraAnimation* animation) override;
        void Undo(CameraAnimation* animation) override;
        ActionType GetType() const override { return ActionType::EDIT_KEYFRAME; }
        std::string GetDescription() const override { return "Edit Keyframe"; }

    private:
        size_t index_;
        CameraKeyframe oldKeyframe_;
        CameraKeyframe newKeyframe_;
    };

public:
    CameraAnimationHistory();

    ~CameraAnimationHistory();

    void Initialize(CameraAnimation* animation);

    /// <summary>
    /// アクションを実行して履歴に積む
    /// </summary>
    void ExecuteAction(std::unique_ptr<Action> action);

    /// <summary>
    /// 追加済みキーフレームを履歴に記録（実行はしない）
    /// </summary>
    /// <param name="index">追加されたキーフレームの添字。範囲外なら記録しない</param>
    void RecordAdd(size_t index);

    /// <summary>
    /// 削除済みキーフレームを履歴に記録（実行はしない）
    /// </summary>
    /// <param name="index">削除されたキーフレームの添字</param>
    /// <param name="keyframe">削除前のキーフレーム内容（Undo 復元用）</param>
    void RecordDelete(size_t index, const CameraKeyframe& keyframe);

    /// <summary>
    /// 編集済みキーフレームを履歴に記録（実行はしない）
    /// </summary>
    /// <param name="index">編集されたキーフレームの添字。範囲外なら記録しない</param>
    /// <param name="oldKf">編集前の内容（Undo 復元用）</param>
    /// <param name="newKf">編集後の内容（Redo 適用用）</param>
    void RecordEdit(size_t index, const CameraKeyframe& oldKf, const CameraKeyframe& newKf);

    void Undo();

    void Redo();

    bool CanUndo() const { return currentIndex_ > 0; }

    bool CanRedo() const { return currentIndex_ < history_.size(); }

    void Clear();

    void SetMaxHistorySize(size_t size) { maxHistorySize_ = size; }

    size_t GetHistorySize() const { return history_.size(); }

    size_t GetCurrentIndex() const { return currentIndex_; }

    /// <summary>
    /// 履歴の一覧文字列（デバッグ用）
    /// </summary>
    std::string GetHistoryInfo() const;

private:
    /// <summary>
    /// maxHistorySize_ を超えた古い履歴を切り詰める
    /// </summary>
    void LimitHistorySize();

private:
    CameraAnimation* animation_ = nullptr;
    std::vector<std::unique_ptr<Action>> history_;
    size_t currentIndex_ = 0;                               ///< 次に Redo する位置
    size_t maxHistorySize_ = 100;
    bool isExecuting_ = false;                              ///< 再帰記録を防ぐフラグ
};

#endif // _DEBUG