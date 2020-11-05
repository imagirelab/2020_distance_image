#include <algorithm> // min, max, clamp
#include <cmath> // powf
#include <random>// mt19937. random_device

#include "tpot_image.h"
#include "convert.h"



static float frand(unsigned int v) {
	unsigned int res = (v >> 9) | 0x3f800000;
	return (*(float*)&res) - 1.0f;
}

static void morton_order(int& x, int& y, int idx, int w_bit, int h_bit)
{
	x = 0;
	y = 0;
	int x_bit = 0;
	int y_bit = 0;
	int bit = idx;
	while (bit) {
		if (x_bit < w_bit) {
			if (bit & 1) { x |= (1 << x_bit); }
			x_bit++;
			bit >>= 1;
		}
		if (y_bit < h_bit) {
			if (bit & 1) { y |= (1 << y_bit); }
			y_bit++;
			bit >>= 1;
		}
	}
}

bool Analize::convert(tpot::color_buffer_f &dst, const CONVERT_OPTIONS options)
{
	//	target.copy(dst); return true;// そのままコピー

	std::mt19937 mt;            // メルセンヌ・ツイスタの32ビット版
	std::random_device rnd;     // 非決定的な乱数生成器
	mt.seed(rnd());            // シード指定

	const tpot::color_buffer_f& target = *options.target;
	int w = target.width();
	int h = target.height();

	tpot::color_buffer_f work[2];
	work[0].Initialize(w, h);
	work[1].Initialize(w, h);

	tpot::color_buffer_f* buf_read = &work[0];
	tpot::color_buffer_f* buf_writ = &work[1];

	// 乱数で初期化
	#pragma omp parallel for
	for (int y = 0; y < h; y++) {
		tpot::color* p = buf_read->get_raw() + w * y;
		for (int x = 0; x < w; x++) {
			p->r = frand(mt());
			p->g = frand(mt());
			p->b = frand(mt());
			p++;
		}
	}

	// Z階数曲線でピクセルを決める
	int w_pow2 = 1, w_bit = 0;
	int h_pow2 = 1, h_bit = 0;
	while (w_pow2 < w) { w_pow2 *= 2; w_bit++; }
	while (h_pow2 < h) { h_pow2 *= 2; h_bit++; }
	int pixels_pow2 = w_pow2 * h_pow2;

	int radius_i = static_cast<int>(options.radius + 0.5f);
	int r2 = static_cast<int>(options.radius * options.radius + 0.5f);

	for (int i = 0; i < 1000; i++) {
		#pragma omp parallel for
		for (int i = 0; i < pixels_pow2; i++) {
			// 座標の検出
			int x, y;
			morton_order(x, y, i, w_bit, h_bit);
			if (w <= x || h <= y) continue;

			const tpot::color* rd = buf_read->see_raw();

			// 平均色に近づける
			tpot::color ave = {0.0f, 0.0f, 0.0f};
			float weight = 0.0f;
			for (int dy = -radius_i; dy <= radius_i; dy++) {
				int sy = y + dy;
				if (sy < 0 || h <= sy) continue;
				for (int dx = -radius_i; dx <= radius_i; dx++) {
					if (dx * dx + dy * dy < r2) continue;// 円形で見る
					int sx = x + dx;
					if (sx < 0 || w <= sx) continue;

					float we = 1.0f - (float)r2 / (float)(dx * dx + dy * dy);
					ave += rd[w * sy + sx] * we;
					weight += we;
				}
			}
			ave /= weight;

			const tpot::color *tgt = target.see_raw();
			tpot::color diff_ave = tgt[w * y + x] - ave;

			// 周辺のピクセルと離す
			tpot::color neigh_diff = { 0.0f, 0.0f, 0.0f };
			float weight_neigh = 0.0f;
			for (int dy = -radius_i; dy <= radius_i; dy++) {
				int sy = y + dy;
				if (sy < 0 || h <= sy) continue;
				for (int dx = -radius_i; dx <= radius_i; dx++) {
					if (dx * dx + dy * dy < r2) continue;// 円形で見る
					int sx = x + dx;
					if (sx < 0 || w <= sx) continue;

					tpot::color d = rd[w * y + x] - rd[w * sy + sx];
					float d2 = d.r * d.r + d.g * d.g + d.b * d.b;
					float we = 1.0f / (d2 + 0.00001f/* 発散除け*/);
					neigh_diff += d * we;
					weight_neigh += we;
				}
			}
			neigh_diff /= weight_neigh;

			// 彩度を上げる (1,1,1)ベクトルからの離れる方向に押す
			const tpot::color &c = rd[w * y + x];
			tpot::color CxI = { c.g - c.b, c.b - c.r, c.r - c.g };
			tpot::color IxCxI = { CxI.b - CxI.g, CxI.r - CxI.b, CxI.g - CxI.r };

			// 明度を中間に持っていく（白や黒があると輪郭として目立ちやすい）
			float lum = tpot::color::rgb2lab(c).r;
			tpot::color diff_lum = { 76.f - lum, 76.0f - lum, 76.f - lum };

			const float k_ave = 0.03f;// 大きいと半径のレンジで同じ色になっていく
			const float k_neigh = 0.05f;// 大きいとばらつく
			const float k_chro = 0.03f;
			const float k_lum = 0.001f;// 大きいと黒が消える
			tpot::color* wt = buf_writ->get_raw();
			wt[w * y + x] = rd[w * y + x]
				+ diff_ave * k_ave
				+ neigh_diff * k_neigh
				+ IxCxI * k_chro
				+ diff_lum * k_lum
				;

			// 色範囲を抑える
			wt[w * y + x].clamp();
		}

		// バッファの入れ替え
		tpot::color_buffer_f* buf_temp = buf_read;
		buf_read = buf_writ;
		buf_writ = buf_temp;
	}

	// 最終バッファに出力
	buf_read->copy(dst);

	return false;
}