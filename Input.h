#pragma once

#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <dinput.h>

#include "Source/Engine/Window/Window.h"

#include <wrl/client.h>

class Input {
public:
	void Setup(const Window& window);
	void Update();

	// キーの押下をチェック
	bool PushKey(BYTE keyNumber);
	// キーのトリガーをチェック
	bool TriggerKey(BYTE keyNumber);

public:
	// 全キーの状態
	BYTE key[256] = {};
	BYTE preKey[256] = {};

	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

private:
	Microsoft::WRL::ComPtr<IDirectInputDevice8> keyboard = nullptr;
};
