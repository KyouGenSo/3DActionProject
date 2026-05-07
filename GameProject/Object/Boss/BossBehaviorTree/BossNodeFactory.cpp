// ===== 全ビルドで使用可能なコード =====

#include "BossNodeFactory.h"

// BehaviorTree ノードのインクルード
#include "../../../BehaviorTree/Composites/BTParallel.h"
#include "../../../BehaviorTree/Composites/BTSelector.h"
#include "../../../BehaviorTree/Composites/BTSequence.h"
#include "../../../BehaviorTree/Composites/BTRandomSelector.h"
#include "../BossBehaviorTree/Actions/BTBossIdle.h"
#include "../BossBehaviorTree/Actions/BTBossDash.h"
#include "../BossBehaviorTree/Actions/BTBossShoot.h"
#include "../BossBehaviorTree/Actions/BTBossRapidFire.h"
#include "../BossBehaviorTree/Actions/BTBossWideShoot.h"
#include "../BossBehaviorTree/Actions/BTBossMeleeAttack.h"
#include "../BossBehaviorTree/Actions/BTBossApproach.h"
#include "../BossBehaviorTree/Actions/BTBossRetreat.h"
#include "../BossBehaviorTree/Actions/BTBossBarrage.h"
#include "../BossBehaviorTree/Actions/BTBossAreaAttack.h"
#include "../BossBehaviorTree/Actions/BTBossMeteorRain.h"
#include "../BossBehaviorTree/Actions/BTBossSlashAttack.h"
#include "../BossBehaviorTree/Actions/BTBossRepelShockwave.h"
#include "../BossBehaviorTree/Actions/BTBossVortexTempest.h"
#include "../BossBehaviorTree/Conditions/BTActionSelector.h"
#include "../BossBehaviorTree/Conditions/BTBossPhaseCondition.h"
#include "../BossBehaviorTree/Conditions/BTBossHPCondition.h"
#include "../BossBehaviorTree/Conditions/BTBossDistanceCondition.h"

/// <summary>
/// ノードの生成
/// </summary>
BTNodePtr BossNodeFactory::CreateNode(const std::string& nodeType) {
    // Composite ノード
    if (nodeType == "BTSelector") {
        return std::make_shared<BTSelector>();
    }
    else if (nodeType == "BTSequence") {
        return std::make_shared<BTSequence>();
    }
    else if (nodeType == "BTRandomSelector") {
        return std::make_shared<BTRandomSelector>();
    }
    else if (nodeType == "BTParallel") {
        return std::make_shared<BTParallel>();  // 既定 policy = MainChild
    }
    // Action ノード（Blackboard 経由で Boss/Player にアクセス）
    else if (nodeType == "BTBossIdle") {
        return std::make_shared<BTBossIdle>();
    }
    else if (nodeType == "BTBossDash") {
        return std::make_shared<BTBossDash>();
    }
    else if (nodeType == "BTBossShoot") {
        return std::make_shared<BTBossShoot>();
    }
    else if (nodeType == "BTBossRapidFire") {
        return std::make_shared<BTBossRapidFire>();
    }
    else if (nodeType == "BTBossWideShoot") {
        return std::make_shared<BTBossWideShoot>();
    }
    else if (nodeType == "BTBossMeleeAttack") {
        return std::make_shared<BTBossMeleeAttack>();
    }
    else if (nodeType == "BTBossApproach") {
        return std::make_shared<BTBossApproach>();
    }
    else if (nodeType == "BTBossRetreat") {
        return std::make_shared<BTBossRetreat>();
    }
    else if (nodeType == "BTBossBarrage") {
        return std::make_shared<BTBossBarrage>();
    }
    else if (nodeType == "BTBossAreaAttack") {
        return std::make_shared<BTBossAreaAttack>();
    }
    else if (nodeType == "BTBossMeteorRain") {
        return std::make_shared<BTBossMeteorRain>();
    }
    else if (nodeType == "BTBossSlashAttack") {
        return std::make_shared<BTBossSlashAttack>();
    }
    else if (nodeType == "BTBossRepelShockwave") {
        return std::make_shared<BTBossRepelShockwave>();
    }
    else if (nodeType == "BTBossVortexTempest") {
        return std::make_shared<BTBossVortexTempest>();
    }
    // Condition ノード
    else if (nodeType == "BTActionSelector") {
        return std::make_shared<BTActionSelector>(BTActionSelector::ActionType::Dash);
    }
    else if (nodeType == "BTBossPhaseCondition") {
        return std::make_shared<BTBossPhaseCondition>();
    }
    else if (nodeType == "BTBossHPCondition") {
        return std::make_shared<BTBossHPCondition>();
    }
    else if (nodeType == "BTBossDistanceCondition") {
        return std::make_shared<BTBossDistanceCondition>();
    }

    return nullptr;
}

