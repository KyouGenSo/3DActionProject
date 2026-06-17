#include "InputHandler.h"
#include "Object/Player/Player.h"
#include "Input.h"
#include "Vector2.h"
#include "GlobalVariables.h"

using namespace Tako;

InputHandler::InputHandler()
{
}

InputHandler::~InputHandler()
{
}

void InputHandler::Initialize()
{
  isMoving_ = false;
  isDashing_ = false;
  isAttacking_ = false;
  isShooting_ = false;
  isParrying_ = false;
  isPaused_ = false;
  moveDirection_ = Vector2(0.0f, 0.0f);
}

void InputHandler::Update()
{
  Input* input = Input::GetInstance();

  moveDirection_ = Vector2(0.0f, 0.0f);
  aimDirection_ = Vector2(0.0f, 0.0f);

  // 移動入力（左スティック + キーボード）
  if (input->IsConnect()) moveDirection_ += input->GetLeftStick();

  moveDirection_ += {static_cast<float>(input->PushKey(DIK_D) - input->PushKey(DIK_A)), static_cast<float>(input->PushKey(DIK_W) - input->PushKey(DIK_S)) };

  // 照準入力（右スティック）
  bool hasRightStickInput = false;
  if (input->IsConnect()) {
    hasRightStickInput = !input->RStickInDeadZone();
    if (hasRightStickInput) {
      Vector2 rightStick = input->GetRightStick();
      aimDirection_ = rightStick.Normalize();
    }
  }

  isMoving_ = !input->LStickInDeadZone() || moveDirection_.Length() > 0.0f;
  isDashing_ = input->TriggerKey(DIK_SPACE) || input->TriggerButton(GamepadButton::A);
  isAttacking_ = input->TriggerKey(DIK_Z) || input->TriggerButton(GamepadButton::X);
  isShooting_ = hasRightStickInput;
  isParrying_ = input->TriggerKey(DIK_F) || input->TriggerButton(GamepadButton::B);
  isPaused_ = input->TriggerKey(DIK_ESCAPE) || input->TriggerButton(GamepadButton::Start);
}

void InputHandler::ResetInputs()
{
    moveDirection_ = Vector2(0.0f, 0.0f);
    isMoving_ = false;
    isDashing_ = false;
    isAttacking_ = false;
    isShooting_ = false;
    isParrying_ = false;
    isPaused_ = false;
}

bool InputHandler::IsMoving() const
{
  return isMoving_;
}

bool InputHandler::IsDashing() const
{
  return isDashing_;
}

bool InputHandler::IsAttacking() const
{
  return isAttacking_;
}

bool InputHandler::IsShooting() const
{
  return isShooting_;
}

bool InputHandler::IsParrying() const
{
  return isParrying_;
}

bool InputHandler::IsPaused() const
{
  return isPaused_;
}

Vector2 InputHandler::GetMoveDirection() const
{
  return moveDirection_;
}

Vector2 InputHandler::GetAimDirection() const
{
  return aimDirection_;
}