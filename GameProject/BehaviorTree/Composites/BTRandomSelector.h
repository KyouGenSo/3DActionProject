#pragma once
#include "../Core/BTComposite.h"
#include "../Core/BTBlackboard.h"
#include <vector>
#include <optional>

/// <summary>
/// ランダムセレクターノード
/// 子ノードをランダムな順序で実行し、最初に成功したところで停止
/// </summary>
class BTRandomSelector : public BTComposite {
public:
    /// <summary>
    /// コンストラクタ
    /// </summary>
    BTRandomSelector();

    /// <summary>
    /// デストラクタ
    /// </summary>
    virtual ~BTRandomSelector() = default;

    /// <summary>
    /// ノードの実行
    /// </summary>
    /// <param name="blackboard">ブラックボード</param>
    /// <returns>実行結果</returns>
    BTNodeStatus Execute(BTBlackboard* blackboard) override;

    /// <summary>
    /// 状態のリセット
    /// </summary>
    void Reset() override;

private: // プライベートメンバー関数
    /// <summary>
    /// 子ノードのインデックスをシャッフル
    /// </summary>
    void ShuffleIndices();

private:
    // シャッフルされたインデックス
    std::vector<size_t> shuffledIndices_;

    // 現在のシャッフル済みインデックス位置（Running 状態の継続用）
    size_t currentShuffledIdx_ = 0;

    // シャッフルが必要かどうか（新しい選択サイクル開始時に true）
    bool needsShuffle_ = true;

    // 前回成功した子ノードのインデックス（連続選択防止用、Reset で保持）
    std::optional<size_t> lastSuccessIdx_;
};
