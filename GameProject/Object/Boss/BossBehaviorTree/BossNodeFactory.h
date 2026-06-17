#pragma once

/// <summary>
/// Boss 専用 BT ノードの型・メタ情報を Tako::BTNodeRegistry に登録するファクトリ
/// </summary>
class BossNodeFactory {
public:
  /// <summary>
  /// 全 Boss 専用ノードを一括登録する。重複登録は上書きされるため複数回呼んでも安全
  /// </summary>
  static void RegisterAll();
};
