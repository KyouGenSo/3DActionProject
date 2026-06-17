#include "CameraAnimationController.h"

using namespace Tako;

CameraAnimationController::CameraAnimationController() {
    animations_["Default"] = std::make_unique<CameraAnimation>();
    animations_["Default"]->SetAnimationName("Default");
    currentAnimationName_ = "Default";
}

void CameraAnimationController::Update(float deltaTime) {
    auto* animation = GetCurrentAnimation();
    if (!animation || !camera_) {
        return;
    }

    animation->Update(deltaTime);

    // ワンショット再生の完了時に自動非アクティブ化
    if (autoDeactivateOnComplete_) {
        auto state = animation->GetPlayState();
        if (state == CameraAnimation::PlayState::STOPPED &&
            !animation->IsLooping()) {
            isActive_ = false;
        }
    }
}

bool CameraAnimationController::IsActive() const {
    // プレビューモードで確実にアクティブにするため、アニメーション状態より優先
    if (isActive_) {
        return true;
    }

    auto* animation = const_cast<CameraAnimationController*>(this)->GetCurrentAnimation();
    if (!animation) {
        return false;
    }

    // 再生中または編集中もアクティブ扱い
    return animation->GetPlayState() == CameraAnimation::PlayState::PLAYING ||
        animation->IsEditingKeyframe();
}

void CameraAnimationController::Activate() {
    isActive_ = true;
}

void CameraAnimationController::Deactivate() {
    isActive_ = false;
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->Stop();
    }
}

void CameraAnimationController::SetCamera(Camera* camera) {
    ICameraController::SetCamera(camera);

    for (auto& pair : animations_) {
        pair.second->SetCamera(camera);
    }
}

void CameraAnimationController::SetAnimationTarget(const Transform* target, bool applyToAll) {
    if (applyToAll) {
        for (auto& pair : animations_) {
            pair.second->SetTarget(target);
        }
    }
    else {
        auto* animation = GetCurrentAnimation();
        if (animation) {
            animation->SetTarget(target);
        }
    }
}

void CameraAnimationController::SetAnimationTargetByName(const std::string& animationName, const Transform* target) {
    auto it = animations_.find(animationName);
    if (it != animations_.end()) {
        it->second->SetTarget(target);
    }
}

void CameraAnimationController::SetCurrentAnimationTarget(const Transform* target) {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->SetTarget(target);
    }
}

bool CameraAnimationController::LoadAnimation(const std::string& name) {
    // 後方互換: 現在のアニメーションに読み込む
    auto* animation = GetCurrentAnimation();
    if (!animation) {
        return false;
    }
    return animation->LoadFromJson(name);
}

void CameraAnimationController::Play() {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->Play();
        isActive_ = true;
    }
}

void CameraAnimationController::Pause() {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->Pause();
    }
}

void CameraAnimationController::Stop() {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->Stop();
        isActive_ = false;
    }
}

void CameraAnimationController::Reset() {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->Reset();
    }
}

void CameraAnimationController::SetAnimationStartMode(CameraAnimation::StartMode mode, float blendDuration) {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->SetStartMode(mode);
        animation->SetBlendDuration(blendDuration);
    }
}

void CameraAnimationController::SetAnimationStartModeByName(const std::string& animationName,
    CameraAnimation::StartMode mode, float blendDuration) {
    auto it = animations_.find(animationName);
    if (it != animations_.end()) {
        it->second->SetStartMode(mode);
        it->second->SetBlendDuration(blendDuration);
    }
}

void CameraAnimationController::AddKeyframe(const CameraKeyframe& keyframe) {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->AddKeyframe(keyframe);
    }
}

void CameraAnimationController::AddKeyframeFromCurrentCamera(float time,
    CameraKeyframe::InterpolationType interpolation) {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->AddKeyframeFromCurrentCamera(time, interpolation);
    }
}

void CameraAnimationController::RemoveKeyframe(size_t index) {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->RemoveKeyframe(index);
    }
}

void CameraAnimationController::ClearKeyframes() {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->ClearKeyframes();
    }
}

void CameraAnimationController::SetLooping(bool loop) {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->SetLooping(loop);
    }
}

void CameraAnimationController::SetPlaySpeed(float speed) {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->SetPlaySpeed(speed);
    }
}

void CameraAnimationController::SetAnimationName(const std::string& name) {
    auto* animation = GetCurrentAnimation();
    if (animation) {
        animation->SetAnimationName(name);
    }
}

CameraAnimation::PlayState CameraAnimationController::GetPlayState() const {
    auto* animation = const_cast<CameraAnimationController*>(this)->GetCurrentAnimation();
    if (animation) {
        return animation->GetPlayState();
    }
    return CameraAnimation::PlayState::STOPPED;
}

