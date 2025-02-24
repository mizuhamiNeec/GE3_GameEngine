#pragma once
#include <numbers>
#include <vector>

#include "Vector/Vec3.h"

struct AABB;

namespace Math {
	//-------------------------------------------------------------------------
	// 定数
	//-------------------------------------------------------------------------
	constexpr float pi = std::numbers::pi_v<float>; // π
	constexpr float deg2Rad = pi / 180.0f; // 度からラジアンへの変換係数
	constexpr float rad2Deg = 180.0f / pi; // ラジアンから度への変換係数

	//-------------------------------------------------------------------------
	// 汎用関数
	//-------------------------------------------------------------------------
	Vec3 Lerp(const Vec3& a, const Vec3& b, float t);
	Vec3 CatmullRomPosition(const std::vector<Vec3>& points, float t);
	Vec3 CatmullRomInterpolation(const Vec3& p0, const Vec3& p1, const Vec3& p2, const Vec3& p3, float t);
	float DeltaAngle(const float& current, const float& target);

	Vec2 WorldToScreen(const Vec3& worldPos, const bool& bClamp, const float& margin, bool& isOffscreen, float& outAngle);

	// TODO: あんたはここにいるべきじゃない...
	bool IsCollision(const AABB& aabb, const Vec3& point);

	//-------------------------------------------------------------------------
	// 単位変換
	//-------------------------------------------------------------------------
	Vec3 HtoM(const Vec3& vec);
	float HtoM(float val);
	Vec3 MtoH(const Vec3& vec);
	float MtoH(float val);
}
