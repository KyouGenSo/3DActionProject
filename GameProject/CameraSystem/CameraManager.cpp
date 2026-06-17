#include "CameraManager.h"
#include <algorithm>
#include <format>
#include "RandomEngine.h"
#include "GlobalVariables.h"

using namespace Tako;

std::unique_ptr<CameraManager> CameraManager::instance_ = nullptr;

CameraManager* CameraManager::GetInstance() {
    if (!instance_) {
        instance_ = std::unique_ptr<CameraManager>(new CameraManager());
    }
    return instance_.get();
}

void CameraManager::Initialize(Camera* camera) {
    camera_ = camera;
    controllers_.clear();
    nameToIndex_.clear();
    needsSort_ = false;

    LoadShakeParameters();
}

void CameraManager::Finalize() {
    DeactivateAllControllers();
    controllers_.clear();
    nameToIndex_.clear();
    camera_ = nullptr;

    instance_.reset();
}

void CameraManager::Update(float deltaTime) {
    if (!camera_) {
        return;
    }

    if (needsSort_) {
        SortControllersByPriority();
    }

    ICameraController* activeController = GetActiveController();
    if (activeController) {
        activeController->Update(deltaTime);
    }

    UpdateShake(deltaTime);
    ApplyShakeOffset();
}

void CameraManager::RegisterController(const std::string& name,
    std::unique_ptr<ICameraController> controller) {
    if (!controller) {
        return;
    }

    // 同名の既存コントローラーを置き換え
    RemoveController(name);

    controller->SetCamera(camera_);

    size_t newIndex = controllers_.size();
    controllers_.push_back({ name, std::move(controller) });
    nameToIndex_[name] = newIndex;

    needsSort_ = true;
}

ICameraController* CameraManager::GetController(const std::string& name) {
    auto it = nameToIndex_.find(name);
    if (it != nameToIndex_.end() && it->second < controllers_.size()) {
        return controllers_[it->second].controller.get();
    }
    return nullptr;
}

bool CameraManager::RemoveController(const std::string& name) {
    auto it = nameToIndex_.find(name);
    if (it == nameToIndex_.end()) {
        return false;
    }

    size_t indexToRemove = it->second;

    controllers_.erase(controllers_.begin() + indexToRemove);
    nameToIndex_.erase(it);

    // 削除でずれた後続インデックスを振り直す
    nameToIndex_.clear();
    for (size_t i = 0; i < controllers_.size(); ++i) {
        nameToIndex_[controllers_[i].name] = i;
    }

    return true;
}

bool CameraManager::ActivateController(const std::string& name) {
    if (GetActiveControllerName() == name) {
        return true;
    }

    DeactivateAllControllers();
    ICameraController* controller = GetController(name);
    if (controller) {
        controller->Activate();
        return true;
    }
    return false;
}

bool CameraManager::DeactivateController(const std::string& name) {
    ICameraController* controller = GetController(name);
    if (controller) {
        controller->Deactivate();
        return true;
    }
    return false;
}

void CameraManager::DeactivateAllControllers() {
    for (auto& entry : controllers_) {
        entry.controller->Deactivate();
    }
}

ICameraController* CameraManager::GetActiveController() const {
    int activeIndex = FindHighestPriorityActiveController();
    if (activeIndex >= 0 && activeIndex < static_cast<int>(controllers_.size())) {
        return controllers_[activeIndex].controller.get();
    }
    return nullptr;
}

std::string CameraManager::GetActiveControllerName() const {
    int activeIndex = FindHighestPriorityActiveController();
    if (activeIndex >= 0 && activeIndex < static_cast<int>(controllers_.size())) {
        return controllers_[activeIndex].name;
    }
    return "";
}

std::string CameraManager::GetDebugInfo() const {
    std::string result = std::format("=== Camera Manager Debug Info ===\n");
    result += std::format("Total Controllers: {}\n", controllers_.size());
    result += std::format("Active Controller: {}\n\n", GetActiveControllerName());

    result += "Controller List (Priority Order):\n";
    for (const auto& entry : controllers_) {
        result += std::format("  - {} [Priority: {}] [Active: {}]\n",
            entry.name,
            static_cast<int>(entry.controller->GetPriority()),
            entry.controller->IsActive() ? "Yes" : "No");
    }

    return result;
}

void CameraManager::SortControllersByPriority() {
    // 優先度降順（operator< が降順を定義）
    std::sort(controllers_.begin(), controllers_.end());

    nameToIndex_.clear();
    for (size_t i = 0; i < controllers_.size(); ++i) {
        nameToIndex_[controllers_[i].name] = i;
    }

    needsSort_ = false;
}

int CameraManager::FindHighestPriorityActiveController() const {
    // ソート済みなので最初のアクティブが最高優先度
    for (size_t i = 0; i < controllers_.size(); ++i) {
        if (controllers_[i].controller->IsActive()) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void CameraManager::StartShake(float intensity) {
    isShaking_ = true;
    shakeTimer_ = 0.0f;
    currentShakeIntensity_ = (intensity > 0.0f) ? intensity : shakeIntensity_;
}

void CameraManager::UpdateShake(float deltaTime) {
    if (!isShaking_) {
        shakeOffset_ = { 0.0f, 0.0f, 0.0f };
        return;
    }

    shakeTimer_ += deltaTime;

    if (shakeTimer_ >= shakeDuration_) {
        isShaking_ = false;
        shakeTimer_ = 0.0f;
        shakeOffset_ = { 0.0f, 0.0f, 0.0f };
        return;
    }

    // 経過に応じて 1.0→0.0 へ線形減衰
    float decay = 1.0f - (shakeTimer_ / shakeDuration_);

    RandomEngine* rng = RandomEngine::GetInstance();
    shakeOffset_.x = rng->GetFloat(-currentShakeIntensity_, currentShakeIntensity_) * decay;
    shakeOffset_.y = rng->GetFloat(-currentShakeIntensity_, currentShakeIntensity_) * decay;
    shakeOffset_.z = rng->GetFloat(-currentShakeIntensity_, currentShakeIntensity_) * decay;
}

void CameraManager::ApplyShakeOffset() {
    if (!camera_) return;

    Vector3 currentPos = camera_->GetTranslate();
    camera_->SetTranslate(currentPos + shakeOffset_);
}

void CameraManager::LoadShakeParameters() {
    GlobalVariables* gv = GlobalVariables::GetInstance();
    shakeDuration_ = gv->GetValueFloat("CameraShake", "Duration");
    shakeIntensity_ = gv->GetValueFloat("CameraShake", "Intensity");
}