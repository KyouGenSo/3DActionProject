#ifdef _DEBUG

#include "CameraAnimationHistory.h"
#include <format>

void CameraAnimationHistory::AddKeyframeAction::Execute(CameraAnimation* animation) {
    animation->AddKeyframe(keyframe_);
}

void CameraAnimationHistory::AddKeyframeAction::Undo(CameraAnimation* animation) {
    animation->RemoveKeyframe(index_);
}

void CameraAnimationHistory::DeleteKeyframeAction::Execute(CameraAnimation* animation) {
    animation->RemoveKeyframe(index_);
}

void CameraAnimationHistory::DeleteKeyframeAction::Undo(CameraAnimation* animation) {
    animation->AddKeyframe(keyframe_);
}

void CameraAnimationHistory::EditKeyframeAction::Execute(CameraAnimation* animation) {
    animation->EditKeyframe(index_, newKeyframe_);
}

void CameraAnimationHistory::EditKeyframeAction::Undo(CameraAnimation* animation) {
    animation->EditKeyframe(index_, oldKeyframe_);
}

CameraAnimationHistory::CameraAnimationHistory() {
    history_.reserve(maxHistorySize_);
}

CameraAnimationHistory::~CameraAnimationHistory() {
    Clear();
}

void CameraAnimationHistory::Initialize(CameraAnimation* animation) {
    animation_ = animation;
    Clear();
}

void CameraAnimationHistory::ExecuteAction(std::unique_ptr<Action> action) {
    if (!animation_ || !action || isExecuting_) {
        return;
    }

    isExecuting_ = true;

    // 新規分岐: 現在位置より後の Redo 履歴を破棄
    if (currentIndex_ < history_.size()) {
        history_.erase(history_.begin() + currentIndex_, history_.end());
    }

    action->Execute(animation_);

    history_.push_back(std::move(action));
    currentIndex_ = history_.size();

    LimitHistorySize();

    isExecuting_ = false;
}

void CameraAnimationHistory::RecordAdd(size_t index) {
    if (!animation_ || index >= animation_->GetKeyframeCount()) {
        return;
    }

    auto action = std::make_unique<AddKeyframeAction>(
        animation_->GetKeyframe(index), index);

    // 既に追加済みなので記録のみ（Execute しない）
    if (currentIndex_ < history_.size()) {
        history_.erase(history_.begin() + currentIndex_, history_.end());
    }
    history_.push_back(std::move(action));
    currentIndex_ = history_.size();
    LimitHistorySize();
}

void CameraAnimationHistory::RecordDelete(size_t index, const CameraKeyframe& keyframe) {
    if (!animation_) {
        return;
    }

    auto action = std::make_unique<DeleteKeyframeAction>(keyframe, index);

    // 既に削除済みなので記録のみ（Execute しない）
    if (currentIndex_ < history_.size()) {
        history_.erase(history_.begin() + currentIndex_, history_.end());
    }
    history_.push_back(std::move(action));
    currentIndex_ = history_.size();
    LimitHistorySize();
}

void CameraAnimationHistory::RecordEdit(size_t index, const CameraKeyframe& oldKf, const CameraKeyframe& newKf) {
    if (!animation_ || index >= animation_->GetKeyframeCount()) {
        return;
    }

    auto action = std::make_unique<EditKeyframeAction>(index, oldKf, newKf);

    // 既に編集済みなので記録のみ（Execute しない）
    if (currentIndex_ < history_.size()) {
        history_.erase(history_.begin() + currentIndex_, history_.end());
    }
    history_.push_back(std::move(action));
    currentIndex_ = history_.size();
    LimitHistorySize();
}

void CameraAnimationHistory::Undo() {
    if (!animation_ || !CanUndo() || isExecuting_) {
        return;
    }

    isExecuting_ = true;

    currentIndex_--;
    history_[currentIndex_]->Undo(animation_);

    isExecuting_ = false;
}

void CameraAnimationHistory::Redo() {
    if (!animation_ || !CanRedo() || isExecuting_) {
        return;
    }

    isExecuting_ = true;

    history_[currentIndex_]->Execute(animation_);
    currentIndex_++;

    isExecuting_ = false;
}

void CameraAnimationHistory::Clear() {
    history_.clear();
    currentIndex_ = 0;
}

void CameraAnimationHistory::LimitHistorySize() {
    if (history_.size() > maxHistorySize_) {
        size_t removeCount = history_.size() - maxHistorySize_;
        history_.erase(history_.begin(), history_.begin() + removeCount);

        if (currentIndex_ > removeCount) {
            currentIndex_ -= removeCount;
        }
        else {
            currentIndex_ = 0;
        }
    }
}

std::string CameraAnimationHistory::GetHistoryInfo() const {
    std::string result = std::format("History: {}/{}\n", currentIndex_, history_.size());

    for (size_t i = 0; i < history_.size(); ++i) {
        const char* prefix = (i == currentIndex_) ? "> " : "  ";
        const char* suffix = (i == currentIndex_ - 1) ? " <-- Current" : "";

        result += std::format("{}{}: {}{}\n", prefix, i, history_[i]->GetDescription(), suffix);
    }

    return result;
}

#endif // _DEBUG