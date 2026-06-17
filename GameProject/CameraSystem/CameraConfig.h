#pragma once
#include <DirectXMath.h>

/// <summary>
/// カメラシステム全体の調整定数
/// </summary>
namespace CameraConfig {

    //==================== 共通設定 ====================

    inline constexpr float FOLLOW_SMOOTHNESS   = 0.18f;                  // 0.0-1.0、大きいほど即追従
    inline constexpr float OFFSET_LERP_SPEED   = 0.08f;                  // 0.0-1.0
    inline constexpr float ROTATION_LERP_SPEED = 0.15f;                  // 0.0-1.0
    inline constexpr float STANDARD_FOV        = 0.44999998807907104;    // ラジアン

    // パーティクルがシーン遷移後も残って表示されるのを防ぐためカメラを退避させる Y 座標
    inline constexpr float HIDDEN_Y = -1000.0f;

    //==================== ThirdPerson 設定 ====================

    namespace ThirdPerson {
        inline constexpr float DEFAULT_OFFSET_X = 0.0f;
        inline constexpr float DEFAULT_OFFSET_Y = 2.0f;
        inline constexpr float DEFAULT_OFFSET_Z = -40.0f;

        inline constexpr float LOOK_DOWN_ANGLE = DirectX::XMConvertToRadians(15.0f);  // ボス注視時の見下ろし
        inline constexpr float DEFAULT_ANGLE_X = DirectX::XMConvertToRadians(8.0f);

        inline constexpr float DEFAULT_ROTATE_SPEED      = 0.05f;
        inline constexpr float GAMEPAD_ROTATE_MULTIPLIER = 1.0f;
    }

    //==================== TopDown 設定 ====================

    namespace TopDown {
        inline constexpr float BASE_HEIGHT       = 10.0f;
        inline constexpr float HEIGHT_MULTIPLIER = 0.6f;  // ターゲット間距離に対する高さ倍率
        inline constexpr float MIN_HEIGHT        = 26.0f;
        inline constexpr float MAX_HEIGHT        = 500.0f;

        inline constexpr float DEFAULT_ANGLE_X = DirectX::XMConvertToRadians(25.0f);

        inline constexpr float BASE_BACK_OFFSET       = -10.0f;
        inline constexpr float BACK_OFFSET_MULTIPLIER = 1.5f;  // ターゲット間距離に対する後退倍率
        inline constexpr float MIN_BACK_OFFSET        = -500.0f;
        inline constexpr float MAX_BACK_OFFSET        = -52.0f;

        inline constexpr float INITIAL_HEIGHT      = 88.0f;
        inline constexpr float INITIAL_BACK_OFFSET = -205.0f;
    }

    //==================== アニメーション設定 ====================

    namespace Animation {
        inline constexpr float  DEFAULT_PLAY_SPEED    = 1.0f;
        inline constexpr size_t KEYFRAME_RESERVE_COUNT = 32;

        inline constexpr float DEFAULT_FOV            = 0.45f;  // ラジアン
        inline constexpr float DEFAULT_BLEND_DURATION = 0.5f;   // 秒

        inline constexpr float MIN_PLAY_SPEED    = -2.0f;
        inline constexpr float MAX_PLAY_SPEED    = 2.0f;
        inline constexpr float KEYFRAME_DRAG_STEP = 0.1f;

        inline constexpr float FOV_MIN_DEGREES     = 10.0f;
        inline constexpr float FOV_MAX_DEGREES     = 120.0f;
        inline constexpr float DEFAULT_FOV_DEGREES = 45.0f;
    }

    //==================== カメラシェイク設定 ====================

    namespace Shake {
        inline constexpr float DEFAULT_DURATION  = 0.3f;  // 秒
        inline constexpr float DEFAULT_INTENSITY = 0.5f;
    }
}
