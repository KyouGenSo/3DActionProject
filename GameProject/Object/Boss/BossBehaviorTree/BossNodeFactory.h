#pragma once

/// <summary>
/// Boss 専用ビヘイビアツリーノードを Tako::BTNodeRegistry に登録するためのファクトリ。
/// 各 BTBoss* / 条件ノードの型情報・メタ情報 (表示名・カラー・カテゴリ) を Registry に登録する。
/// </summary>
class BossNodeFactory {
public:
  /// <summary>
  /// Boss 専用ノード (Action 14 種 + Condition 4 種) を Tako::BTNodeRegistry に一括登録する。
  /// アプリ起動時 (Boss::InitializeAI 等) で 1 回呼び出すこと。
  /// 重複登録は Registry 側で上書き処理されるため、複数回呼んでも安全。
  /// </summary>
  static void RegisterAll();
};
