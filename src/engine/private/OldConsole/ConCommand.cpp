#include "../../public/OldConsole/ConCommand.h"

#include <sstream>

#include "../../public/OldConsole/ConVarManager.h"
#include "../../public/OldConsole/Console.h"

void ConCommand::Init() {
	RegisterCommand(
		"toggle",
		[](const std::vector<std::string>& args) {
			ConVarManager::ToggleConVar(args[0]);
		},
		"Toggles a convar on or off, or cycles through a set of values."
	);
}

void ConCommand::RegisterCommand(const std::string& name, const CommandCallback& callback, const std::string& help) {
	commands_[name] = { callback, help };
}

bool ConCommand::ExecuteCommand(const std::string& command) {
	auto tokens = TokenizeCommand(command);
	if (tokens.empty()) {
		return false;
	}

	const auto& cmdName = tokens[0];
	auto it = commands_.find(cmdName);
	if (it != commands_.end()) {
		const auto& callback = it->second.first;
		const std::vector args(tokens.begin() + 1, tokens.end());
		callback(args);
		return true;
	}

	return false;
}

std::unordered_map<std::string, std::pair<CommandCallback, std::string>> ConCommand::GetCommands() {
	return commands_;
}

void ConCommand::Help() {
	for (const auto& [commandName, commandData] : commands_) {
		Console::Print(" - " + commandName + " : " + commandData.second + "\n", kConFgColorDark, Channel::None);
	}
}

std::vector<std::string> ConCommand::TokenizeCommand(const std::string& command) {
	std::istringstream stream(command);
	std::vector<std::string> tokens;
	std::string token;
	while (stream >> token) {
		tokens.emplace_back(token);
	}
	return tokens;
}

std::unordered_map<std::string, std::pair<ConCommand::CommandCallback, std::string>> ConCommand::commands_;
