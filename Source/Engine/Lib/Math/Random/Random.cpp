#include "Random.h"

#include "../Vector/Vec3.h"

// 静的メンバ変数の定義
std::mt19937_64 Random::randomEngine_{std::random_device{}()}; // 初期化リストでシード値を設定
std::mutex Random::mtx_; // mutexも定義

//-----------------------------------------------------------------------------
// 指定された範囲の乱数を生成します
//-----------------------------------------------------------------------------
int Random::IntRange(const int& min, const int& max) {
	std::uniform_int_distribution<int> distribution(min, max);
	std::lock_guard lock(mtx_);
	return distribution(randomEngine_);
}

//-----------------------------------------------------------------------------
// 指定された範囲の乱数を生成します
//-----------------------------------------------------------------------------
float Random::FloatRange(const float& min, const float& max) {
	std::uniform_real_distribution<float> distribution(min, max);
	std::lock_guard lock(mtx_);
	return distribution(randomEngine_);
}

//-----------------------------------------------------------------------------
// 指定された範囲の乱数を生成します
//-----------------------------------------------------------------------------
Vec3 Random::Vec3Range(const Vec3& min, const Vec3& max) {
	return {
		FloatRange(min.x, max.x),
		FloatRange(min.y, max.y),
		FloatRange(min.z, max.z)};
}
