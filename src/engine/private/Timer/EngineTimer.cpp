#include <engine/public/Timer/EngineTimer.h>

#include <algorithm>

#include "engine/public/OldConsole/ConVarManager.h"

double EngineTimer::mDeltaTime = 0;
double EngineTimer::mTotalTime = 0;
uint64_t EngineTimer::mFrameCount = 0;

EngineTimer::EngineTimer() : mStartTime(Clock::now()),
mLastFrameTime(Clock::now()) {
}

void EngineTimer::StartFrame() {
	// フレーム開始時刻を記録
	mFrameStartTime = Clock::now();

	//#ifdef _DEBUG
	//	// 前フレームの経過時間（deltaTime_）を用いてデバッグ表示する場合、
	//	// EndFrame()で更新したdeltaTime_を使うか、別途前フレームの時間を保存する必要があります。
	//	ImGui::Begin((StrUtils::ConvertToUtf8(kIconTimer) + " EngineTimer").c_str());
	//	ImGui::Text("%.2f FPS", 1.0 / deltaTime_);
	//	ImGui::Text("%.2f ms", deltaTime_ * 1000.0);
	//
	//	const int totalMilliseconds = GetMillisecond(); // 0.01秒単位
	//	const int days = GetDay();
	//	const int hours = GetHour();
	//	const int minutes = GetMinute();
	//	const int secs = GetSecond();
	//	const int centiseconds = totalMilliseconds % 100;
	//
	//	ImGui::Text(
	//		"Uptime: %02d:%02d:%02d:%02d.%02d",
	//		days,
	//		hours,
	//		minutes,
	//		secs,
	//		centiseconds
	//	);
	//	ImGui::End();
	//#endif
}

void EngineTimer::EndFrame() {
	const auto fpsMaxConVar = ConVarManager::GetConVar("cl_fpsmax");
	const double fpsLimit = fpsMaxConVar->GetValueAsDouble();
	if (fpsLimit > 0) {
		const double frameLimitDuration = 1.0 / fpsLimit;

		// 現在時刻を取得し、フレーム処理にかかった時間を計算
		TimePoint currentTime = Clock::now();
		const double frameProcessingTime = std::chrono::duration<double>(currentTime - mFrameStartTime).count();

		// 次フレームまでの残り時間を計算
		double remainingTime = frameLimitDuration - frameProcessingTime;
		if (remainingTime > 0.0) {
			// 残り時間の大部分をスリープ
			const double sleepTime = remainingTime * 0.5;
			if (sleepTime > 0.0) {
				std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
			}

			// 忙待ちで正確なタイミングに補正
			while (std::chrono::duration<double>(Clock::now() - mFrameStartTime).count() < frameLimitDuration) {
				// Busy wait
			}
		}
	}

	// フレーム終了時刻を記録し、次回のdeltaTime_を更新
	TimePoint frameEndTime = Clock::now();
	mDeltaTime = std::chrono::duration<double>(frameEndTime - mFrameStartTime).count();
	mLastFrameTime = frameEndTime;

	++mFrameCount;
}

float EngineTimer::ScaledDelta() {
	const auto timeScaleConVar = ConVarManager::GetConVar("host_timescale");
	const double timeScale = timeScaleConVar->GetValueAsDouble();
	return static_cast<float>(mDeltaTime * timeScale);
}

void EngineTimer::SetTimeScale(const float& scale) {
	ConVarManager::GetConVar("host_timescale")->SetValueFromString(std::to_string(scale));
}

float EngineTimer::GetDeltaTime() {
	// スパイク防止
	mDeltaTime = std::min<double>(mDeltaTime, 1.0f / 60.0f);

	return static_cast<float>(mDeltaTime);
}

float EngineTimer::GetScaledDeltaTime() {
	return static_cast<float>(mDeltaTime) * GetTimeScale();
}

double EngineTimer::GetDeltaTimeDouble() {
	return mDeltaTime;
}

double EngineTimer::GetScaledDeltaTimeDouble() {
	return mDeltaTime * GetTimeScale();
}

float EngineTimer::GetTotalTime() {
	return static_cast<float>(mTotalTime);
}

float EngineTimer::GetTimeScale() {
	return ConVarManager::GetConVar("host_timescale")->GetValueAsFloat();
}

uint64_t EngineTimer::GetFrameCount() {
	return mFrameCount;
}

int EngineTimer::GetDay() {
	return GetMillisecond() / (100 * 60 * 60 * 24);
}

int EngineTimer::GetHour() {
	return GetMillisecond() / (100 * 60 * 60) % 24;
}

int EngineTimer::GetMinute() {
	return GetMillisecond() / (100 * 60) % 60;
}

int EngineTimer::GetSecond() {
	return GetMillisecond() / 100 % 60;
}

int EngineTimer::GetMillisecond() {
	return static_cast<int>(mTotalTime * 100.0);
}

/// <summary>
/// エンジンが起動してからの経過時間を取得します
/// </summary>
DateTime EngineTimer::GetUpDateTime() {
	return {
		.day = GetDay(),
		.hour = GetHour(),
		.minute = GetMinute(),
		.second = GetSecond(),
		.millisecond = GetMillisecond()
	};
}

DateTime EngineTimer::GetNow() {
	const auto now = Clock::now();
	const auto nowTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	struct tm nowTm;
	localtime_s(&nowTm, &nowTime);
	return {
		.year = nowTm.tm_year + 1900,
		.month = nowTm.tm_mon + 1,
		.day = nowTm.tm_mday,
		.hour = nowTm.tm_hour,
		.minute = nowTm.tm_min,
		.second = nowTm.tm_sec,
		.millisecond = GetMillisecond()
	};
}
