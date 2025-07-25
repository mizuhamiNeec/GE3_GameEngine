#include <engine/public/OldConsole/Console.h>
#include <engine/public/ResourceSystem/Shader/Shader.h>
#include <engine/public/ResourceSystem/Shader/ShaderManager.h>

Shader* ShaderManager::LoadShader(const std::string& name, const std::string& vsPath, const std::string& psPath, const std::string& gsPath) {
	// パスを結合してキーにする
	std::string filePath = vsPath + ";" + psPath + ";" + gsPath;

	// 既に読み込まれているシェーダがあればそれを返す
	if (shaders_.contains(filePath)) {
		return shaders_[filePath].get();
	}

	// 新しいシェーダを作成
	if (auto shader = std::make_unique<Shader>(name, vsPath, psPath, gsPath)) {
		// シェーダの読み込みに成功したので登録
		shaders_[filePath] = std::move(shader);
		return shaders_[filePath].get();
	}

	Console::Print(
		"シェーダの読み込みに失敗しました: " + filePath + "\n",
		kConTextColorError,
		Channel::ResourceSystem
	);

	return nullptr;
}

Shader* ShaderManager::GetShader(const std::string& name) {
	auto it = shaders_.find(name);
	return it != shaders_.end() ? it->second.get() : nullptr;
}

void ShaderManager::Init() {
	Console::Print(
		"ShaderManager を初期化しています...\n",
		kConTextColorGray,
		Channel::ResourceSystem
	);
	shaders_.clear();
}

void ShaderManager::Shutdown() {
	Console::Print("ShaderManager を終了しています...\n", kConTextColorWait, Channel::ResourceSystem);

	// 個々のシェーダーインスタンスをクリーンアップ
	for (auto& [path, shader] : shaders_) {
		if (shader) {
			shader->Release();
			shader.reset();
		}
	}
	shaders_.clear();

	// 静的DXCリソースの解放
	Shader::ReleaseStaticResources();
}