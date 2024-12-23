#include "Engine.h"

#ifdef _DEBUG
#include "imgui/imgui_internal.h"
#endif

#include "Camera/Camera.h"
#include "Debug/Debug.h"

#include "Input/InputSystem.h"

#include "Lib/Console/ConCommand.h"
#include "Lib/Console/ConVarManager.h"
#include "Lib/Console/Console.h"
#include "Lib/Utils/ClientProperties.h"
#include "Model/ModelManager.h"
#include "Object3D/Object3DCommon.h"
#include "Particle/ParticleManager.h"
#include "Renderer/D3D12.h"
#include "Renderer/SrvManager.h"
#include "Sprite/SpriteCommon.h"
#include "TextureManager/TextureManager.h"
#include "Window/Window.h"
#include "Window/WindowsUtils.h"

Engine::Engine() = default;

void Engine::Run() {
	Init();
	while (true) {
		if (Window::ProcessMessage() || bWishShutdown)
			break; // ゲームループを抜ける
		Update();
	}
	Shutdown();
}

void Engine::DrawGrid(
	const float gridSize, const float range, const Vec4& color, const Vec4& majorColor,
	const Vec4& axisColor, const Vec4& minorColor) {
	// const float range = 16384.0f;
	constexpr float majorInterval = 1024.0f;
	const float minorInterval = gridSize * 8.0f;

	for (float x = -range; x <= range; x += gridSize) {
		Vec4 lineColor = color;
		if (fmod(x, majorInterval) == 0) {
			lineColor = majorColor;
		} else if (fmod(x, minorInterval) == 0) {
			lineColor = minorColor;
		}
		if (x == 0) {
			lineColor = axisColor;
		}
		Debug::DrawLine(Vec3(x, 0, -range), Vec3(x, 0, range), lineColor);
	}

	for (float z = -range; z <= range; z += gridSize) {
		Vec4 lineColor = color;
		if (fmod(z, majorInterval) == 0) {
			lineColor = majorColor;
		} else if (fmod(z, minorInterval) == 0) {
			lineColor = minorColor;
		}
		if (z == 0) {
			lineColor = axisColor;
		}
		Debug::DrawLine(Vec3(-range, 0, z), Vec3(range, 0, z), lineColor);
	}
}

void Engine::Init() {
	RegisterConsoleCommandsAndVariables();

	// ウィンドウの作成
	window_ = std::make_unique<Window>(L"Window", kClientWidth, kClientHeight);
	window_->Create(nullptr);

	InputSystem::Initialize();

	// レンダラ
	renderer_ = std::make_unique<D3D12>();
	renderer_->Init(window_.get());

	// SRVマネージャの初期化
	srvManager_ = std::make_unique<SrvManager>();
	srvManager_->Init(renderer_.get());

#ifdef _DEBUG
	imGuiManager_ = std::make_unique<ImGuiManager>();
	imGuiManager_->Init(renderer_.get(), window_.get(), srvManager_.get());

	console_ = std::make_unique<Console>();
#endif

	// テクスチャマネージャ
	TextureManager::GetInstance()->Init(renderer_.get(), srvManager_.get());

	// 3Dモデルマネージャ
	ModelManager::GetInstance()->Init(renderer_.get());

	// カメラの作成
	camera_ = std::make_unique<Camera>();
	camera_->SetPos(Vec3::forward * -5.0f + Vec3::up * 2.0f);
	camera_->SetRotate(Vec3::right * 15.0f * Math::deg2Rad);
	rot = camera_->GetRotate();

	// モデル
	modelCommon_ = std::make_unique<ModelCommon>();
	modelCommon_->Init(renderer_.get());

	// オブジェクト3D
	object3DCommon_ = std::make_unique<Object3DCommon>();
	object3DCommon_->Init(renderer_.get());
	object3DCommon_->SetDefaultCamera(camera_.get());

	// スプライト
	spriteCommon_ = std::make_unique<SpriteCommon>();
	spriteCommon_->Init(renderer_.get());

	// ライン
	lineCommon_ = std::make_unique<LineCommon>();
	lineCommon_->Init(renderer_.get());
	lineCommon_->SetDefaultCamera(camera_.get());

	Debug::Init(lineCommon_.get());

	//-------------------------------------------------------------------------
	// コマンドのリセット
	//-------------------------------------------------------------------------
	HRESULT hr = renderer_->GetCommandAllocator()->Reset();
	assert(SUCCEEDED(hr));
	hr = renderer_->GetCommandList()->Reset(
		renderer_->GetCommandAllocator(),
		nullptr);
	assert(SUCCEEDED(hr));

	time_ = std::make_unique<EngineTimer>();

	// シーン
	gameScene_ = std::make_unique<GameScene>();
	gameScene_->Init(
		renderer_.get(),
		window_.get(),
		spriteCommon_.get(),
		object3DCommon_.get(),
		modelCommon_.get(),
		srvManager_.get(),
		time_.get());

	hr = renderer_->GetCommandList()->Close();
	assert(SUCCEEDED(hr));
}

