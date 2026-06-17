#include "ParryState.h"
#include "PlayerStateMachine.h"
#include "../Player.h"
#include "Input/InputHandler.h"
#include "GlobalVariables.h"
#include "EmitterManager.h"
#include <algorithm>  // for std::min
#ifdef _DEBUG
#include "ImGuiManager.h"
#endif

using namespace Tako;

void ParryState::Enter(Player* player)
{
    // TODO: アニメーション作成後に実装
    // player->GetModel()->PlayAnimation("Parry");

    parryTimer_ = 0.0f;

    EmitterManager* em = player->GetEmitterManager();
    if (em) {
        em->SetEmitterPosition("parry_effect", player->GetTranslate());
        em->SetEmitterActive("parry_effect", true);
    }
}

void ParryState::Update(Player* player, float deltaTime)
{
    GlobalVariables* gv = GlobalVariables::GetInstance();
    parryDuration_ = gv->GetValueFloat("ParryState", "ParryDuration");

    parryTimer_ += deltaTime;

    // エフェクトをプレイヤー前方に追従
    EmitterManager* em = player->GetEmitterManager();
    if (em) {
        Vector3 effectPos = player->GetFrontPosition(0.0f);
        em->SetEmitterPosition("parry_effect", effectPos);
    }

    if (parryTimer_ >= parryDuration_) {
        PlayerStateMachine* stateMachine = player->GetStateMachine();
        if (stateMachine) {
            stateMachine->ChangeState("Idle");
        }
    }
}

void ParryState::Exit(Player* player)
{
    parryTimer_ = 0.0f;

    EmitterManager* em = player->GetEmitterManager();
    if (em) {
        em->SetEmitterActive("parry_effect", false);
    }

    player->StartParryCooldown();
}

void ParryState::HandleInput(Player* player)
{
    // パリィ中は入力を受け付けない
}

void ParryState::OnParrySuccess(Player* player)
{
    PlayerStateMachine* stateMachine = player->GetStateMachine();
    if (stateMachine) {
        stateMachine->ChangeState("Attack");
    }
}

void ParryState::DrawImGui(Player* player)
{
#ifdef _DEBUG
    ImGui::Text("=== Parry State Details ===");
    ImGui::Separator();

    ImGui::Text("Parry Timer: %.3f / %.3f", GetParryTimer(), GetParryDuration());

    float totalProgress = (GetParryDuration() > 0.0f) ? (GetParryTimer() / GetParryDuration()) : 0.0f;
    ImGui::Text("Total Progress:");
    ImGui::ProgressBar(totalProgress);

    if (ImGui::TreeNode("Parry Parameters")) {
        float parryDuration = GetParryDuration();
        if (ImGui::SliderFloat("Parry Duration", &parryDuration, 0.2f, 2.0f, "%.2f sec")) SetParryDuration(parryDuration);

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("Frame Data")) {
        ImGui::Text("Total Frames: %d", (int)(GetParryDuration() * 60.0f));
        ImGui::Text("Current Frame: %d", (int)(GetParryTimer() * 60.0f));
        ImGui::TreePop();
    }
#endif
}