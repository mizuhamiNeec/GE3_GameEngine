#include "Engine.h"

#include "ConVars.h"
#include "ConVar.h"
#include "Source/Engine/TextureManager/TextureManager.h"

void Engine::Run() {
	Initialize();
	Update();
	Shutdown();
}

void Engine::Initialize() {
	// ウィンドウの作成
	window_ = std::make_unique<Window>();
	WindowConfig windowConfig = {
		.windowTitle = kWindowTitle,
		.clientWidth = kClientWidth,
		.clientHeight = kClientHeight,
		.windowClassName = kWindowClassName,
		.dwStyle = WS_OVERLAPPEDWINDOW,
		.dwExStyle = WS_EX_OVERLAPPEDWINDOW,
		.nCmdShow = ConVars::GetInstance().GetConVar("cl_nshowcmd")->GetInt(),
	};
	window_->CreateMainWindow(windowConfig);

	// レンダラ
	renderer_ = std::make_unique<D3D12>();
	renderer_->Init(window_.get());

	// 共通スプライト
	spriteCommon_ = std::make_unique<SpriteCommon>();
	spriteCommon_->Init(renderer_.get());

	// 入力
	input_ = std::make_unique<Input>();
	input_->Init(window_.get());

	// シーン
	gameScene_ = std::make_unique<GameScene>();
	gameScene_->Init(renderer_.get(), window_.get());

#ifdef _DEBUG
	imGuiManager_ = std::make_unique<ImGuiManager>();
	imGuiManager_->Init(renderer_.get(), window_.get());

	console_ = std::make_unique<Console>();
#endif
}

void Engine::Update() const {
	while (true) {
		/* ----------- 更新処理 ---------- */

		input_->Update();

		// コンソール表示切り替え
		if (input_->TriggerKey(DIK_GRAVE)) {
			Console::ToggleConsole();
		}

#ifdef _DEBUG
		imGuiManager_->NewFrame();
		console_->Update();
#endif

		// ゲームシーンの更新
		gameScene_->Update();

#ifdef _DEBUG // cl_showfps
		if (ConVars::GetInstance().GetConVar("cl_showfps")->GetInt() == 1) {
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f,0.0f });

			ImGuiWindowFlags windowFlags =
				ImGuiWindowFlags_NoBackground |
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoSavedSettings;

			ImVec2 windowPos = ImVec2(0.0f, 128.0f);
			ImVec2 windowSize = ImVec2(1080.0f, 80.0f);

			windowPos.x = ImGui::GetMainViewport()->Pos.x + windowPos.x;
			windowPos.y = ImGui::GetMainViewport()->Pos.y + windowPos.y;

			ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

			ImGui::Begin("##cl_showfps", nullptr, windowFlags);

			ImVec2 textPos = ImGui::GetCursorScreenPos();

			ImDrawList* drawList = ImGui::GetWindowDrawList();

			float outlineSize = 1.0f;

			std::string text = std::format("{:.2f} fps", ImGui::GetIO().Framerate);

			ImU32 textColor = IM_COL32(255, 0, 0, 255);
			ImU32 outlineColor = IM_COL32(0, 0, 0, 94);

			TextOutlined(
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
#endif


		/* ---------- Pre ----------- */

		renderer_->PreRender();

		/* ---------- コマンド積み ----------- */

		gameScene_->Render();

		spriteCommon_->Render();

#ifdef _DEBUG
		imGuiManager_->EndFrame();
#endif

		/* ---------- Post ----------- */
		renderer_->PostRender();

		/* ---------- ゲームループ終了 ---------- */

		if (Window::ProcessMessage()) {
			break; // ゲームループを抜ける
		}
	}
}

void Engine::Shutdown() {
	gameScene_->Shutdown();
	spriteCommon_->Shutdown();

	// ImGuiManagerのシャットダウンは最後に行う
	if (imGuiManager_) {
		imGuiManager_->Shutdown();
	}
	TextureManager::GetInstance().Shutdown();
}