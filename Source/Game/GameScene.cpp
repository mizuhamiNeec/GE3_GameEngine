#include "GameScene.h"

#include "../Engine.h"
#include "../Engine/Lib/Console/ConVarManager.h"
#include "../Engine/Lib/Math/Quaternion/Quaternion.h"
#include "../Engine/Lib/Math/Random/Random.h"
#include "../Engine/Lib/Timer/EngineTimer.h"
#include "../Engine/Model/ModelManager.h"
#include "../ImGuiManager/ImGuiManager.h"
#include "../Lib/Math/MathLib.h"
#include "../Object3D/Object3D.h"
#include "../Particle/ParticleCommon.h"
#include "../Sprite/SpriteCommon.h"
#include "../TextureManager/TextureManager.h"
#include "../Debug/Debug.h"

void GameScene::Init(
	D3D12* renderer, Window* window,
	SpriteCommon* spriteCommon, Object3DCommon* object3DCommon,
	ModelCommon* modelCommon, ParticleCommon* particleCommon,
	EngineTimer* engineTimer
) {
	renderer_ = renderer;
	window_ = window;
	spriteCommon_ = spriteCommon;
	object3DCommon_ = object3DCommon;
	modelCommon_ = modelCommon;
	particleCommon_ = particleCommon;
	timer_ = engineTimer;

#pragma region テクスチャ読み込み
	TextureManager::GetInstance()->LoadTexture("./Resources/Textures/empty.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/Textures/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("./Resources/Textures/circle.png");
#pragma endregion

#pragma region スプライト類
	sprite_ = std::make_unique<Sprite>();
	sprite_->Init(spriteCommon_, "./Resources/Textures/uvChecker.png");
	sprite_->SetSize({512.0f, 512.0f, 0.0f});
#pragma endregion

#pragma region 3Dオブジェクト類
	// .objファイルからモデルを読み込む
	ModelManager::GetInstance()->LoadModel("reflectionTest.obj");

	object3D_ = std::make_unique<Object3D>();
	object3D_->Init(object3DCommon_);
	// 初期化済みの3Dオブジェクトにモデルを紐づける
	object3D_->SetModel("reflectionTest.obj");
	object3D_->SetPos(Vec3::zero);
#pragma endregion

#pragma region パーティクル類
	particle_ = std::make_unique<ParticleObject>();
	particle_->Init(particleCommon_, "./Resources/Textures/circle.png");
#pragma endregion
}

void GameScene::Update() {
	sprite_->Update();
	//object3D_->Update();
	particle_->Update(EngineTimer::GetScaledDeltaTime());

#ifdef _DEBUG

	// // ライン描画
	// {
	// 	static Vec3 pos;
	// 	static Vec3 rot;
	//
	// 	ImGui::Begin("Axis");
	// 	ImGui::DragFloat3("pos##axis", &pos.x, 0.01f);
	// 	ImGui::DragFloat3("rot##axis", &rot.x, 0.01f);
	// 	ImGui::End();
	//
	// 	Debug::DrawAxis(pos, Quaternion::Euler(rot));
	// }

#endif

#ifdef _DEBUG
#pragma region cl_showpos
	if (ConVarManager::GetConVar("cl_showpos")->GetValueAsString() == "1") {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
		constexpr ImGuiWindowFlags windowFlags =
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoDocking;
		ImVec2 windowPos = ImVec2(0.0f, 128.0f + 16.0f);
		windowPos.x = ImGui::GetMainViewport()->Pos.x + windowPos.x;
		windowPos.y = ImGui::GetMainViewport()->Pos.y + windowPos.y;
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);

		// テキストのサイズを取得
		ImGuiIO io = ImGui::GetIO();
		std::string text = std::format(
			"name: {}\n"
			"pos : {:.2f} {:.2f} {:.2f}\n"
			"rot : {:.2f} {:.2f} {:.2f}\n"
			"vel : {:.2f}\n",
			ConVarManager::GetConVar("name")->GetValueAsString(),
			object3DCommon_->GetDefaultCamera()->GetPos().x, object3DCommon_->GetDefaultCamera()->GetPos().y,
			object3DCommon_->GetDefaultCamera()->GetPos().z,
			object3DCommon_->GetDefaultCamera()->GetRotate().x * Math::rad2Deg,
			object3DCommon_->GetDefaultCamera()->GetRotate().y * Math::rad2Deg,
			object3DCommon_->GetDefaultCamera()->GetRotate().z * Math::rad2Deg,
			0.0f
		);
		ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

		// ウィンドウサイズをテキストサイズに基づいて設定
		ImVec2 windowSize = ImVec2(textSize.x + 20.0f, textSize.y + 20.0f); // 余白を追加
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

		ImGui::Begin("##cl_showpos", nullptr, windowFlags);

		ImVec2 textPos = ImGui::GetCursorScreenPos();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		float outlineSize = 1.0f;

		ImU32 textColor = IM_COL32(255, 255, 255, 255);
		ImU32 outlineColor = IM_COL32(0, 0, 0, 94);
		ImGuiManager::TextOutlined(
			drawList,
			textPos,
			text.c_str(),
			textColor,
			outlineColor,
			outlineSize
		);

		ImGui::PopStyleVar();
		ImGui::End();
	}
#pragma endregion
#endif
}

void GameScene::Render() {
	//----------------------------------------
	// オブジェクト3D共通描画設定
	object3DCommon_->Render();
	//----------------------------------------

	//object3D_->Draw();

	//----------------------------------------
	// パーティクル共通描画設定
	particleCommon_->Render();
	//----------------------------------------
	particle_->Draw();

	//----------------------------------------
	// スプライト共通描画設定
	spriteCommon_->Render();
	//----------------------------------------
	//sprite_->Draw();
}

void GameScene::Shutdown() {
	spriteCommon_->Shutdown();
	object3DCommon_->Shutdown();
	particleCommon_->Shutdown();
}