/// <summary>
/// Boss/Player の依存関係を持つノードの生成
/// </summary>
BTNodePtr BossNodeFactory::CreateNodeWithDependencies(
    const std::string& nodeType,
    [[maybe_unused]] Boss* boss,
    [[maybe_unused]] Player* player) {

    // 現在は全ノードが Blackboard 経由で参照するため、CreateNode と同じ
    return CreateNode(nodeType);
}

// ===== デバッグビルドのみのコード =====

#ifdef _DEBUG

#include <typeinfo>
#include <algorithm>

// 静的メンバの定義
std::vector<BossNodeFactory::NodeTypeInfo> BossNodeFactory::nodeTypes_;
bool BossNodeFactory::initialized_ = false;

/// <summary>
/// ノードタイプ情報の初期化
/// </summary>
void BossNodeFactory::InitializeNodeTypes() {
    if (initialized_) return;

    // ノードタイプ情報の登録
    nodeTypes_ = {
        // ========== Composite ノード ==========
        {
            "BTSelector",
            "Selector",
            NodeCategory::Composite,
            ImVec4(0.8f, 0.4f, 0.2f, 1.0f),  // オレンジ
            true  // 子ノードを持てる
        },
        {
            "BTSequence",
            "Sequence",
            NodeCategory::Composite,
            ImVec4(0.2f, 0.6f, 0.8f, 1.0f),  // 青
            true  // 子ノードを持てる
        },
        {
            "BTRandomSelector",
            "Random Selector",
            NodeCategory::Composite,
            ImVec4(0.9f, 0.6f, 0.2f, 1.0f),  // 明るいオレンジ
            true  // 子ノードを持てる
        },
        {
            "BTParallel",
            "Parallel",
            NodeCategory::Composite,
            ImVec4(0.5f, 0.5f, 0.9f, 1.0f),  // 紫味の青（並列実行）
            true  // 子ノードを持てる
        },

        // ========== Action ノード ==========
        {
            "BTBossIdle",
            "Idle",
            NodeCategory::Action,
            ImVec4(0.2f, 0.8f, 0.4f, 1.0f),  // 緑
            false  // 子ノードを持てない
        },
        {
            "BTBossDash",
            "Dash",
            NodeCategory::Action,
            ImVec4(0.3f, 0.8f, 0.5f, 1.0f),  // 明るい緑
            false
        },
        {
            "BTBossShoot",
            "Shoot",
            NodeCategory::Action,
            ImVec4(0.8f, 0.3f, 0.3f, 1.0f),  // 赤
            false
        },
        {
            "BTBossRapidFire",
            "Rapid Fire",
            NodeCategory::Action,
            ImVec4(0.9f, 0.2f, 0.5f, 1.0f),  // マゼンタ
            false
        },
        {
            "BTBossWideShoot",
            "Wide Shoot",
            NodeCategory::Action,
            ImVec4(0.95f, 0.3f, 0.6f, 1.0f),  // ピンク
            false
        },
        {
            "BTBossMeleeAttack",
            "Melee Attack",
            NodeCategory::Action,
            ImVec4(0.9f, 0.4f, 0.1f, 1.0f),  // オレンジ-赤
            false
        },
        {
            "BTBossApproach",
            "Approach",
            NodeCategory::Action,
            ImVec4(0.4f, 0.9f, 0.4f, 1.0f),  // 明るい緑（移動系）
            false
        },
        {
            "BTBossRetreat",
            "Retreat",
            NodeCategory::Action,
            ImVec4(0.3f, 0.7f, 0.9f, 1.0f),  // 水色（Approach と対になる色）
            false
        },
        {
            "BTBossBarrage",
            "Barrage",
            NodeCategory::Action,
            ImVec4(0.7f, 0.2f, 0.9f, 1.0f),  // 紫（弾幕攻撃）
            false
        },
        {
            "BTBossAreaAttack",
            "Area Attack",
            NodeCategory::Action,
            ImVec4(0.9f, 0.1f, 0.1f, 1.0f),  // 赤（エリア攻撃）
            false
        },
        {
            "BTBossMeteorRain",
            "Meteor Rain",
            NodeCategory::Action,
            ImVec4(0.9f, 0.5f, 0.0f, 1.0f),  // オレンジ（メテオ攻撃）
            false
        },
        {
            "BTBossSlashAttack",
            "Slash Attack",
            NodeCategory::Action,
            ImVec4(0.8f, 0.1f, 0.6f, 1.0f),  // マゼンタ（斬撃攻撃）
            false
        },
        {
            "BTBossRepelShockwave",
            "Repel Shockwave",
            NodeCategory::Action,
            ImVec4(0.4f, 0.7f, 1.0f, 1.0f),  // 水色（衝撃波）
            false
        },
        {
            "BTBossVortexTempest",
            "Vortex Tempest",
            NodeCategory::Action,
            ImVec4(0.6f, 0.3f, 0.9f, 1.0f),  // 紫（渦・嵐）
            false
        },

        // ========== Condition ノード ==========
        {
            "BTActionSelector",
            "Action Selector",
            NodeCategory::Condition,
            ImVec4(0.8f, 0.8f, 0.2f, 1.0f),  // 黄色
            false
        },
        {
            "BTBossPhaseCondition",
            "Phase Condition",
            NodeCategory::Condition,
            ImVec4(0.5f, 0.2f, 0.9f, 1.0f),  // 紫
            false
        },
        {
            "BTBossHPCondition",
            "HP Condition",
            NodeCategory::Condition,
            ImVec4(0.9f, 0.5f, 0.2f, 1.0f),  // オレンジ
            false
        },
        {
            "BTBossDistanceCondition",
            "Distance Condition",
            NodeCategory::Condition,
            ImVec4(0.2f, 0.7f, 0.5f, 1.0f),  // 緑
            false
        }
    };

    initialized_ = true;
}

