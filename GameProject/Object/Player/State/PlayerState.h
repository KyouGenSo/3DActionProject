#pragma once
#include <string>

class Player;
class PlayerStateMachine;

/// <summary>
/// プレイヤーステート基底クラス
/// </summary>
class PlayerState
{
public:
	PlayerState(const std::string& name) : stateName_(name) {}

	virtual ~PlayerState() = default;

	virtual void Enter(Player* player) = 0;

	virtual void Update(Player* player, float deltaTime) = 0;

	virtual void Exit(Player* player) = 0;

	virtual void HandleInput(Player* player) {}

	/// <summary>
	/// 指定状態へ遷移可能か（既定では常に許可）
	/// </summary>
	/// <param name="stateName">遷移先の状態名</param>
	/// <returns>遷移を許可するなら true</returns>
	virtual bool CanTransitionTo(const std::string& stateName) const { return true; }

	const std::string& GetName() const { return stateName_; }

	virtual void DrawImGui(Player* player) {}

protected:
	/// <summary>
	/// ステートマシン経由で別状態へ遷移する（両引数が有効な場合のみ）
	/// </summary>
	/// <param name="stateMachine">遷移を実行するステートマシン</param>
	/// <param name="newState">遷移先の状態</param>
	void ChangeState(PlayerStateMachine* stateMachine, PlayerState* newState);

private:
	std::string stateName_;
};