void Engine::Update() {
#ifdef _DEBUG
	ImGuiManager::NewFrame();
	Console::Update();
#endif

	time_->StartFrame();

	/* ----------- 更新処理 ---------- */
	camera_->Update();

#ifdef _DEBUG
	// カメラの操作
	static float moveSpd = 4.0f;

	static bool firstReset = true; // 初回リセットフラグ
	static bool cursorHidden = false;

	static bool bOpenPopup = false; // ポップアップ表示フラグ
	static float popupTimer = 0.0f;

	if (InputSystem::IsPressed("attack2")) {
		if (!cursorHidden) {
			ShowCursor(FALSE); // カーソルを非表示にする
			cursorHidden = true;
		}

		Vec2 delta = InputSystem::GetMouseDelta();

		if (!firstReset) {
			// 回転
			float sensitivity = std::stof(ConVarManager::GetConVar("sensitivity")->GetValueAsString());
			float m_pitch = 0.022f;
			float m_yaw = 0.022f;
			float min = -89.0f;
			float max = 89.0f;

			rot.y += delta.y * sensitivity * m_pitch * Math::deg2Rad;
			rot.x += delta.x * sensitivity * m_yaw * Math::deg2Rad;

			rot.y = std::clamp(rot.y, min * Math::deg2Rad, max * Math::deg2Rad);

			Camera* cam = object3DCommon_->GetDefaultCamera();

			cam->SetRotate(Vec3::up * rot.x + Vec3::right * rot.y);

			Vec3 moveInput = {0.0f, 0.0f, 0.0f};

			if (InputSystem::IsPressed("forward")) {
				moveInput.z += 1.0f;
			}

			if (InputSystem::IsPressed("back")) {
				moveInput.z -= 1.0f;
			}

			if (InputSystem::IsPressed("moveright")) {
				moveInput.x += 1.0f;
			}

			if (InputSystem::IsPressed("moveleft")) {
				moveInput.x -= 1.0f;
			}

			if (InputSystem::IsPressed("moveup")) {
				moveInput.y += 1.0f;
			}

			if (InputSystem::IsPressed("movedown")) {
				moveInput.y -= 1.0f;
			}

			moveInput.Normalize();

			Quaternion camRot = Quaternion::Euler(cam->GetRotate());
			Vec3 cameraForward = camRot * Vec3::forward;
			Vec3 cameraRight = camRot * Vec3::right;
			Vec3 cameraUp = camRot * Vec3::up;

			if (InputSystem::IsTriggered("invprev")) {
				moveSpd += 1.0f;
			}

			if (InputSystem::IsTriggered("invnext")) {
				moveSpd -= 1.0f;
			}

			static float oldMoveSpd = 0.0f;
			if (moveSpd != oldMoveSpd) {
				bOpenPopup = true;
				popupTimer = 0.0f;
			}

			moveSpd = std::clamp(moveSpd, 1.0f, 100.0f);

			oldMoveSpd = moveSpd;

			cam->SetPos(cam->GetPos() + (cameraForward * moveInput.z + cameraRight * moveInput.x + cameraUp * moveInput.y) * moveSpd * EngineTimer::GetScaledDeltaTime());
		}
		// カーソルをウィンドウの中央にリセット
		POINT centerCursorPos = {
			static_cast<LONG>(window_->GetClientWidth() / 2), static_cast<LONG>(window_->GetClientHeight() / 2)};
		ClientToScreen(window_->GetWindowHandle(), &centerCursorPos); // クライアント座標をスクリーン座標に変換
		SetCursorPos(centerCursorPos.x, centerCursorPos.y);

		firstReset = false; // 初回リセット完了
	} else {
		if (cursorHidden) {
			ShowCursor(TRUE); // カーソルを表示する
			cursorHidden = false;
		}
		firstReset = true; // マウスボタンが離されたら初回リセットフラグをリセット
	}

	// 移動速度が変更されたらImGuiで現在の移動速度をポップアップで表示
	if (bOpenPopup) {

		// ビューポートのサイズと位置を取得
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 viewportPos = viewport->Pos;
		ImVec2 viewportSize = viewport->Size;
		ImVec2 windowSize = ImVec2(256.0f, 32.0f);

		// ウィンドウの中央下部位置を計算
		ImVec2 windowPos(
			viewportPos.x + (viewportSize.x) * 0.5f,
			viewportPos.y + (viewportSize.y) * 0.75f);

		// ウィンドウの位置を調整
		windowPos.x -= windowSize.x * 0.5f;
		windowPos.y -= windowSize.y * 0.5f;

		// ウィンドウの位置を設定
		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

		// ウィンドウを角丸に
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 16.0f);
		// タイトルバーを非表示

		ImGui::Begin(
			"##move speed",
			nullptr,
			ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_NoFocusOnAppearing |
				ImGuiWindowFlags_NoScrollbar);

		ImGui::SetCursorPos(
			ImVec2((windowSize.x - ImGui::CalcTextSize(std::format("{:.2f}", moveSpd).c_str()).x) * 0.5f, (windowSize.y - ImGui::GetFontSize()) * 0.5f));
		ImGui::Text("%.2f", moveSpd);

		// 一定時間経過後にポップアップをフェードアウトして閉じる
		popupTimer += EngineTimer::GetDeltaTime(); // ゲーム内ではないのでScaledDeltaTimeではなくDeltaTimeを使用
		if (popupTimer >= 3.0f) {
			ImGui::CloseCurrentPopup();
			bOpenPopup = false;
			popupTimer = 0.0f;
		}

		ImGui::End();

		ImGui::PopStyleVar();
	}