/// <summary>
/// 利用可能なノードタイプ一覧の取得
/// </summary>
std::vector<std::string> BossNodeFactory::GetAvailableNodeTypes() {
    InitializeNodeTypes();

    std::vector<std::string> types;
    for (const auto& nodeType : nodeTypes_) {
        types.push_back(nodeType.typeName);
    }
    return types;
}

/// <summary>
/// カテゴリごとのノードタイプ取得
/// </summary>
std::vector<std::string> BossNodeFactory::GetNodeTypesByCategory(NodeCategory category) {
    InitializeNodeTypes();

    std::vector<std::string> types;
    for (const auto& nodeType : nodeTypes_) {
        if (nodeType.category == category) {
            types.push_back(nodeType.typeName);
        }
    }
    return types;
}

/// <summary>
/// ノードタイプの取得（逆引き）
/// </summary>
std::string BossNodeFactory::GetNodeType(const BTNodePtr& node) {
    if (!node) return "";

    // RTTI を使用してタイプを判定
    const std::type_info& typeInfo = typeid(*node);

    // 各タイプと比較
    if (typeInfo == typeid(BTSelector)) return "BTSelector";
    if (typeInfo == typeid(BTSequence)) return "BTSequence";
    if (typeInfo == typeid(BTRandomSelector)) return "BTRandomSelector";
    if (typeInfo == typeid(BTParallel)) return "BTParallel";
    if (typeInfo == typeid(BTBossIdle)) return "BTBossIdle";
    if (typeInfo == typeid(BTBossDash)) return "BTBossDash";
    if (typeInfo == typeid(BTBossShoot)) return "BTBossShoot";
    if (typeInfo == typeid(BTBossRapidFire)) return "BTBossRapidFire";
    if (typeInfo == typeid(BTBossWideShoot)) return "BTBossWideShoot";
    if (typeInfo == typeid(BTBossMeleeAttack)) return "BTBossMeleeAttack";
    if (typeInfo == typeid(BTBossApproach)) return "BTBossApproach";
    if (typeInfo == typeid(BTBossRetreat)) return "BTBossRetreat";
    if (typeInfo == typeid(BTBossBarrage)) return "BTBossBarrage";
    if (typeInfo == typeid(BTBossAreaAttack)) return "BTBossAreaAttack";
    if (typeInfo == typeid(BTBossMeteorRain)) return "BTBossMeteorRain";
    if (typeInfo == typeid(BTBossSlashAttack)) return "BTBossSlashAttack";
    if (typeInfo == typeid(BTBossRepelShockwave)) return "BTBossRepelShockwave";
    if (typeInfo == typeid(BTBossVortexTempest)) return "BTBossVortexTempest";
    if (typeInfo == typeid(BTActionSelector)) return "BTActionSelector";
    if (typeInfo == typeid(BTBossPhaseCondition)) return "BTBossPhaseCondition";
    if (typeInfo == typeid(BTBossHPCondition)) return "BTBossHPCondition";
    if (typeInfo == typeid(BTBossDistanceCondition)) return "BTBossDistanceCondition";

    return "";
}

