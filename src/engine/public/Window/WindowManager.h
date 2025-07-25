#pragma once
#include <memory>

#include "Base/BaseWindow.h"

class OldWindowManager {
public:
	OldWindowManager();
	~OldWindowManager();

	static void AddWindow(std::unique_ptr<BaseWindow> window);

	[[nodiscard]] static bool ProcessMessage();

	static BaseWindow* GetMainWindow();
	static void ClearWindows();
	static const std::vector<std::unique_ptr<BaseWindow>>& GetWindows();

private:
	static std::vector<std::unique_ptr<BaseWindow>> windows_;
};