#endif

	// ゲームシーンの更新
	gameScene_->Update();

	// グリッドの表示
	DrawGrid(
		1.0f,
		64,
		{.x = 0.28f, .y = 0.28f, .z = 0.28f, .w = 1.0f},
		{.x = 0.39f, .y = 0.2f, .z = 0.02f, .w = 1.0f},
		{.x = 0.0f, .y = 0.39f, .z = 0.39f, .w = 1.0f},
		{.x = 0.39f, .y = 0.39f, .z = 0.39f, .w = 1.0f});

#ifdef _DEBUG
	ImGuiViewportP* viewport = static_cast<ImGuiViewportP*>(static_cast<void*>(ImGui::GetMainViewport()));
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_MenuBar;

	ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, 10.0f);

	if (ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down, 38, window_flags)) {
		if (ImGui::BeginMenuBar()) {
			ImGui::PopStyleVar();

			ImGui::Text("ハリボテ");

			ImGui::BeginDisabled(true);

			// アングルスナップ
			{
				const float windowHeight = ImGui::GetWindowSize().y;
				const char* items[] = {
					"0.25°", "0.5°", "1°", "5°", "5.625°", "11.25°", "15°", "22.5°", "30°", "45°", "90°"};
				static int itemCurrentIndex = 6;
				const char* comboLabel = items[itemCurrentIndex];

				ImGui::Text("Angle: ");

				// 垂直中央に配置
				float comboHeight = ImGui::GetFrameHeight();
				float offsetY = (windowHeight - comboHeight) * 0.5f;
				ImGui::SetCursorPosY(offsetY);

				if (ImGui::BeginCombo("##angle", comboLabel)) {
					for (int n = 0; n < IM_ARRAYSIZE(items); ++n) {
						const bool isSelected = (itemCurrentIndex == n);
						if (ImGui::Selectable(items[n], isSelected)) {
							itemCurrentIndex = n;
						}
						if (isSelected) {
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}

			ImGui::EndDisabled();

			ImGui::EndMenuBar();
		}
		ImGui::End();
	}
#endif

#ifdef _DEBUG // cl_showfps
	if (ConVarManager::GetConVar("cl_showfps")->GetValueAsString() != "0") {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});

		ImGuiWindowFlags windowFlags =
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoBringToFrontOnFocus;

		ImVec2 windowPos = ImVec2(0.0f, 128.0f);

		windowPos.x = ImGui::GetMainViewport()->Pos.x + windowPos.x;
		windowPos.y = ImGui::GetMainViewport()->Pos.y + windowPos.y;

		ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
		std::string text;
		float fps;
		if (ConVarManager::GetConVar("cl_showfps")->GetValueAsString() == "1") {
			fps = 1.0f / time_->GetScaledDeltaTime();
		}
		if (ConVarManager::GetConVar("cl_showfps")->GetValueAsString() == "2") {
			ImGuiIO io = ImGui::GetIO();
			fps = io.Framerate;
		}

		text = std::format("{:.2f} fps", fps);

		ImVec2 textSize = ImGui::CalcTextSize(text.c_str());

		// ウィンドウサイズをテキストサイズに基づいて設定
		ImVec2 windowSize = ImVec2(textSize.x + 20.0f, textSize.y + 20.0f); // 余白を追加
		ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

		ImGui::Begin("##cl_showfps", nullptr, windowFlags);

		ImVec2 textPos = ImGui::GetCursorScreenPos();

		ImDrawList* drawList = ImGui::GetWindowDrawList();

		float outlineSize = 1.0f;

		ImU32 textColor = ImGui::ColorConvertFloat4ToU32(kConsoleColorError);
		if (fps >= 59.9f) {
			textColor = ImGui::ColorConvertFloat4ToU32(kConsoleColorFloat);
		} else if (fps >= 29.9f) {
			textColor = ImGui::ColorConvertFloat4ToU32(kConsoleColorWarning);
		}

		ImU32 outlineColor = IM_COL32(0, 0, 0, 94);

		ImGuiManager::TextOutlined(
			drawList,
			textPos,
			text.c_str(),
			textColor,
			outlineColor,
			outlineSize);

		ImGui::PopStyleVar();

		ImGui::End();
	}
#endif

#ifdef _DEBUG
	Debug::Update();
#endif

	InputSystem::Update();

	/* ---------- Pre ----------- */
	renderer_->PreRender();
	srvManager_->PreDraw();
	/* ---------- コマンド積み ----------- */

	gameScene_->Render();

	//-------------------------------------------------------------------------
	lineCommon_->Render();
	Debug::Draw();
	//-------------------------------------------------------------------------

#ifdef _DEBUG
	imGuiManager_->EndFrame();
#endif

	/* ---------- Post ----------- */
	renderer_->PostRender();
	/* ---------- ゲームループ終了 ---------- */

	time_->EndFrame();
}

