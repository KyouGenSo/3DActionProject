#include "DamageFeedback.h"
#include "../CameraSystem/CameraManager.h"
#include "Input.h"
#include "PostEffectManager.h"
#include "PostEffectStruct.h"
#include "EmitterManager.h"
#include "RandomEngine.h"
#include "CameraSystem/CameraManager.h"

using namespace Tako;

void DamageFeedback::TriggerHitFeedback(const HitParams& params)
{
    CameraManager::GetInstance()->StartShake(params.shakeIntensity);

    Input::GetInstance()->SetVibration(
        params.vibrationLow,
        params.vibrationHigh,
        params.vibrationDuration);

    VignetteParam vignetteParam{};
    vignetteParam.power = params.vignettePower;
    vignetteParam.range = params.vignetteRange;
    vignetteParam.color = params.vignetteColor;
    PostEffectManager::GetInstance()->ApplyTemporaryEffect(
        "Vignette",
        params.vignetteDuration,
        vignetteParam);
}

void DamageFeedback::TriggerParryFeedback(
    const Vector3& position,
    EmitterManager* emitterManager,
    const ParryParams& params)
{
    if (emitterManager) {
        emitterManager->SetEmitterPosition(params.emitterBaseName, position);

        // 同名エミッターの衝突を避けるため一意名で複製する
        int uniqueId = RandomEngine::GetInstance()->GetInt(0, 999999);
        std::string tempEmitterName = params.emitterBaseName + "_temp_" + std::to_string(uniqueId);

        emitterManager->CreateTemporaryEmitterFrom(
            params.emitterBaseName,
            tempEmitterName,
            params.emitterDuration);
        emitterManager->SetEmitterActive(tempEmitterName, true);
    }

    CameraManager::GetInstance()->StartShake(params.shakeIntensity);

    Input::GetInstance()->SetVibration(
        params.vibrationLow,
        params.vibrationHigh,
        params.vibrationDuration);

    VignetteParam vignetteParam{};
    vignetteParam.power = params.vignettePower;
    vignetteParam.range = params.vignetteRange;
    vignetteParam.color = params.vignetteColor;
    PostEffectManager::GetInstance()->ApplyTemporaryEffect(
        "Vignette",
        params.vignetteDuration,
        vignetteParam);
}
