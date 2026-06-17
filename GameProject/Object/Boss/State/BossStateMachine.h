#pragma once
#include <memory>
#include <unordered_map>
#include <string>

class Boss;
class BossState;

/// <summary>
/// 外部イベント駆動の状態（スタン・離脱等）を管理。AI 意思決定は BehaviorTree が担う
/// </summary>
class BossStateMachine {
public:
	BossStateMachine(Boss* boss);

	~BossStateMachine();

	void Update(float deltaTime);

	/// <summary>
	/// 指定名のステートへ遷移。未登録名・現在と同じステートなら何もしない（Exit→Enter を呼ぶ）
	/// </summary>
	/// <param name="stateName">遷移先のステート登録名</param>
	void ChangeState(const std::string& stateName);

	/// <summary>
	/// ステートを登録名に紐付けて保持。同名は上書き
	/// </summary>
	/// <param name="name">遷移時に使う登録名</param>
	/// <param name="state">所有権を移すステート実体</param>
	void RegisterState(const std::string& name, std::unique_ptr<BossState> state);

	BossState* GetCurrentState() const { return currentState_; }

	const std::string& GetCurrentStateName() const;

	/// <summary>
	/// 登録名からステート実体を取得
	/// </summary>
	/// <param name="name">ステート登録名</param>
	/// <returns>該当ステート。未登録なら nullptr</returns>
	BossState* GetState(const std::string& name) const;

private:
	Boss* boss_;
	BossState* currentState_ = nullptr;
	std::unordered_map<std::string, std::unique_ptr<BossState>> states_;

	static const std::string kEmptyString;
};