void Engine::Shutdown() const {
	gameScene_->Shutdown();

	ModelManager::Shutdown();
	TextureManager::Shutdown();

	Debug::Shutdown();

#ifdef _DEBUG
	// ImGuiManagerのシャットダウンは最後に行う
	if (imGuiManager_) {
		imGuiManager_->Shutdown();
	}
#endif
}

void Engine::RegisterConsoleCommandsAndVariables() {
	// コンソールコマンドを登録
	ConCommand::RegisterCommand("bind", [](const std::vector<std::string>& args) {
			if (args.size() < 2) {
				Console::Print("Usage: bind <key> <command>\n");
				return;
			}
			std::string key = args[0];
			std::string command = args[1];
			InputSystem::BindKey(key, command); }, "Bind a key to a command.");
	ConCommand::RegisterCommand("clear", Console::Clear, "Clear all console output.");
	ConCommand::RegisterCommand("cls", Console::Clear, "Clear all console output.");
	ConCommand::RegisterCommand("help", Console::Help, "Find help about a convar/concommand.");
	ConCommand::RegisterCommand("toggleconsole", Console::ToggleConsole, "Show/hide the console.");
	ConCommand::RegisterCommand("neofetch", Console::Neofetch, "Show system info.");
	ConCommand::ExecuteCommand("neofetch");
	ConCommand::RegisterCommand("quit", Quit, "Exit the engine.");
	// コンソール変数を登録
	ConVarManager::RegisterConVar<int>("cl_showpos", 1, "Draw current position at top of screen");
	ConVarManager::RegisterConVar<int>("cl_showfps", 2, "Draw fps meter (1 = fps, 2 = smooth)");
	ConVarManager::RegisterConVar<int>("cl_fpsmax", kMaxFps, "Frame rate limiter");
	ConVarManager::RegisterConVar<std::string>("name", "unnamed", "Current user name", ConVarFlags::ConVarFlags_Notify);
	Console::SubmitCommand("name " + WindowsUtils::GetWindowsUserName());
	ConVarManager::RegisterConVar<float>("sensitivity", 2.0f, "Mouse sensitivity.");
	ConVarManager::RegisterConVar<float>("host_timescale", 1.0f, "Prescale the clock by this amount.");
	ConVarManager::RegisterConVar<float>("sv_gravity", 800.0f, "World gravity.");
	ConVarManager::RegisterConVar<float>("sv_maxvelocity", 3500.0f, "Maximum speed any ballistically moving object is allowed to attain per axis.");
	ConVarManager::RegisterConVar<float>("sv_accelerate", 10.0f, "Linear acceleration amount (old value is 5.6)");
	ConVarManager::RegisterConVar<float>("sv_maxspeed", 320.0f, "Maximum speed a player can move.");
	ConVarManager::RegisterConVar<float>("sv_stopspeed", 100.0f, "Minimum stopping speed when on ground.");
	ConVarManager::RegisterConVar<float>("sv_friction", 4.0f, "World friction.");

	// デフォルトのバインド
	Console::SubmitCommand("bind ` toggleconsole");
	Console::SubmitCommand("bind w +forward");
	Console::SubmitCommand("bind s +back");
	Console::SubmitCommand("bind a +moveleft");
	Console::SubmitCommand("bind d +moveright");
	Console::SubmitCommand("bind e +moveup");
	Console::SubmitCommand("bind q +movedown");
	Console::SubmitCommand("bind space +jump");
	Console::SubmitCommand("bind lshift +sprint");
	Console::SubmitCommand("bind lctrl +crouch");
	Console::SubmitCommand("bind r +reload");
	// Console::SubmitCommand("bind e +use");
	Console::SubmitCommand("bind mouse1 +attack1");
	Console::SubmitCommand("bind mouse2 +attack2");
	Console::SubmitCommand("bind mouse3 +attack3");
	Console::SubmitCommand("bind mouse4 +attack4");
	Console::SubmitCommand("bind mouse5 +attack5");
	Console::SubmitCommand("bind mousewheelup +invprev");
	Console::SubmitCommand("bind mousewheeldown +invnext");
	Console::SubmitCommand("bind c +changecamera");
}

void Engine::Quit([[maybe_unused]] const std::vector<std::string>& args) {
	bWishShutdown = true;
}

bool Engine::bWishShutdown = false;
