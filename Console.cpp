#include "Console.h"

#include <cstring>
#include <format>
#include <sstream>

#include "ConVar.h"
#include "ConVars.h"
#include "Source/Engine/Utils/ConvertString.h"

#ifdef _DEBUG
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#endif

#include "Source/Engine/Utils/ClientProperties.h"

#ifdef _DEBUG
int ConsoleCallback(ImGuiInputTextCallbackData* data) {
	switch (data->EventFlag) {
	case ImGuiInputTextFlags_CallbackCompletion:
		Console::Print("Completion", kConsoleWarning);
		break;

	case ImGuiInputTextFlags_CallbackHistory:
	{
		const int prev_history_index = historyIndex;
		if (data->EventKey == ImGuiKey_UpArrow) {
			if (historyIndex > 0) {
				historyIndex--;
			}
		} else if (data->EventKey == ImGuiKey_DownArrow) {
			if (historyIndex < static_cast<int>(history.size()) - 1) {
				historyIndex++;
			} else {
				historyIndex = static_cast<int>(history.size());  // 履歴が空の場合はサイズと一致させる
			}
		}
		if (prev_history_index != historyIndex) {
			data->DeleteChars(0, data->BufTextLen);
			if (historyIndex < static_cast<int>(history.size())) {
				data->InsertChars(0, history[historyIndex].c_str());
			} else {
				data->InsertChars(0, "");  // 履歴が空の場合は空文字列を挿入
			}
		}
	}
	break;

	case ImGuiInputTextFlags_CallbackEdit:
		break;

	case ImGuiInputTextFlags_CallbackResize:
		Console::Print("Resize	", kConsoleError);
		break;
	default:;
	}
	return 0;
}
#endif	

void Console::Update() {
#ifdef _DEBUG
	if (!bShowConsole) {
		return;
	}

	ImGuiWindowFlags consoleWindowFlags =
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoDocking;

	if (bShowPopup) {
		consoleWindowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	}

	ImGui::SetNextWindowSizeConstraints({ 256.0f,256.0f }, { 8192.0f,8192.0f });

	bool open = ImGui::Begin("Console", &bShowConsole, consoleWindowFlags);

	if (open) {
		ImVec2 size = ImGui::GetContentRegionAvail();
		size.y -= ImGui::GetFrameHeightWithSpacing() + 9.0f;

		ImGui::Spacing();

		if (ImGui::BeginChild("##scrollbox", size, true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar)) {
			for (const ConsoleText& consoleText : consoleTexts) {
				ImGui::PushStyleColor(ImGuiCol_Text, consoleText.color);
				ImGui::Selectable((consoleText.text).c_str());
				ImGui::PopStyleColor();
			}

			// 一番下にスクロールしているか?
			if (wishScrollToBottom && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
				ImGui::SetScrollHereY(1.0f);
			}

			// スクロールをいじった?
			if (ImGui::GetScrollY() < ImGui::GetScrollMaxY()) {
				wishScrollToBottom = false;
			} else {
				wishScrollToBottom = true;
			}

			ScrollToBottom();
		}

		ImGui::EndChild();

		ImGui::Spacing();

		size = ImGui::GetContentRegionAvail();
		size.x -= ImGui::CalcTextSize("  Submit  ").x;

		ImGui::PushItemWidth(size.x);

		ImGuiInputTextFlags inputTextFlags =
			ImGuiInputTextFlags_EnterReturnsTrue |
			ImGuiInputTextFlags_EscapeClearsAll |
			ImGuiInputTextFlags_CallbackAlways |
			ImGuiInputTextFlags_CallbackCharFilter |
			ImGuiInputTextFlags_CallbackCompletion |
			ImGuiInputTextFlags_CallbackEdit |
			ImGuiInputTextFlags_CallbackHistory;

		if (ImGui::InputText("##input", inputText, IM_ARRAYSIZE(inputText), inputTextFlags, reinterpret_cast<ImGuiInputTextCallback>(ConsoleCallback))) {
			ImGui::SetKeyboardFocusHere(-1);
			SubmitCommand(inputText);
			if (!std::string(inputText).empty()) {
				history.emplace_back(inputText);
				historyIndex = static_cast<int>(history.size());
			}
			memset(inputText, 0, sizeof inputText);
		}

		if (ImGui::IsItemActive()) {
			bShowPopup = true;
		} else {
			bShowPopup = false;
		}

		ImGui::SameLine();

		if (ImGui::Button(" Submit ")) {
			SubmitCommand(inputText);
			if (!std::string(inputText).empty()) {
				history.emplace_back(inputText);
				historyIndex = static_cast<int>(history.size());
			}
			memset(inputText, 0, sizeof inputText);
		}
	}
	ImGui::End();

	if (consoleTexts.size() >= kConsoleMaxLineCount) {
		consoleTexts.erase(consoleTexts.begin());
	}
#endif
}

