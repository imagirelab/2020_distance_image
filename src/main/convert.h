#pragma once

// #include <algorithm> // min, max, clamp
// #include <cmath> // powf
// #include "tpot_image.h" // color_buffer_f

struct CONVERT_OPTIONS{
	const tpot::color_buffer_f* target = nullptr;	// ���G
	float radius = 5.0f;							// ���ω����a
};

class Analize
{
public:
	static bool convert(tpot::color_buffer_f &dst, const CONVERT_OPTIONS options);
};
