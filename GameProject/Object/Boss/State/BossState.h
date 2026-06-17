#pragma once
#include <string>

class Boss;

/// <summary>
/// ボスステート基底（Enter/Update/Exit インターフェース）
/// </summary>
class BossState {
public: //メンバー関数
	BossState(const std::string& name) : stateName_(name) {}

	virtual ~BossState() = default;

	virtual void Enter(Boss* boss) = 0;

	virtual void Update(Boss* boss, float deltaTime) = 0;

	virtual void Exit(Boss* boss) = 0;

	//=============================
	//Getter
	//=============================
	const std::string& GetName() const { return stateName_; }

protected: //メンバー変数
	std::string stateName_;
};