void Console::UpdateRepeatCount(const std::string& message, const ImVec4 color) {
	message; color;
#ifdef _DEBUG
	repeatCounts.back()++;

	if (repeatCounts.back() >= kConsoleRepeatError) {
		consoleTexts.back() = { std::format("{} [x{}]", message, repeatCounts.back()), kConsoleError };
	} else if (repeatCounts.back() >= kConsoleRepeatWarning) {
		consoleTexts.back() = { std::format("{} [x{}]", message, repeatCounts.back()), kConsoleWarning };
	} else {
		consoleTexts.back() = { std::format("{} [x{}]", message, repeatCounts.back()), color };
}
#endif
}

void Console::Print(const std::string& message, const ImVec4 color) {
	message; color;
#ifdef _DEBUG
	if (message.empty()) {
		return;
	}

	if (!consoleTexts.empty() && consoleTexts.back().text.find(message) == 0) {
		// 前のメッセージと同じ場合、カウントを増加させる
		UpdateRepeatCount(message, color);
	} else {
		// 前のメッセージと異なる場合、新しいメッセージを追加
		consoleTexts.push_back({ message, color });
		repeatCounts.push_back(1);
		OutputDebugString(ConvertString(message).c_str());
	}

	wishScrollToBottom = true;
#endif
}

void Console::ToggleConsole() {
#ifdef _DEBUG
	bShowConsole = !bShowConsole;
#endif
}

void Console::ScrollToBottom() {
#ifdef _DEBUG
	if (wishScrollToBottom) {
		ImGui::SetScrollHereY(1.0f);
		wishScrollToBottom = false;
	}
#endif
}

void Console::SubmitCommand(const std::string& command) {
	command;
#ifdef _DEBUG
	std::string trimmedCommand = TrimSpaces(command);

	if (trimmedCommand.empty()) {
		return;
	}

	const std::vector<std::string> tokens = TokenizeCommand(trimmedCommand);

	AddHistory(trimmedCommand);

	if (ConVars::GetInstance().GetAllConVars().contains(tokens[0])) {
		if (tokens.size() < 2) {
			Print(ConVars::GetInstance().GetConVar(tokens[0])->GetDescription());
		}

		for (int i = 1; i < tokens.size(); ++i) {
			ConVars::GetInstance().GetConVar(tokens[0])->SetValue(std::stoi(tokens[i]));
		}
	} else {
		Print(std::format("Unknown command: {}", trimmedCommand));
	}

	wishScrollToBottom = true;
#endif
}

void Console::AddHistory(const std::string& command) {
	command;
#ifdef _DEBUG
	consoleTexts.push_back({ "> " + command,ImVec4(0.8f,1.0f,1.0f,1.0f) });
#endif	
}

void Console::ShowPopup() {
#ifdef _DEBUG
	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImVec2 inputTextRectMin = ImGui::GetItemRectMin();
	ImVec2 inputTextRectMax = ImGui::GetItemRectMax();

	ImGuiWindowFlags popupFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_HorizontalScrollbar |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus;

	// 見た目が悪いので角を丸めない
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	// テキストボックスの上に配置
	ImGui::SetNextWindowPos(ImVec2(inputTextRectMin.x, windowPos.y + windowSize.y));
	// テキストボックスの幅に設定
	ImGui::SetNextWindowSize(ImVec2(inputTextRectMax.x - inputTextRectMin.x, ImGui::GetTextLineHeightWithSpacing() * kConsoleSuggestLineCount));

	if (ImGui::Begin("history_popup", nullptr, popupFlags)) {
		ImGui::PushAllowKeyboardFocus(false);

		for (auto h : history) {
			ImGui::Selectable(h.c_str());
		}

		ImGui::SetScrollHereY(1.0f);

		ImGui::PopAllowKeyboardFocus();
	}

	ImGui::End();

	ImGui::PopStyleVar();
#endif
}

std::string Console::TrimSpaces(const std::string& string) {
	const size_t start = string.find_first_not_of(" \t\n\r");
	const size_t end = string.find_last_not_of(" \t\n\r");

	if (start == std::string::npos || end == std::string::npos) {
		return "";
	}

	return string.substr(start, end - start + 1);
}

std::vector<std::string> Console::TokenizeCommand(const std::string& command) {
	std::istringstream stream(command);
	std::vector<std::string> tokens;
	std::string token;

	while (stream >> token) {
		tokens.push_back(token);
	}

	return tokens;
}