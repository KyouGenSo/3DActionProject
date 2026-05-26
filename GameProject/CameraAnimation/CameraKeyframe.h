#pragma once
#include "Vector3.h"
#include <json.hpp>

/// <summary>
/// カメラアニメーションのキーフレーム構造体
/// 時間軸上の特定ポイントでのカメラ状態を保持
/// </summary>
struct CameraKeyframe {
    /// <summary>
    /// 補間タイプ
    /// </summary>
    enum class InterpolationType {
        LINEAR,         ///< 線形補間
        EASE_IN,        ///< 緩やかに開始
        EASE_OUT,       ///< 緩やかに終了
        EASE_IN_OUT,    ///< 両端で緩やか
        CUBIC_BEZIER    ///< カスタムベジェカーブ
    };

    /// <summary>
    /// 座標系タイプ
    /// </summary>
    enum class CoordinateType {
        WORLD,          ///< ワールド座標系
        TARGET_RELATIVE ///< ターゲット相対座標系（ターゲットからのオフセット）
    };

    float time = 0.0f;                                             ///< キーフレームの時刻（秒）

    Tako::Vector3 position = { 0.0f, 0.0f, 0.0f };                      ///< カメラ位置（WORLD モード）またはオフセット（TARGET_RELATIVE モード）

    Tako::Vector3 rotation = { 0.0f, 0.0f, 0.0f };                      ///< カメラ回転（オイラー角、ラジアン）

    float fov = 0.45f;                                             ///< 視野角（ラジアン）

    InterpolationType interpolation = InterpolationType::LINEAR;   ///< このキーフレームから次への補間方法

    CoordinateType coordinateType = CoordinateType::WORLD;         ///< 座標系タイプ（デフォルトはワールド座標）

    /// <summary>
    /// デフォルトコンストラクタ
    /// </summary>
    CameraKeyframe() = default;

    /// <summary>
    /// パラメータ指定コンストラクタ
    /// </summary>
    CameraKeyframe(float t, const Tako::Vector3& pos, const Tako::Vector3& rot, float f,
        InterpolationType interp = InterpolationType::LINEAR,
        CoordinateType coordType = CoordinateType::WORLD)
        : time(t), position(pos), rotation(rot), fov(f), interpolation(interp), coordinateType(coordType) {
    }
};

/// <summary>
/// JSON 変換用のヘルパー関数
/// </summary>
namespace nlohmann {
    template <>
    struct adl_serializer<CameraKeyframe::CoordinateType> {
        static void to_json(json& j, const CameraKeyframe::CoordinateType& type) {
            switch (type) {
            case CameraKeyframe::CoordinateType::WORLD:
                j = "WORLD";
                break;
            case CameraKeyframe::CoordinateType::TARGET_RELATIVE:
                j = "TARGET_RELATIVE";
                break;
            }
        }

        static void from_json(const json& j, CameraKeyframe::CoordinateType& type) {
            std::string str = j.get<std::string>();
            if (str == "WORLD") {
                type = CameraKeyframe::CoordinateType::WORLD;
            }
            else if (str == "TARGET_RELATIVE") {
                type = CameraKeyframe::CoordinateType::TARGET_RELATIVE;
            }
            else {
                type = CameraKeyframe::CoordinateType::WORLD; // デフォルト
            }
        }
    };

    template <>
    struct adl_serializer<CameraKeyframe::InterpolationType> {
        static void to_json(json& j, const CameraKeyframe::InterpolationType& type) {
            switch (type) {
            case CameraKeyframe::InterpolationType::LINEAR:
                j = "LINEAR";
                break;
            case CameraKeyframe::InterpolationType::EASE_IN:
                j = "EASE_IN";
                break;
            case CameraKeyframe::InterpolationType::EASE_OUT:
                j = "EASE_OUT";
                break;
            case CameraKeyframe::InterpolationType::EASE_IN_OUT:
                j = "EASE_IN_OUT";
                break;
            case CameraKeyframe::InterpolationType::CUBIC_BEZIER:
                j = "CUBIC_BEZIER";
                break;
            }
        }

        static void from_json(const json& j, CameraKeyframe::InterpolationType& type) {
            std::string str = j.get<std::string>();
            if (str == "LINEAR") {
                type = CameraKeyframe::InterpolationType::LINEAR;
            }
            else if (str == "EASE_IN") {
                type = CameraKeyframe::InterpolationType::EASE_IN;
            }
            else if (str == "EASE_OUT") {
                type = CameraKeyframe::InterpolationType::EASE_OUT;
            }
            else if (str == "EASE_IN_OUT") {
                type = CameraKeyframe::InterpolationType::EASE_IN_OUT;
            }
            else if (str == "CUBIC_BEZIER") {
                type = CameraKeyframe::InterpolationType::CUBIC_BEZIER;
            }
            else {
                type = CameraKeyframe::InterpolationType::LINEAR; // デフォルト
            }
        }
    };

    template <>
    struct adl_serializer<CameraKeyframe> {
        static void to_json(json& j, const CameraKeyframe& keyframe) {
            j = json{
                {"time", keyframe.time},
                {"position", {keyframe.position.x, keyframe.position.y, keyframe.position.z}},
                {"rotation", {keyframe.rotation.x, keyframe.rotation.y, keyframe.rotation.z}},
                {"fov", keyframe.fov},
                {"interpolation", keyframe.interpolation},
                {"coordinateType", keyframe.coordinateType}
            };
        }

        static void from_json(const json& j, CameraKeyframe& keyframe) {
            keyframe.time = j.at("time").get<float>();

            auto pos = j.at("position");
            keyframe.position.x = pos[0].get<float>();
            keyframe.position.y = pos[1].get<float>();
            keyframe.position.z = pos[2].get<float>();

            auto rot = j.at("rotation");
            keyframe.rotation.x = rot[0].get<float>();
            keyframe.rotation.y = rot[1].get<float>();
            keyframe.rotation.z = rot[2].get<float>();

            keyframe.fov = j.at("fov").get<float>();
            keyframe.interpolation = j.at("interpolation").get<CameraKeyframe::InterpolationType>();

            // coordinateType が存在しない場合は WORLD をデフォルトとする
            if (j.contains("coordinateType")) {
                keyframe.coordinateType = j.at("coordinateType").get<CameraKeyframe::CoordinateType>();
            }
            else {
                keyframe.coordinateType = CameraKeyframe::CoordinateType::WORLD;
            }
        }
    };
}