#pragma once
#include <memory>
#include <string>

#include <Scene/Base/Scene.h>

class SceneFactory {
public:
	template <typename T>
	void RegisterScene(const std::string& name) {
		sceneCreators_[name] = []() -> std::shared_ptr<Scene> {
			return std::make_shared<T>();
			};
	}

	std::shared_ptr<Scene> CreateScene(const std::string& name) {
		auto it = sceneCreators_.find(name);
		if (it != sceneCreators_.end()) {
			return it->second();
		}
		return nullptr;
	}

private:
	std::unordered_map<std::string, std::function<std::shared_ptr<Scene>()>> sceneCreators_;
};
