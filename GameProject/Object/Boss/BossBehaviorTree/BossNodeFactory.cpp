#include "BossNodeFactory.h"

#include "BTNodeRegistry.h"
#include "BTNodeMeta.h"
#include <memory>

// Action ノード (14 種)
#include "Actions/BTBossIdle.h"
#include "Actions/BTBossDash.h"
#include "Actions/BTBossShoot.h"
#include "Actions/BTBossRapidFire.h"
#include "Actions/BTBossWideShoot.h"
#include "Actions/BTBossMeleeAttack.h"
#include "Actions/BTBossApproach.h"
#include "Actions/BTBossRetreat.h"
#include "Actions/BTBossBarrage.h"
#include "Actions/BTBossAreaAttack.h"
#include "Actions/BTBossMeteorRain.h"
#include "Actions/BTBossSlashAttack.h"
#include "Actions/BTBossRepelShockwave.h"
#include "Actions/BTBossVortexTempest.h"
#include "Actions/BTBossTeleport.h"

// Condition ノード (4 種)
#include "Conditions/BTActionSelector.h"
#include "Conditions/BTBossPhaseCondition.h"
#include "Conditions/BTBossHPCondition.h"
#include "Conditions/BTBossDistanceCondition.h"

void BossNodeFactory::RegisterAll() {
  auto* reg = Tako::BTNodeRegistry::GetInstance();

  // 標準コンポジットを先に登録
  reg->Initialize();

  // ========================================================================
  //                              Action ノード 
  // ========================================================================
  reg->RegisterNode<BTBossIdle>("BTBossIdle", Tako::NodeMeta{
    "Idle", Tako::NodeCategory::Action,
    Tako::NodeColor(0.2f, 0.8f, 0.4f, 1.0f), false  // 緑
  });
  reg->RegisterNode<BTBossDash>("BTBossDash", Tako::NodeMeta{
    "Dash", Tako::NodeCategory::Action,
    Tako::NodeColor(0.3f, 0.8f, 0.5f, 1.0f), false  // 明るい緑
  });
  reg->RegisterNode<BTBossShoot>("BTBossShoot", Tako::NodeMeta{
    "Shoot", Tako::NodeCategory::Action,
    Tako::NodeColor(0.8f, 0.3f, 0.3f, 1.0f), false  // 赤
  });
  reg->RegisterNode<BTBossRapidFire>("BTBossRapidFire", Tako::NodeMeta{
    "Rapid Fire", Tako::NodeCategory::Action,
    Tako::NodeColor(0.9f, 0.2f, 0.5f, 1.0f), false  // マゼンタ
  });
  reg->RegisterNode<BTBossWideShoot>("BTBossWideShoot", Tako::NodeMeta{
    "Wide Shoot", Tako::NodeCategory::Action,
    Tako::NodeColor(0.95f, 0.3f, 0.6f, 1.0f), false  // ピンク
  });
  reg->RegisterNode<BTBossMeleeAttack>("BTBossMeleeAttack", Tako::NodeMeta{
    "Melee Attack", Tako::NodeCategory::Action,
    Tako::NodeColor(0.9f, 0.4f, 0.1f, 1.0f), false  // オレンジ-赤
  });
  reg->RegisterNode<BTBossApproach>("BTBossApproach", Tako::NodeMeta{
    "Approach", Tako::NodeCategory::Action,
    Tako::NodeColor(0.4f, 0.9f, 0.4f, 1.0f), false  // 明るい緑 (移動系)
  });
  reg->RegisterNode<BTBossRetreat>("BTBossRetreat", Tako::NodeMeta{
    "Retreat", Tako::NodeCategory::Action,
    Tako::NodeColor(0.3f, 0.7f, 0.9f, 1.0f), false  // 水色
  });
  reg->RegisterNode<BTBossBarrage>("BTBossBarrage", Tako::NodeMeta{
    "Barrage", Tako::NodeCategory::Action,
    Tako::NodeColor(0.7f, 0.2f, 0.9f, 1.0f), false  // 紫 (弾幕攻撃)
  });
  reg->RegisterNode<BTBossAreaAttack>("BTBossAreaAttack", Tako::NodeMeta{
    "Area Attack", Tako::NodeCategory::Action,
    Tako::NodeColor(0.9f, 0.1f, 0.1f, 1.0f), false  // 赤 (エリア攻撃)
  });
  reg->RegisterNode<BTBossMeteorRain>("BTBossMeteorRain", Tako::NodeMeta{
    "Meteor Rain", Tako::NodeCategory::Action,
    Tako::NodeColor(0.9f, 0.5f, 0.0f, 1.0f), false  // オレンジ (メテオ攻撃)
  });
  reg->RegisterNode<BTBossSlashAttack>("BTBossSlashAttack", Tako::NodeMeta{
    "Slash Attack", Tako::NodeCategory::Action,
    Tako::NodeColor(0.8f, 0.1f, 0.6f, 1.0f), false  // マゼンタ (斬撃攻撃)
  });
  reg->RegisterNode<BTBossRepelShockwave>("BTBossRepelShockwave", Tako::NodeMeta{
    "Repel Shockwave", Tako::NodeCategory::Action,
    Tako::NodeColor(0.4f, 0.7f, 1.0f, 1.0f), false  // 水色 (衝撃波)
  });
  reg->RegisterNode<BTBossVortexTempest>("BTBossVortexTempest", Tako::NodeMeta{
    "Vortex Tempest", Tako::NodeCategory::Action,
    Tako::NodeColor(0.6f, 0.3f, 0.9f, 1.0f), false  // 紫 (渦・嵐)
  });
  reg->RegisterNode<BTBossTeleport>("BTBossTeleport", Tako::NodeMeta{
    "Teleport", Tako::NodeCategory::Action,
    Tako::NodeColor(0.5f, 0.9f, 0.9f, 1.0f), false  // 水色 (瞬間移動)
  });

  // ========================================================================
  //                               Condition 
  // ========================================================================
  // BTActionSelector は引数付きコンストラクタ (ActionType::Dash) のため RegisterFactory を使用
  reg->RegisterFactory("BTActionSelector",
    [] { return std::static_pointer_cast<Tako::BTNode>(
      std::make_shared<BTActionSelector>(BTActionSelector::ActionType::Dash)); },
    Tako::NodeMeta{
      "Action Selector", Tako::NodeCategory::Condition,
      Tako::NodeColor(0.8f, 0.8f, 0.2f, 1.0f), false  // 黄色
    });
  reg->RegisterNode<BTBossPhaseCondition>("BTBossPhaseCondition", Tako::NodeMeta{
    "Phase Condition", Tako::NodeCategory::Condition,
    Tako::NodeColor(0.5f, 0.2f, 0.9f, 1.0f), false  // 紫
  });
  reg->RegisterNode<BTBossHPCondition>("BTBossHPCondition", Tako::NodeMeta{
    "HP Condition", Tako::NodeCategory::Condition,
    Tako::NodeColor(0.9f, 0.5f, 0.2f, 1.0f), false  // オレンジ
  });
  reg->RegisterNode<BTBossDistanceCondition>("BTBossDistanceCondition", Tako::NodeMeta{
    "Distance Condition", Tako::NodeCategory::Condition,
    Tako::NodeColor(0.2f, 0.7f, 0.5f, 1.0f), false
  });
}