float CameraAnimationController::GetDuration() const {
    auto* animation = const_cast<CameraAnimationController*>(this)->GetCurrentAnimation();
    if (animation) {
        return animation->GetDuration();
    }
    return 0.0f;
}

float CameraAnimationController::GetCurrentTime() const {
    auto* animation = const_cast<CameraAnimationController*>(this)->GetCurrentAnimation();
    if (animation) {
        return animation->GetPlaybackTime();
    }
    return 0.0f;
}

bool CameraAnimationController::IsEditingKeyframe() const {
    auto* anim = const_cast<CameraAnimationController*>(this)->GetCurrentAnimation();
    if (anim) {
        return anim->IsEditingKeyframe();
    }
    return false;
}

const Transform* CameraAnimationController::GetAnimationTarget() const {
    auto* anim = const_cast<CameraAnimationController*>(this)->GetCurrentAnimation();
    if (anim) {
        return anim->GetTarget();
    }
    return nullptr;
}

//==================== アニメーション管理 ====================

CameraAnimation* CameraAnimationController::GetCurrentAnimation() {
    auto it = animations_.find(currentAnimationName_);
    if (it != animations_.end()) {
        return it->second.get();
    }
    return nullptr;
}

CameraAnimation* CameraAnimationController::GetAnimation(const std::string& name) {
    auto it = animations_.find(name);
    if (it != animations_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool CameraAnimationController::CreateAnimation(const std::string& name) {
    if (animations_.find(name) != animations_.end()) {
        return false;
    }

    animations_[name] = std::make_unique<CameraAnimation>();
    animations_[name]->SetAnimationName(name);

    if (camera_) {
        animations_[name]->SetCamera(camera_);
    }

    return true;
}

bool CameraAnimationController::SwitchAnimation(const std::string& name) {
    if (animations_.find(name) == animations_.end()) {
        return false;
    }

    // 現在のアニメーションを FOV 復元なしで停止
    auto* current = GetCurrentAnimation();
    if (current) {
        current->StopWithoutRestore();
    }

    currentAnimationName_ = name;

    auto* newAnim = GetCurrentAnimation();
    if (newAnim && camera_) {
        newAnim->SetCamera(camera_);
    }

    return true;
}

bool CameraAnimationController::DeleteAnimation(const std::string& name) {
    // Default は削除不可
    if (name == "Default") {
        return false;
    }

    auto it = animations_.find(name);
    if (it == animations_.end()) {
        return false;
    }

    // 削除対象が現在のものなら Default に戻す
    if (name == currentAnimationName_) {
        currentAnimationName_ = "Default";
    }

    animations_.erase(it);
    return true;
}

bool CameraAnimationController::RenameAnimation(const std::string& oldName, const std::string& newName) {
    // Default はリネーム不可
    if (oldName == "Default") {
        return false;
    }

    auto it = animations_.find(oldName);
    if (it == animations_.end()) {
        return false;
    }

    if (animations_.find(newName) != animations_.end()) {
        return false;
    }

    auto animation = std::move(it->second);
    animation->SetAnimationName(newName);
    animations_.erase(it);
    animations_[newName] = std::move(animation);

    if (oldName == currentAnimationName_) {
        currentAnimationName_ = newName;
    }

    return true;
}

bool CameraAnimationController::DuplicateAnimation(const std::string& sourceName, const std::string& newName) {
    auto* source = GetAnimation(sourceName);
    if (!source) {
        return false;
    }

    if (animations_.find(newName) != animations_.end()) {
        return false;
    }

    animations_[newName] = std::make_unique<CameraAnimation>();
    animations_[newName]->SetAnimationName(newName);

    if (camera_) {
        animations_[newName]->SetCamera(camera_);
    }

    for (size_t i = 0; i < source->GetKeyframeCount(); ++i) {
        animations_[newName]->AddKeyframe(source->GetKeyframe(i));
    }

    animations_[newName]->SetLooping(source->IsLooping());

    return true;
}

bool CameraAnimationController::LoadAnimationFromFile(const std::string& name) {
    if (!CreateAnimation(name)) {
        return false;
    }

    auto* anim = GetAnimation(name);
    if (!anim || !anim->LoadFromJson(name)) {
        // 読み込み失敗時は作成済みエントリを巻き戻す
        DeleteAnimation(name);
        return false;
    }

    return true;
}

bool CameraAnimationController::SaveAnimationToFile(const std::string& name) {
    auto* anim = GetAnimation(name);
    if (!anim) {
        return false;
    }

    return anim->SaveToJson(name);
}

std::vector<std::string> CameraAnimationController::GetAnimationList() const {
    std::vector<std::string> names;
    names.reserve(animations_.size());

    for (const auto& pair : animations_) {
        names.push_back(pair.first);
    }

    return names;
}