#include "ResultScene.h"
#include "LineRenderer.h"
#include "GPUParticle.h"
#include "Input.h"
#include "Object3dBasic.h"
#include "SceneManager.h"
#include "Sprite.h"
#include "SpriteBasic.h"
#include "EnginePaths.h"

#ifdef _DEBUG
#include"ImGui.h"
#include "DebugCamera.h"
#endif

using namespace Tako;

ResultScene::ResultScene(const std::string& titleTexture)
    : titleTexture_(titleTexture)
{
}

void ResultScene::Initialize()
{
#ifdef _DEBUG
    DebugCamera::GetInstance()->Initialize();
    Object3dBasic::GetInstance()->SetDebug(false);
    LineRenderer::GetInstance()->SetDebug(false);
    GPUParticle::GetInstance()->SetIsDebug(false);
#endif

    /// ================================== ///
    ///              初期化処理              ///
    /// ================================== ///

    (*Object3dBasic::GetInstance()->GetCamera())->SetRotate(Vector3(0.2f, 0.0f, 0.0f));
    (*Object3dBasic::GetInstance()->GetCamera())->SetTranslate(Vector3(0.0f, cameraHiddenY_, -34.0f));

    backGround_ = std::make_unique<Sprite>();
    backGround_->Initialize(EnginePaths::TexturePath("black.dds"));
    backGround_->SetPos(Vector2(0.f, 0.f));
    backGround_->SetSize(Vector2(static_cast<float>(WinApp::clientWidth), static_cast<float>(WinApp::clientHeight)));

    titleText_ = std::make_unique<Sprite>();
    titleText_->Initialize(titleTexture_);
    titleText_->SetPos(Vector2(WinApp::clientWidth / 2.f - titleTextHalfWidth_, titleTextY_));

    pressButtonText_ = std::make_unique<Sprite>();
    pressButtonText_->Initialize("titlescene_button.dds");
    pressButtonText_->SetPos(Vector2(
        WinApp::clientWidth / 2.f - pressButtonText_->GetSize().x / 2.f,
        WinApp::clientHeight - buttonBottomOffset_));

}


void ResultScene::Finalize()
{

}

void ResultScene::Update()
{
    /// ================================== ///
    ///              更新処理               ///
    /// ================================== ///

    backGround_->Update();
    titleText_->Update();
    pressButtonText_->Update();

    if (Input::GetInstance()->TriggerKey(DIK_SPACE))
    {
        SceneManager::GetInstance()->ChangeScene("title");
    }

    if (Input::GetInstance()->TriggerButton(GamepadButton::A)) {
        SceneManager::GetInstance()->ChangeScene("title");
    }
}

void ResultScene::Draw()
{
    /// ================================== ///
    ///              描画処理               ///
    /// ================================== ///
    //------------------背景 Sprite の描画------------------//
    SpriteBasic::GetInstance()->SetCommonRenderSetting();

    backGround_->Draw();


    //-------------------Model の描画-------------------//
    Object3dBasic::GetInstance()->SetCommonRenderSetting();




    //------------------前景 Sprite の描画------------------//
    SpriteBasic::GetInstance()->SetCommonRenderSetting();

    titleText_->Draw();

    pressButtonText_->Draw();

}

void ResultScene::DrawWithoutEffect()
{
    /// ================================== ///
    ///              描画処理               ///
    /// ================================== ///

    //------------------背景 Sprite の描画------------------//
    SpriteBasic::GetInstance()->SetCommonRenderSetting();




    //-------------------Model の描画-------------------//
    Object3dBasic::GetInstance()->SetCommonRenderSetting();





    //------------------前景 Sprite の描画------------------//
    SpriteBasic::GetInstance()->SetCommonRenderSetting();




}

void ResultScene::DrawImGui()
{
#ifdef _DEBUG

    /// ================================== ///
    ///             ImGui の描画              ///
    /// ================================== ///


#endif // _DEBUG
}
