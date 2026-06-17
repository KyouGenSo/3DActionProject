#pragma once
#include "Camera.h"
#include "Transform.h"
#include <memory>
#include <vector>

/// <summary>
/// カメラコントローラーの優先度。数値が大きいほど優先
/// </summary>
enum class CameraControlPriority {
    USER_INPUT = 0,
    FOLLOW_DEFAULT = 50,
    SCRIPTED_EVENT = 75,
    ANIMATION = 100,
    DEBUG_OVERRIDE = 999
};

/// <summary>
/// カメラコントローラーインターフェース
/// </summary>
class ICameraController {
public: //メンバー関数
    virtual ~ICameraController() = default;

    virtual void Update(float deltaTime) = 0;
    virtual void Activate() = 0;
    virtual void Deactivate() = 0;

    //===================================================================================
    //Setter
    //===================================================================================
    virtual void SetCamera(Tako::Camera* camera) { camera_ = camera; }

    //===================================================================================
    //Getter
    //===================================================================================
    virtual bool IsActive() const = 0;
    virtual CameraControlPriority GetPriority() const = 0;
    Tako::Camera* GetCamera() const { return camera_; }

protected: //メンバー変数
    Tako::Camera* camera_ = nullptr;
    bool isActive_ = false;
};

/// <summary>
/// ターゲット追従型コントローラーの基底クラス
/// </summary>
class TargetedCameraController : public ICameraController {
public: //メンバー関数
    //===================================================================================
    //Setter
    //===================================================================================
    virtual void SetTarget(const Tako::Transform* target) {
        primaryTarget_ = target;
    }

    virtual void SetAdditionalTargets(const std::vector<const Tako::Transform*>& targets) {
        additionalTargets_ = targets;
    }

protected: //メンバー変数
    const Tako::Transform* primaryTarget_ = nullptr;
    std::vector<const Tako::Transform*> additionalTargets_;
};