/// <summary>
/// ノードタイプ情報の取得
/// </summary>
BossNodeFactory::NodeTypeInfo BossNodeFactory::GetNodeTypeInfo(const std::string& nodeType) {
    InitializeNodeTypes();

    for (const auto& info : nodeTypes_) {
        if (info.typeName == nodeType) {
            return info;
        }
    }

    // デフォルト値を返す
    NodeTypeInfo defaultInfo;
    defaultInfo.typeName = nodeType;
    defaultInfo.displayName = nodeType;
    defaultInfo.category = NodeCategory::Action;
    defaultInfo.color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    defaultInfo.isComposite = false;
    return defaultInfo;
}

/// <summary>
/// ノードの表示名を取得
/// </summary>
std::string BossNodeFactory::GetNodeDisplayName(const std::string& nodeType) {
    NodeTypeInfo info = GetNodeTypeInfo(nodeType);
    return info.displayName;
}

/// <summary>
/// ノードの色を取得
/// </summary>
ImVec4 BossNodeFactory::GetNodeColor(const std::string& nodeType) {
    NodeTypeInfo info = GetNodeTypeInfo(nodeType);
    return info.color;
}

/// <summary>
/// コンポジットノードかどうか判定
/// </summary>
bool BossNodeFactory::IsCompositeNode(const std::string& nodeType) {
    NodeTypeInfo info = GetNodeTypeInfo(nodeType);
    return info.isComposite;
}

/// <summary>
/// ノードカテゴリを取得
/// </summary>
BossNodeFactory::NodeCategory BossNodeFactory::GetNodeCategory(const std::string& nodeType) {
    NodeTypeInfo info = GetNodeTypeInfo(nodeType);
    return info.category;
}

#endif // _DEBUG
