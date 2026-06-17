#pragma once
#include <memory>
#include <unordered_map>
#include <string>

class Player;
class PlayerState;

/// <summary>
/// プレイヤー状態管理マシン
/// </summary>
class PlayerStateMachine
{
public:
	PlayerStateMachine(Player* player);

	~PlayerStateMachine();

	void Initialize();

	void Update(float deltaTime);

	void HandleInput();

	/// <summary>
	/// 状態を名前で変更（未登録名は無視）
	/// </summary>
	/// <param name="stateName">遷移先の登録済み状態名</param>
	void ChangeState(const std::string& stateName);

	/// <summary>
	/// 状態をポインタで変更。現状態と同一・遷移不可・nullptr の場合は何もしない
	/// </summary>
	/// <param name="newState">遷移先の状態</param>
	void ChangeState(PlayerState* newState);

	/// <summary>
	/// 状態を名前付きで登録する（所有権を受け取る）
	/// </summary>
	/// <param name="name">状態名（ChangeState で指定するキー）</param>
	/// <param name="state">登録する状態インスタンス</param>
	void RegisterState(const std::string& name, std::unique_ptr<PlayerState> state);

	PlayerState* GetCurrentState() const { return currentState_; }

	/// <summary>
	/// 名前で状態を取得（なければ nullptr）
	/// </summary>
	/// <param name="name">取得する状態名</param>
	/// <returns>該当する状態。未登録なら nullptr</returns>
	PlayerState* GetState(const std::string& name) const;

	/// <summary>
	/// 登録済み状態名をソート済みで取得
	/// </summary>
	/// <returns>昇順ソート済みの登録状態名一覧</returns>
	std::vector<std::string> GetAllStateNames() const;

private:
	Player* player_;
	PlayerState* currentState_;
	PlayerState* previousState_;
	std::unordered_map<std::string, std::unique_ptr<PlayerState>> states_;
};