#pragma once

// #include <algorithm> // min, max, clamp
// #include <cmath> // powf
// #include "tpot_image.h" // color_buffer_f

struct CONVERT_OPTIONS{
	const tpot::color_buffer_f* target = nullptr;	// Œ³ŠG
	float radius = 5.0f;							// •½‹Ï‰»”¼Œa
};

class Analize
{
public:
	static bool convert(tpot::color_buffer_f &dst, const CONVERT_OPTIONS options);
};
